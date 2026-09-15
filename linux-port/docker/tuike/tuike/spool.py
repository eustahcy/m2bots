"""The request/response directory Tuike shares with the game container.

The whole interface between a web panel and a running Metin2 server is a few
small key=value files on a shared volume. The panel writes a request; a helper
inside the game container (m2-supervise, m2-server-settings, m2-map-regens)
picks it up, does the work and writes back a status. The panel never gets a
shell, a socket or the game's admin port - which also accepts SHUTDOWN.

Every file is published atomically: written under a temporary name and then
moved or hard-linked into place, so a helper never reads half a request.
"""
import os
import re
import socket
import time
import uuid
from pathlib import Path

import pymysql

from . import config, db, engine
from .gamedata import bots as botdata
from .gamedata.actions import DEFAULT_GAME_LANGUAGE, GAME_LANGUAGE_NAMES
from .text import clamp

RATE_NAMES = engine.RATE_NAMES
RATE_LABELS = (("exp", "Doświadczenie"), ("drop", "Drop przedmiotów"), ("yang", "Drop Yang"))
RATE_MIN, RATE_MAX = 1, 10000
RESPAWN_MIN, RESPAWN_MAX = 1, 3600

SPOOL = config.RATES_SPOOL
AI_WEIGHTS_FILE = SPOOL / "playerbot_weights.tsv"

# A restart request older than this was abandoned; the console stops claiming
# one is under way.
RESTART_STALE_SECONDS = 600
# The settings helper rewrites its .ready file on a timer. Older than this and
# it is not running.
SETTINGS_READY_MAX_AGE = 20
# Only a request this old, with no helper alive, may be cleared by hand.
SETTINGS_STALE_SECONDS = 600


# --- reading ----------------------------------------------------------------
def read_values(path):
    """A small key=value status file written by a fixed helper."""
    result = {}
    try:
        content = Path(path).read_text(encoding="utf-8", errors="replace")
    except OSError:
        return result
    for line in content.splitlines():
        key, separator, value = line.partition("=")
        if separator:
            result[key.strip()] = value.strip()
    return result


def file_age(path):
    """Seconds since a file was last written, or None if it is not there."""
    try:
        return max(0, int(time.time() - Path(path).stat().st_mtime))
    except OSError:
        return None


def publish(name, lines, exclusive=False):
    """Write a request atomically. `exclusive` refuses to overwrite one queued.

    A hard link is an atomic, exclusive publication on a shared volume: it
    fails outright if the target exists, which is what "do not queue a second
    request on top of the first" needs. os.replace is the right call where the
    newest request simply wins.
    """
    SPOOL.mkdir(parents=True, exist_ok=True)
    temporary = SPOOL / f"{config.REQUEST_PREFIX}{uuid.uuid4().hex}.new"
    try:
        temporary.write_text("\n".join(lines) + "\n", encoding="utf-8")
        try:
            temporary.chmod(0o660)
        except OSError:
            pass
        if exclusive:
            os.link(temporary, SPOOL / name)
        else:
            os.replace(temporary, SPOOL / name)
    finally:
        temporary.unlink(missing_ok=True)


def request_id():
    return config.REQUEST_PREFIX + uuid.uuid4().hex


# --- rates ------------------------------------------------------------------
def rate_status():
    return read_values(SPOOL / "rates.status")


def read_rates():
    """The multipliers in force: what the helper last confirmed, else the table."""
    status = rate_status()
    if all(str(status.get(name, "")).isdigit() for name in RATE_NAMES):
        return {name: int(status[name]) for name in RATE_NAMES}
    values = {name: 100 for name in RATE_NAMES}
    try:
        for row in db.rows(f"SELECT name, value FROM {config.RATES_TABLE}"):
            if row["name"] in values:
                values[row["name"]] = int(row["value"])
    except (KeyError, ValueError, pymysql.MySQLError):
        pass
    return values


def validate_rates(form):
    """Read the three multipliers out of a form. Raises ValueError if bad."""
    values = {}
    for name in RATE_NAMES:
        raw = (form.get(name) or "").strip()
        try:
            value = int(raw)
        except ValueError:
            raise ValueError("Wpisz całkowite wartości liczbowe mnożników.")
        if not RATE_MIN <= value <= RATE_MAX:
            raise ValueError(f"Mnożniki muszą mieścić się w zakresie {RATE_MIN}–{RATE_MAX}%.")
        values[name] = value
    return values


def persist_rates(values):
    """On mt2009 a rate is six rows of player.quest, read by the db core at boot."""
    if not engine.IS_MT2009:
        return
    with db.connect() as connection, connection.cursor() as cursor:
        for name, flags in engine.MT2009_RATE_FLAGS.items():
            for flag in flags:
                cursor.execute(
                    "REPLACE INTO player.quest (dwPID, szName, szState, lValue) VALUES (0, %s, '', %s)",
                    (flag, int(values[name])),
                )
        # The classic panel's table too, so every panel shows the same numbers.
        try:
            for name in RATE_NAMES:
                cursor.execute(
                    f"INSERT INTO {config.RATES_TABLE} (name, value) VALUES (%s, %s) "
                    "ON DUPLICATE KEY UPDATE value=VALUES(value)",
                    (name, int(values[name])),
                )
        except pymysql.MySQLError:
            # An install whose classic panel never created the table is fine;
            # the quest rows above are what the game actually reads.
            pass
        connection.commit()


def queue_restart(values):
    """The plain path the game container has always watched: rates + restart."""
    persist_rates(values)
    now = int(time.time())
    publish("request", [
        f"id={request_id()}",
        f"exp={values['exp']}", f"drop={values['drop']}", f"yang={values['yang']}",
        f"time={now}",
    ])
    (SPOOL / "rates.status").write_text(
        "state=running\n"
        f"time={now}\nexp={values['exp']}\ndrop={values['drop']}\nyang={values['yang']}\n"
        f"message=restart requested by {config.PANEL_CODENAME}\n",
        encoding="utf-8",
    )


def restart_in_flight():
    status = rate_status()
    if status.get("state") != "running":
        return False
    try:
        started = int(status.get("time", "0"))
    except (TypeError, ValueError):
        return False
    return 0 < time.time() - started < RESTART_STALE_SECONDS


# --- the server-settings helper ---------------------------------------------
def port_open(port):
    try:
        with socket.create_connection((config.GAME_HOST, port), timeout=0.4):
            return True
    except OSError:
        return False


def settings_helper_status():
    """Whether the game-side helper is alive, not merely whether a file exists."""
    ready = read_values(SPOOL / "server-settings.ready")
    ready_age = file_age(SPOOL / "server-settings.ready")
    request_age = file_age(SPOOL / "server-settings.request")
    alive = (ready.get("capability") == "server-settings"
             and ready_age is not None and ready_age <= SETTINGS_READY_MAX_AGE)
    result = {
        "ready": alive,
        "ready_age": ready_age,
        "pending": request_age is not None,
        "request_age": request_age,
        "can_clear": bool(request_age is not None
                          and request_age >= SETTINGS_STALE_SECONDS and not alive),
    }
    if alive:
        result["message"] = "Helper ustawień serwera jest gotowy."
    elif result["pending"]:
        result["message"] = (
            "Zlecenie nie jest odbierane przez helper gry. Sprawdź instalację integracji; "
            "po 10 minutach można usunąć wyłącznie zaległe zlecenie.")
    else:
        # Telling the operator to install something this build does not ship is
        # not help, and both buttons that matter work without it.
        result["message"] = (
            "Ta wersja serwera nie zawiera silnikowej integracji respawnów, więc zmiana "
            "czasów odradzania jest niedostępna. Restart serwera i zmiana rat działają "
            "normalnie i niczego nie wymagają.")
    return result


def queue_server_settings(action, values=None, changes=None):
    """Apply rates and respawns in one restart, or restart on its own.

    Without the helper the respawn half has nothing to carry it out and is
    refused. A plain restart and a rates-only apply never needed it - the game
    container has always watched the rates spool - so those still go through.
    """
    support = settings_helper_status()
    if not support["ready"]:
        # An untouched respawn field arrives as "reset" for every map, so "no
        # respawn change" means "nothing but resets".
        respawn_changes = {key: value for key, value in (changes or {}).items() if value != "reset"}
        if action == "restart" or (action == "apply" and values and not respawn_changes):
            if restart_in_flight():
                raise FileExistsError("restart już trwa")
            queue_restart(values if action == "apply" else read_rates())
            return
        raise RuntimeError(support["message"])
    lines = [f"id={request_id()}", f"action={action}", "source=panel"]
    if action == "apply":
        lines.extend(f"{name}={values[name]}" for name in RATE_NAMES)
        lines.extend(f"map_{key}={value}" for key, value in (changes or {}).items())
    publish("server-settings.request", lines, exclusive=True)


def clear_stale_request():
    """Remove an abandoned request, but only when nothing could still read it."""
    if not settings_helper_status()["can_clear"]:
        raise RuntimeError(
            "Nie można usunąć zlecenia: helper może je jeszcze przetwarzać albo "
            "zlecenie nie jest wystarczająco stare.")
    (SPOOL / "server-settings.request").unlink()
    (SPOOL / "server-settings.status").write_text(
        "state=failed\npercent=0\n"
        "message=Usunięto zaległe zlecenie bez aktywnego helpera gry.\n"
        f"time={int(time.time())}\n",
        encoding="utf-8",
    )


def restart_progress():
    """How far a restart has got, from the helper if it reports, else by probe."""
    status = read_values(SPOOL / "server-settings.status")
    if status:
        status["percent"] = clamp(status.get("percent", 0), 0, 100, default=0)
        status["stage"] = status.get("message", "Oczekiwanie na stan serwera")
        if (SPOOL / "server-settings.request").exists() and status.get("state") != "running":
            status.update(state="running", percent=5, stage="Zlecenie oczekuje na serwer")
        return status
    # No helper: infer the stage from which ports answer. The auth server comes
    # up before the world, so the two probes together give three stages.
    rates = rate_status()
    auth, world = port_open(config.GAME_LOGIN_PORT), port_open(config.GAME_WORLD_PORT)
    state = rates.get("state", "unknown")
    if state == "running":
        if not auth:
            return {"percent": 25, "stage": "Zatrzymywanie procesów gry", "state": state}
        if not world:
            return {"percent": 65, "stage": "Serwer logowania działa — uruchamianie świata", "state": state}
        return {"percent": 85, "stage": "Sprawdzanie kanału i usług", "state": state}
    if state == "ok" and auth and world:
        return {"percent": 100, "stage": "Serwer działa", "state": state}
    if state == "failed":
        return {"percent": 100, "stage": rates.get("message", "Restart nie powiódł się"), "state": state}
    return {
        "percent": 100 if auth and world else 40,
        "stage": "Serwer działa" if auth and world else "Oczekiwanie na usługi",
        "state": state,
        "last_restart_time": rates.get("time", ""),
    }


# --- map respawns -----------------------------------------------------------
def map_regen_status():
    """Current per-map respawn overrides, as the helper last reported them."""
    result = {"state": "idle", "message": "Brak zapisanej zmiany", "values": {}, "stones": {}}
    for key, value in read_values(SPOOL / "map-regens.status").items():
        if key.startswith("map_stone_") and key[10:].isdigit():
            result["stones"][int(key[10:])] = value
        elif key.startswith("map_") and key[4:].isdigit():
            result["values"][int(key[4:])] = value
        else:
            result[key] = value
    return result


def queue_map_regens(changes):
    """The standalone respawn path, for a helper that handles only these."""
    now = int(time.time())
    publish("map-regens.request",
            [f"id={request_id()}", f"time={now}"]
            + [f"map_{key}={value}" for key, value in changes.items()])
    (SPOOL / "map-regens.status").write_text(
        f"state=running\ntime={now}\n"
        "message=Zapisano zestaw zmian respawnu; rdzenie zostaną ponownie uruchomione.\n",
        encoding="utf-8",
    )


# --- live bot behaviour -----------------------------------------------------
def read_ai_weights():
    """The live goal weights; anything the file does not mention is neutral."""
    values = {key: botdata.AI_WEIGHT_NEUTRAL for key, _label, _icon in botdata.AI_WEIGHT_KEYS}
    values.update(botdata.AI_LIVE_DEFAULTS)
    try:
        content = AI_WEIGHTS_FILE.read_text(encoding="utf-8", errors="replace")
    except OSError:
        return values
    for line in content.splitlines():
        fields = line.split("#", 1)[0].split()
        if len(fields) < 2:
            continue
        key, raw = fields[0].upper(), fields[1]
        if key not in values:
            continue
        if key in botdata.AI_SWITCHES:
            values[key] = 0 if raw.lower() in ("0", "off", "no") else 1
        elif key in botdata.AI_PERCENTAGES:
            values[key] = clamp(raw, 0, 100, default=values[key] or 0)
        elif key in botdata.AI_PERMILLE:
            values[key] = clamp(raw, 0, 1000, default=values[key] or 0)
        elif key in botdata.AI_REFINE_FLOOR:
            values[key] = clamp(raw, botdata.AI_REFINE_FLOOR_MIN,
                                botdata.AI_REFINE_FLOOR_MAX, default=values[key] or 0)
        else:
            values[key] = clamp(raw, botdata.AI_WEIGHT_MIN, botdata.AI_WEIGHT_MAX,
                                default=botdata.AI_WEIGHT_NEUTRAL)
    return values


def _preserved_weight_lines():
    """Keys a newer core understands and this panel does not - kept verbatim."""
    known = {key for key, _label, _icon in botdata.AI_WEIGHT_KEYS} | botdata.AI_SPECIAL_KEYS
    preserved = []
    try:
        content = AI_WEIGHTS_FILE.read_text(encoding="utf-8", errors="replace")
    except OSError:
        return preserved
    for line in content.splitlines():
        fields = line.split("#", 1)[0].split()
        if len(fields) != 2:
            continue
        key, value = fields[0].upper(), fields[1]
        if key not in known and re.fullmatch(r"[A-Z][A-Z0-9_]{0,63}", key) and re.fullmatch(r"-?\d{1,10}", value):
            preserved.append(f"{key}\t{value}")
    return preserved


# --- the language the game speaks -------------------------------------------
def game_language():
    """The language the game is in now.

    English when nothing says otherwise: that is what the server files ship as,
    and m2-lang writes the note only once it has switched.
    """
    code = read_values(SPOOL / "lang.status").get("lang", "").strip().lower()
    return code if code in GAME_LANGUAGE_NAMES else DEFAULT_GAME_LANGUAGE


def queue_language(code):
    """Ask the game container to switch languages.

    The panel only asks. m2-lang puts the four files in place and the
    supervisor restarts the cores, because they read those files while they
    boot and never again - so this disconnects anyone playing for the same half
    minute a rate change does.

    The client is the other half and this does not touch it: its menus and item
    names come from a pack chosen by the locale.cfg next to the executable.
    """
    if code not in GAME_LANGUAGE_NAMES:
        raise ValueError("Nieznany język.")
    if code == game_language():
        raise FileExistsError(f"Gra już mówi po {GAME_LANGUAGE_NAMES[code]}.")
    stamp = int(time.time())
    publish("lang.request", [f"id={request_id()}", f"lang={code}", f"time={stamp}"])


def write_ai_weights(values):
    """Replace the values this panel knows, without erasing newer-core settings.

    Every value is clamped here rather than only in the form that collects it:
    this function owns the file's format, and the core reads whatever it finds
    without arguing. A weight of 9999 in here is a world nobody can explain.
    """
    SPOOL.mkdir(parents=True, exist_ok=True)
    content = [
        f"# Metin2 Playerbots — wagi celów ustawione przez {config.PANEL_CODENAME}.",
        f"# {botdata.AI_WEIGHT_MIN} = rzadko · {botdata.AI_WEIGHT_NEUTRAL} = domyślnie"
        f" · {botdata.AI_WEIGHT_MAX} = często.",
        "# Rdzeń odczytuje plik co pięć sekund; restart nie jest wymagany.",
        "",
    ]
    content.extend(
        "{}\t{}".format(key, clamp(values.get(key), botdata.AI_WEIGHT_MIN,
                                   botdata.AI_WEIGHT_MAX, default=botdata.AI_WEIGHT_NEUTRAL))
        for key, _label, _icon in botdata.AI_WEIGHT_KEYS
    )
    for key in botdata.AI_SWITCHES:
        content.append(f"{key}\t{1 if values.get(key, 1) else 0}")
    for key in botdata.AI_PERCENTAGES:
        content.append(f"{key}\t{clamp(values.get(key), 0, 100, default=0)}")
    for key in botdata.AI_PERMILLE:
        if values.get(key) is not None:
            content.append(f"{key}\t{clamp(values.get(key), 0, 1000, default=0)}")
    for key in botdata.AI_REFINE_FLOOR:
        if values.get(key) is not None:
            content.append(f"{key}\t{clamp(values.get(key), botdata.AI_REFINE_FLOOR_MIN, botdata.AI_REFINE_FLOOR_MAX, default=0)}")
    content.extend(_preserved_weight_lines())
    temporary = AI_WEIGHTS_FILE.with_suffix(".tsv.new")
    temporary.write_text("\n".join(content) + "\n", encoding="utf-8")
    os.replace(temporary, AI_WEIGHTS_FILE)
