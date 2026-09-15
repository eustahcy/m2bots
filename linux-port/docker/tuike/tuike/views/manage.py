"""The console: multipliers, respawns, bot behaviour, restarts and updates.

Nothing here touches a character. The two halves are deliberately different in
kind: behaviour is a file the core re-reads every five seconds and needs no
restart, while multipliers and respawns are read only at boot and therefore
always come with one.
"""
from flask import Blueprint, flash, redirect, render_template, request, url_for

from .. import live, settings, spool, updater
from ..gamedata import bots as botdata
from ..gamedata.actions import GAME_LANGUAGE_NAMES, GAME_LANGUAGES, RATE_PRESETS
from ..gamedata.maps import MAP_RESPAWN_OPTIONS, MAP_STONE_RESPAWN_IDS
from ..queries import world
from ..security import login_required, protected_admin, require_csrf, sign_out
from ..text import clamp

bp = Blueprint("manage", __name__)

RESTART_CONFIRMATION = "RESTART"


def _back(tab):
    """Redirect to /manage, landing on the tab the form that was just
    submitted actually lives on - the tabs are pure client-side state
    (see manage.js), so the fragment is the only way a redirect can say
    which one that is."""
    return redirect(url_for("manage.index") + f"#{tab}")


@bp.route("/manage")
@login_required
def index():
    current = settings.read()
    update_state = updater.status()
    update_state["protected"] = protected_admin()
    return render_template(
        "manage/index.html",
        rates=spool.read_rates(),
        rate_labels=spool.RATE_LABELS,
        rate_presets=RATE_PRESETS,
        languages=GAME_LANGUAGES,
        language=spool.game_language(),
        language_name=GAME_LANGUAGE_NAMES.get(spool.game_language(), "English"),
        status=world.server_status(),
        restart=spool.restart_progress(),
        helper=spool.settings_helper_status(),
        respawn_options=MAP_RESPAWN_OPTIONS,
        stone_respawn_ids=MAP_STONE_RESPAWN_IDS,
        respawn_status=spool.map_regen_status(),
        weights=spool.read_ai_weights(),
        weight_keys=botdata.AI_WEIGHT_KEYS,
        weight_bounds=(botdata.AI_WEIGHT_MIN, botdata.AI_WEIGHT_MAX, botdata.AI_WEIGHT_NEUTRAL),
        chest_suggestions=botdata.AI_CHEST_SUGGESTIONS,
        refine_floor_suggestions=botdata.AI_REFINE_FLOOR_SUGGESTION,
        refine_floor_bounds=(botdata.AI_REFINE_FLOOR_MIN, botdata.AI_REFINE_FLOOR_MAX),
        updater=update_state,
        release=updater.release_status(),
        map_load=world.named_map_load(),
        bot_count=len(live.bots(current)),
        themes=settings.THEMES,
        densities=settings.DENSITIES,
        monitor_modes=settings.MONITOR_MODES,
        confirmation=RESTART_CONFIRMATION,
    )


# --- appearance and access --------------------------------------------------
@bp.post("/manage/settings")
@login_required
def apply_settings():
    require_csrf()
    values, error = settings.validate_display(request.form)
    if error:
        flash(error, "error")
        return _back("appearance")
    current = settings.read()
    password_hash, error = settings.resolve_password(request.form, current)
    if error:
        flash(error, "error")
        return _back("appearance")
    protect = request.form.get("auth_enabled") == "1"
    if not protect and current.get("auth_enabled") == "1":
        # Turning protection off drops the hash, so nothing that could be
        # brute-forced is left behind - and the current session with it.
        sign_out()
    values.update({"auth_enabled": "1" if protect else "0",
                   "auth_password_hash": password_hash, "setup_complete": "1"})
    settings.write(values)
    flash("Ustawienia panelu zapisane.")
    return _back("appearance")


# --- multipliers and respawns -----------------------------------------------
def _respawn_changes(form):
    """Read the respawn grid. An empty field means "restore the default"."""
    changes = {}
    for index, name in MAP_RESPAWN_OPTIONS:
        for prefix in ("", "stone_"):
            if prefix and index not in MAP_STONE_RESPAWN_IDS:
                continue
            key = f"{prefix}{index}"
            field = f"map_{key}"
            # A browser tab opened before the Metin fields existed does not
            # send them; missing is not the same as blank.
            if field not in form:
                continue
            raw = form[field].strip()
            if not raw:
                changes[key] = "reset"
                continue
            try:
                seconds = int(raw)
            except ValueError:
                raise ValueError(f"{name}: respawn podaje się w całych sekundach.")
            if not spool.RESPAWN_MIN <= seconds <= spool.RESPAWN_MAX:
                raise ValueError(f"{name}: respawn musi mieścić się w zakresie "
                                 f"{spool.RESPAWN_MIN}–{spool.RESPAWN_MAX} sekund.")
            changes[key] = seconds
    return changes


@bp.post("/manage/restart-config")
@login_required
def restart_config():
    require_csrf()
    action = request.form.get("submit_action", "apply")
    if action not in ("apply", "restart"):
        flash("Nieprawidłowa akcja.", "error")
        return _back("world")
    try:
        values = spool.validate_rates(request.form) if action == "apply" else {}
        changes = _respawn_changes(request.form) if action == "apply" else {}
        spool.queue_server_settings(action, values, changes)
    except ValueError as error:
        flash(str(error), "error")
    except RuntimeError as error:
        flash(str(error), "error")
    except FileExistsError:
        flash("Poprzednie zlecenie nadal trwa. Poczekaj na zakończenie restartu.", "error")
    except OSError:
        flash("Nie udało się zapisać zlecenia do kolejki gry.", "error")
    else:
        flash("Zestaw zapisany do kolejki: jeden restart zastosuje raty i respawn."
              if action == "apply" else
              "Zlecono restart bez zapisywania zmian w formularzu.")
    return _back("world")


@bp.post("/manage/clear-stale")
@login_required
def clear_stale():
    require_csrf()
    try:
        spool.clear_stale_request()
    except RuntimeError as error:
        flash(str(error), "error")
    except OSError:
        flash("Nie udało się usunąć zaległego zlecenia z kolejki.", "error")
    else:
        flash("Usunięto zaległe zlecenie. Zainstaluj integrację gry, zanim zlecisz kolejną zmianę.")
    return _back("world")


@bp.post("/manage/restart")
@login_required
def restart():
    require_csrf()
    if request.form.get("confirmation", "").strip().upper() != RESTART_CONFIRMATION:
        flash(f"Aby potwierdzić restart, wpisz {RESTART_CONFIRMATION}.", "error")
        return _back("world")
    try:
        spool.queue_restart(spool.read_rates())
    except OSError:
        flash("Nie udało się zapisać zlecenia restartu.", "error")
    else:
        flash("Restart został zlecony. Pasek postępu pokaże kolejne etapy.")
    return _back("world")


# --- live bot behaviour -----------------------------------------------------
@bp.post("/manage/behavior")
@login_required
def behavior():
    require_csrf()
    # Start from what the file holds, so a form opened before a field existed
    # cannot silently reset it.
    values = spool.read_ai_weights()
    for key, _label, _icon in botdata.AI_WEIGHT_KEYS:
        values[key] = clamp(request.form.get(key), botdata.AI_WEIGHT_MIN,
                            botdata.AI_WEIGHT_MAX, default=botdata.AI_WEIGHT_NEUTRAL)
    for key in botdata.AI_SWITCHES:
        if key in request.form:
            values[key] = 1 if "1" in request.form.getlist(key) else 0
    for key in botdata.AI_PERCENTAGES:
        if key in request.form:
            values[key] = clamp(request.form.get(key), 0, 100,
                                default=botdata.AI_LIVE_DEFAULTS[key])
    for key in botdata.AI_PERMILLE:
        if key not in request.form:
            continue
        parsed = clamp(request.form.get(key), 0, 1000, default=None)
        # A malformed chest field must not turn a real server value into a
        # guessed default, so it is left exactly as it was.
        if parsed is not None:
            values[key] = parsed
    for key in botdata.AI_REFINE_FLOOR:
        if key not in request.form:
            continue
        parsed = clamp(request.form.get(key), botdata.AI_REFINE_FLOOR_MIN,
                       botdata.AI_REFINE_FLOOR_MAX, default=None)
        if parsed is not None:
            values[key] = parsed
    try:
        spool.write_ai_weights(values)
    except OSError:
        flash("Nie udało się zapisać wag Playerbots.", "error")
    else:
        flash("Zachowanie botów zapisane — nowy plan działania wejdzie w życie "
              "do 5 sekund, bez restartu.")
    return _back("behavior")


# --- the language the game speaks -------------------------------------------
@bp.post("/manage/language")
@login_required
def language():
    require_csrf()
    code = (request.form.get("lang") or "").strip().lower()
    try:
        spool.queue_language(code)
    except ValueError as error:
        flash(str(error), "error")
    except FileExistsError as error:
        flash(str(error))
    except OSError:
        flash("Nie udało się zapisać zlecenia zmiany języka do kolejki gry.", "error")
    else:
        # Said here rather than afterwards: the game container picks the request
        # up within five seconds and then restarts the cores, so by the time
        # this page comes back the switch is under way but not finished.
        flash(f"Zlecono zmianę języka gry na {GAME_LANGUAGE_NAMES[code]}. "
              "Rdzenie zostaną zaraz uruchomione ponownie — gracze rozłączą się "
              "na pół minuty. Klient gry ma własny pakiet językowy i ta zmiana go nie dotyczy.")
    return _back("world")


# --- the isolated updater ---------------------------------------------------
@bp.post("/manage/update")
@login_required
def update():
    require_csrf()
    if not protected_admin():
        flash("Aktualizacje z panelu wymagają włączonej ochrony hasłem.", "error")
        return _back("updates")
    try:
        updater.queue()
    except (OSError, RuntimeError) as error:
        flash(str(error), "error")
    else:
        flash("Przyjęto zlecenie aktualizacji. Serwer zostanie przebudowany przez "
              "odizolowany updater; postęp jest widoczny poniżej.")
    return _back("updates")
