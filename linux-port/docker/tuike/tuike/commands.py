"""Asking the game to change one character, and what to do when it cannot.

The panel never edits a living character itself. It writes a row into
player.web_admin_queue, the in-game web_admin quest picks it up, does the work
with the engine's own rules and writes the answer back into the same row.

That leaves one case: the character is not logged in, so no quest can act for
it. Three of the commands - an item, some Yang, a level - are plain enough to
write straight into the database, and this module does that only after taking
the queue row away from the quest. Skip that step and a player who logs in
during the wait receives the reward twice. Warping and speed cannot be faked:
map_index cannot be derived from coordinates, so a hand-written warp can drop a
character into the void, and an affect has no database representation at all.
Those are refused with a reason rather than guessed at.
"""
import time

import pymysql

from . import config, db, engine
from .gamedata.actions import SPEED_DURATION_SECONDS, WARP_LOCATIONS

# Every word the in-game quest may leave in a row's status when it has finished,
# plus the two written outside it: the sweep's player_offline and the panel's
# own cancelled. Anything else in that column is the quest's claim stamp - it
# takes a row by writing a token there, does the work, and only then writes the
# result. Reading the stamp as an answer reported it to the operator as a
# failure, and the speed command hit it every time because its handler is the
# slowest in the quest.
FINAL_STATUSES = frozenset((
    "done", "bad_args", "no_skill", "has_item", "full", "failed",
    "qty_too_big", "partial", "no_gm", "unknown_cmd",
    "player_offline", "cancelled",
))
# Human wording for the ones an operator can actually meet.
STATUS_LABELS = {
    "done": "Wykonano",
    "bad_args": "Nieprawidłowe dane polecenia",
    "no_skill": "Warunek nie jest spełniony",
    "has_item": "Postać już to ma",
    "full": "Brak miejsca w ekwipunku",
    "failed": "Gra nie mogła tego wykonać",
    "qty_too_big": "Zbyt duża ilość jak na jedno wydanie",
    "partial": "Wydano mniej, niż proszono",
    "no_gm": "Postać nie ma uprawnień do tej komendy",
    "unknown_cmd": "Quest w grze wymaga aktualizacji",
}

# How long a request waits for the quest before the panel gives up. The quest
# answers an online character within a couple of ticks; longer than this and
# nobody is going to.
WAIT_SECONDS = 7.0
POLL_SECONDS = 0.6
# The quest waits this long before claiming a row for somebody who is not
# logged in, which is why a request aimed at an offline character leaves no
# evidence either way.
HELPER_SEEN_CACHE_SECONDS = 60

# player.item.count is the limit on a single stack the engine will create.
MAX_ITEM_COUNT = 200
MAX_LEVEL = 120
# How many inventory slots to search for a free one when writing an item
# directly. r40250 gives a character four pages of 45.
INVENTORY_SLOTS = 180

_helper_cache = {"at": 0.0, "seen": False}


class CommandError(ValueError):
    """Something the operator can fix, or a limit worth explaining."""


def helper_seen():
    """Has the in-game helper ever answered on this server?

    Evidence, not configuration: a row that ever reached any status other than
    pending or cancelled was moved by the quest, because nothing else touches
    them. Without this the panel cannot tell "the player is not logged in" from
    "nobody is listening", and reporting the first for both tells operators
    their character was not in game while they were standing in it.

    Cached for a minute, and once the answer is yes it never goes back.
    """
    if _helper_cache["seen"]:
        return True
    if time.time() - _helper_cache["at"] < HELPER_SEEN_CACHE_SECONDS:
        return False
    _helper_cache["at"] = time.time()
    try:
        row = db.one(
            f"SELECT 1 AS seen FROM {config.GAME_QUEUE_TABLE}"
            " WHERE status NOT IN ('pending','cancelled') LIMIT 1")
    except pymysql.MySQLError:
        return False
    _helper_cache["seen"] = bool(row)
    return _helper_cache["seen"]


def queue_and_wait(name, cmd, arg1, arg2, wait=None):
    """Queue one command and wait for the quest. Returns (status, row id).

    On "timeout" the row is still pending and the quest may yet pick it up, so
    the caller MUST claim the row before applying anything itself.
    """
    wait = WAIT_SECONDS if wait is None else wait
    with db.connect() as connection, connection.cursor() as cursor:
        cursor.execute(
            f"INSERT INTO {config.GAME_QUEUE_TABLE} (player_name, cmd, arg1, arg2)"
            " VALUES (%s, %s, %s, %s)",
            (name, cmd, str(arg1), str(arg2)))
        queue_id = cursor.lastrowid

    deadline = time.time() + wait
    while time.time() < deadline:
        time.sleep(POLL_SECONDS)
        row = db.one(f"SELECT status FROM {config.GAME_QUEUE_TABLE} WHERE id = %s", (queue_id,))
        if not row:
            # Something removed the row. Never apply on top of that.
            return "gone", queue_id
        if row.get("status") in FINAL_STATUSES:
            return row["status"], queue_id
    return "timeout", queue_id


def _claim(cursor, queue_id, status):
    """Take the row away from the quest before writing anything ourselves."""
    cursor.execute(
        f"UPDATE {config.GAME_QUEUE_TABLE} SET status = 'cancelled'"
        " WHERE id = %s AND status = 'pending'", (queue_id,))
    if cursor.rowcount == 1:
        return True
    # The quest itself reported the player as offline, so the row is already
    # out of the pending pool and nobody will deliver it.
    return status == "player_offline"


def _apply_offline(cursor, pid, cmd, arg1, arg2, status, name):
    """Write the change straight into the database. Only ITEM, GOLD and LEVEL."""
    if cmd == "GOLD":
        cursor.execute("UPDATE player.player SET gold = GREATEST(0, gold + %s) WHERE id = %s",
                       (int(arg1), pid))
        return
    if cmd == "LEVEL":
        cursor.execute("UPDATE player.player SET level = %s WHERE id = %s", (int(arg1), pid))
        return
    if cmd == "ITEM":
        cursor.execute(
            "SELECT pos FROM player.item WHERE owner_id = %s AND window = 'INVENTORY'", (pid,))
        used = {row["pos"] for row in cursor.fetchall()}
        free = next((slot for slot in range(INVENTORY_SLOTS) if slot not in used), None)
        if free is None:
            raise CommandError(f"{name}: plecak jest pełny, nie ma gdzie odłożyć przedmiotu.")
        cursor.execute(
            "INSERT INTO player.item (owner_id, window, pos, count, vnum)"
            " VALUES (%s, 'INVENTORY', %s, %s, %s)", (pid, free, int(arg2), int(arg1)))
        return
    # Warp and speed need the quest. Faking a warp by writing x/y/map_index is
    # not safe, and an affect is not stored anywhere we could write it.
    if status == "player_offline":
        raise CommandError(
            f"{name} nie jest w grze, a tego nie da się zrobić poza grą. "
            "Teleport i prędkość wymagają zalogowanej postaci.")
    if not helper_seen():
        raise CommandError(
            "Ta instalacja nie ma w grze questa web_admin, więc teleport i prędkość "
            "są niedostępne. Przedmioty, Yang i poziom działają także bez niego.")
    raise CommandError(
        f"Gra nie odpowiedziała w ciągu {int(WAIT_SECONDS)} s. Spróbuj ponownie, "
        f"gdy {name} będzie w świecie.")


def normalise(cmd, form):
    """Turn one action form into (cmd, arg1, arg2). Raises CommandError."""
    if cmd == "ITEM":
        vnum = (form.get("vnum") or "").strip()
        if not vnum.isdigit() or not 1 <= int(vnum) <= 2147483647:
            raise CommandError("Podaj prawidłowy VNUM przedmiotu.")
        try:
            quantity = int((form.get("quantity") or "1").strip())
        except ValueError:
            raise CommandError("Ilość podaje się liczbą całkowitą.")
        if not 1 <= quantity <= MAX_ITEM_COUNT:
            raise CommandError(f"Ilość musi mieścić się w zakresie 1–{MAX_ITEM_COUNT}.")
        return cmd, vnum, str(quantity)

    if cmd == "GOLD":
        preset = (form.get("preset") or "").strip()
        raw = (form.get("amount") or "").strip() if preset == "custom" else preset
        try:
            amount = int(raw)
        except (TypeError, ValueError):
            raise CommandError("Podaj kwotę Yang jako liczbę całkowitą.")
        if amount == 0:
            raise CommandError("Kwota zero niczego nie zmienia.")
        if abs(amount) > 2_000_000_000:
            raise CommandError("Kwota jest poza zakresem, jaki gra potrafi zapisać.")
        return cmd, str(amount), "1"

    if cmd == "LEVEL":
        try:
            level = int((form.get("level") or "").strip())
        except ValueError:
            raise CommandError("Poziom podaje się liczbą całkowitą.")
        # Checked here rather than left to the server, which does not refuse
        # it: PointChange returns without a word and everything upstream
        # reports success.
        if not 1 <= level <= MAX_LEVEL:
            raise CommandError(f"Poziom musi mieścić się w zakresie 1–{MAX_LEVEL}.")
        return cmd, str(level), "1"

    if cmd == "WARP":
        target = (form.get("target") or "").strip()
        if target not in {coords for _emoji, _name, coords in WARP_LOCATIONS}:
            raise CommandError("Wybierz jedno z dostępnych miejsc.")
        x, y = target.split(" ", 1)
        return cmd, x, y

    if cmd == "SPEED":
        raw = (form.get("percent") or "").strip()
        if not raw.lstrip("-").isdigit() or not 0 <= int(raw) <= 100:
            raise CommandError("Wybierz jedną z dostępnych prędkości.")
        return cmd, raw, str(SPEED_DURATION_SECONDS)

    raise CommandError("Nieznana czynność.")


# --- "warp me to this bot" ---------------------------------------------------
# The reverse of everything above: instead of moving a bot, this moves whoever
# is operating the panel to a bot's location, so they can go look at it.
#
# The database only knows who played most recently, not who is in the game
# right now - last_play is written when a character saves, minutes after
# login. Picking the newest last_play once queued a warp for the character who
# played BEFORE the one actually sitting in the game, which answered nothing
# and left the row waiting to fire on that other character's next login. The
# honest fix: ask every recently active human character to warp at once,
# accept whichever one actually answers, and withdraw the rest before their
# turn comes.
RECENT_HUMAN_DAYS = 7
RECENT_HUMAN_LIMIT = 8
WARP_ME_WAIT_SECONDS = 6.0


def _recent_human_names():
    """Characters that are not Playerbots and have played in the last week."""
    return [row["name"] for row in db.rows(
        f"SELECT name FROM player.player WHERE NOT {engine.BOT_IS_BARE}"
        " AND last_play >= NOW() - INTERVAL %s DAY"
        " ORDER BY last_play DESC LIMIT %s",
        (RECENT_HUMAN_DAYS, RECENT_HUMAN_LIMIT))]


def warp_operator_to(x, y):
    """Teleport whichever human character is actually online. Returns its name."""
    names = _recent_human_names()
    if not names:
        raise CommandError(
            f"Żadna postać gracza nie logowała się w ostatnich {RECENT_HUMAN_DAYS} dniach.")

    with db.connect() as connection, connection.cursor() as cursor:
        cursor.executemany(
            f"INSERT INTO {config.GAME_QUEUE_TABLE} (player_name, cmd, arg1, arg2)"
            " VALUES (%s, 'WARP', %s, %s)",
            [(name, str(x), str(y)) for name in names])
        # lastrowid after executemany is unreliable across drivers for
        # anything but the last row, so the candidate rows are looked back up
        # by what was just written rather than trusted from the insert.
        marks = ",".join(["%s"] * len(names))
        cursor.execute(
            f"SELECT id, player_name FROM {config.GAME_QUEUE_TABLE}"
            f" WHERE cmd = 'WARP' AND status = 'pending' AND arg1 = %s AND arg2 = %s"
            f" AND player_name IN ({marks})",
            (str(x), str(y), *names))
        candidates = {row["id"]: row["player_name"] for row in cursor.fetchall()}

    if not candidates:
        raise CommandError("Nie udało się zapisać zlecenia teleportu.")
    ids = list(candidates)
    marks = ",".join(["%s"] * len(ids))

    moved_name, final_status = None, "timeout"
    deadline = time.time() + WARP_ME_WAIT_SECONDS
    while time.time() < deadline and moved_name is None:
        time.sleep(POLL_SECONDS)
        for row in db.rows(f"SELECT id, status FROM {config.GAME_QUEUE_TABLE} WHERE id IN ({marks})", ids):
            if row.get("status") not in ("pending", None):
                moved_name, final_status = candidates.get(row["id"]), row["status"]
                break

    # Nobody else gets teleported later for a click made now.
    db.execute(
        f"DELETE FROM {config.GAME_QUEUE_TABLE} WHERE status = 'pending' AND id IN ({marks})", ids)

    if moved_name is None:
        raise CommandError("Żadna z ostatnio aktywnych postaci gracza nie jest teraz w grze.")
    if final_status != "done":
        raise CommandError(f"{moved_name}: {STATUS_LABELS.get(final_status, final_status)}.")
    return moved_name


def run(pid, cmd, arg1, arg2):
    """Carry out one action. Returns the message to show. Raises CommandError."""
    character = db.one("SELECT name FROM player.player WHERE id = %s", (pid,))
    if not character:
        raise CommandError("Tej postaci już nie ma.")
    name = character["name"]

    status, queue_id = queue_and_wait(name, cmd, arg1, arg2)
    if status == "done":
        return f"{name}: wykonano w grze."
    if status not in ("player_offline", "timeout", "gone"):
        raise CommandError(f"{name}: {STATUS_LABELS.get(status, status)}.")
    if status == "gone":
        raise CommandError("Zlecenie zniknęło z kolejki gry. Nic nie zostało zastosowane.")

    with db.connect() as connection, connection.cursor() as cursor:
        if not _claim(cursor, queue_id, status):
            # The quest grabbed it while we were waiting - do NOT apply again.
            row = db.one(f"SELECT status FROM {config.GAME_QUEUE_TABLE} WHERE id = %s", (queue_id,))
            late = row.get("status", "gone") if row else "gone"
            if late == "done":
                return f"{name}: gra zdążyła to wykonać tuż przed upływem czasu."
            raise CommandError(f"{name}: gra odebrała zlecenie i odpowiedziała "
                               f"„{STATUS_LABELS.get(late, late)}”.")
        _apply_offline(cursor, pid, cmd, arg1, arg2, status, name)

    if status == "timeout" and not helper_seen():
        return (f"{name}: zapisano bezpośrednio w bazie. W tej instalacji nie działa quest "
                "web_admin, więc zmiana będzie widoczna przy następnym zalogowaniu.")
    return (f"{name} nie jest w grze, więc zmiana została zapisana w bazie "
            "i pojawi się przy następnym zalogowaniu.")
