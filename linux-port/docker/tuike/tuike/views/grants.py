"""Mass item grants: choose the item and the conditions, preview, then send."""
import json
import secrets
import time

from flask import Blueprint, abort, flash, redirect, render_template, request, session, url_for

from .. import config, db, grants
from ..gamedata import items as itemdata
from ..queries import economy
from ..schema import ensure
from ..security import login_required, require_csrf
from ..text import game_text

bp = Blueprint("grants", __name__)

PREVIEW_KEY = "grant_preview"
DEFAULT_VNUM = 50051  # "Zdjęcie Konia" - the thing an operator grants most often.


def _requested_item(values):
    """The vnum and quantity from the form, both checked."""
    try:
        vnum = int(values.get("vnum", DEFAULT_VNUM))
    except (TypeError, ValueError):
        raise grants.GrantError("Nieprawidłowy VNUM.")
    if not 1 <= vnum <= 2147483647:
        raise grants.GrantError("Nieprawidłowy VNUM.")
    quantity = grants.number(values.get("quantity", "1"), "Ilość",
                             grants.MAX_ITEM_COUNT, allow_empty=False)
    return vnum, quantity


def _fingerprint(vnum, quantity, criteria, only_missing):
    """What the preview was of. Sending something else has to preview again."""
    return json.dumps((vnum, quantity, criteria, only_missing), sort_keys=True)


def _send(cursor, connection, vnum, quantity, criteria, only_missing, fingerprint):
    """Write one row per recipient, under a lock, inside a transaction."""
    preview = session.get(PREVIEW_KEY)
    if (not preview or preview.get("fingerprint") != fingerprint
            or time.time() - preview["time"] > grants.PREVIEW_TTL_SECONDS):
        raise grants.GrantError("Najpierw odśwież podgląd odbiorców.")
    cursor.execute("SELECT GET_LOCK(%s, 10) AS acquired", (config.GRANTS_LOCK,))
    if cursor.fetchone()["acquired"] != 1:
        abort(409, "Inne nadanie jest właśnie zapisywane.")
    try:
        connection.begin()
        # Re-read inside the transaction: the preview may be minutes old and
        # the world moves.
        recipients = grants.candidates(cursor, vnum, criteria, only_missing)
        grants.record(cursor, preview["batch"], recipients, vnum, quantity,
                      criteria, only_missing)
        connection.commit()
    except Exception:
        connection.rollback()
        raise
    finally:
        cursor.execute("SELECT RELEASE_LOCK(%s)", (config.GRANTS_LOCK,))
    session.pop(PREVIEW_KEY, None)
    return recipients


@bp.route("/manage/items", methods=["GET", "POST"])
@login_required
def index():
    try:
        vnum, quantity = _requested_item(request.values)
        criteria = grants.criteria_from(request.values)
    except grants.GrantError as error:
        abort(400, str(error))
    # A hidden "0" precedes the checkbox, so an unchecked box arrives as 0
    # rather than not arriving at all.
    only_missing = (request.values.getlist("only_missing") or ["1"])[-1] == "1"

    with db.connect() as connection, connection.cursor() as cursor:
        # The grant tables may not exist yet on a stack whose collector has
        # never run, and this page is the one that needs them.
        ensure(cursor)

        if request.method == "POST":
            require_csrf()
            if request.form.get("action") == "cancel":
                cancelled = grants.cancel_batch(request.form.get("batch", ""))
                flash(f"Anulowano oczekujące nadania: {cancelled}.")
                return redirect(url_for("grants.index", vnum=vnum))

        prototype = economy.prototype(vnum)
        if not prototype:
            abort(404, "Nie ma przedmiotu o tym VNUM.")
        if not 1 <= int(prototype["size"] or 0) <= 3 or int(prototype["type"] or 0) in itemdata.UNGRANTABLE_ITEM_TYPES:
            abort(400, "Ten przedmiot nie jest obsługiwany przez zwykły ekwipunek.")
        prototype["name"] = game_text(prototype["locale_name"])

        fingerprint = _fingerprint(vnum, quantity, criteria, only_missing)
        if request.method == "POST":
            try:
                recipients = _send(cursor, connection, vnum, quantity, criteria,
                                   only_missing, fingerprint)
            except grants.GrantError as error:
                abort(400, str(error))
            flash(f"Zlecono {quantity}× VNUM {vnum} dla {len(recipients)} postaci.")
            return redirect(url_for("grants.index", vnum=vnum))

        recipients = grants.candidates(cursor, vnum, criteria, only_missing)
        session[PREVIEW_KEY] = {"fingerprint": fingerprint,
                                "batch": secrets.token_hex(16), "time": time.time()}
        return render_template(
            "grants.html",
            item=prototype, vnum=vnum, quantity=quantity, criteria=criteria,
            criteria_text=grants.criteria_text, only_missing=only_missing,
            recipients=recipients, jobs=grants.JOBS, fields=grants.CRITERIA_FIELDS,
            max_quantity=grants.MAX_ITEM_COUNT, max_pending=grants.MAX_PENDING,
            stats=grants.worker_stats(cursor), history=grants.history(cursor),
        )
