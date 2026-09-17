"""The character list, one character's profile, and the actions on it."""
from flask import Blueprint, abort, flash, redirect, render_template, request, url_for

from .. import commands, gm, live
from ..gamedata.actions import (GM_RANKS, GOLD_PRESETS, SPEED_PRESETS, WARP_LOCATIONS,
                                gm_rank_label)
from ..gamedata.characters import EQUIPMENT_LAYOUT, INVENTORY_COLUMNS, INVENTORY_PAGE_SIZE
from ..queries import characters as queries
from ..queries import world
from ..security import login_required, require_csrf

bp = Blueprint("characters", __name__)


@bp.route("/players")
@login_required
def roster():
    query = request.args.get("q", "").strip()
    totals = world.totals()
    bots = world.bot_count()
    return render_template(
        "characters/roster.html",
        characters=queries.roster(query),
        query=query,
        counts={
            "characters": totals.get("characters", 0),
            "bots": bots,
            "players": max(0, int(totals.get("characters") or 0) - int(bots or 0)),
            "live": len(live.statuses()),
        },
    )


@bp.route("/player/<int:pid>")
@login_required
def profile(pid):
    character = queries.profile(pid)
    if not character:
        abort(404)
    account_id = character.get("account_id")
    name = character["name"]
    character = queries.enrich(character, live.statuses().get(pid))
    return render_template(
        "characters/profile.html",
        character=character,
        logs=queries.recent_logs(pid),
        gear_history=queries.gear_history(pid),
        stats=queries.stat_summary(pid),
        ties=queries.ties(pid),
        offline_shop=queries.offline_shop(pid),
        layout=EQUIPMENT_LAYOUT,
        page_size=INVENTORY_PAGE_SIZE,
        columns=INVENTORY_COLUMNS,
        # The admin tools, and everything they offer.
        warp_locations=WARP_LOCATIONS,
        speed_presets=SPEED_PRESETS,
        gold_presets=GOLD_PRESETS,
        gm_ranks=GM_RANKS,
        gm_rank=gm.rank_of(name),
        gm_rank_label=gm_rank_label,
        max_item_count=commands.MAX_ITEM_COUNT,
        max_level=commands.MAX_LEVEL,
        helper_seen=commands.helper_seen(),
        **queries.belongings(pid, account_id),
    )


@bp.post("/player/<int:pid>/action")
@login_required
def action(pid):
    """Ask the game to change this character - an item, Yang, a level, a warp."""
    require_csrf()
    try:
        cmd, arg1, arg2 = commands.normalise(request.form.get("cmd", ""), request.form)
        flash(commands.run(pid, cmd, arg1, arg2))
    except commands.CommandError as error:
        flash(str(error), "error")
    except Exception:
        # A database that went away mid-action, most likely. The operator needs
        # to know it did not happen, not to see a traceback.
        flash("Nie udało się wykonać czynności. Sprawdź, czy baza i gra odpowiadają.", "error")
    return redirect(url_for("characters.profile", pid=pid))


@bp.post("/player/<int:pid>/warp-me")
@login_required
def warp_me(pid):
    """Teleport the operator's own character to this bot's last known position.

    A JSON endpoint rather than a redirect: the reference this borrows from
    shows it as a one-click button with an inline result, not a page reload,
    and the several seconds it takes to fan out and wait for an answer reads
    far better as a status line than as a stalled navigation.
    """
    require_csrf()
    character = queries.profile(pid)
    if not character:
        abort(404)
    try:
        name = commands.warp_operator_to(character["x"], character["y"])
        return {"ok": True, "name": name}
    except commands.CommandError as error:
        return {"ok": False, "error": str(error)}
    except Exception:
        return {"ok": False, "error": "Nie udało się połączyć z bazą lub kolejką gry."}


@bp.post("/player/<int:pid>/warp-shop")
@login_required
def warp_shop(pid):
    """Teleport the operator's character to this character's stall.

    The stall's own coordinates, not the owner's: IkarusShop keeps a stall
    open after its owner logs off or wanders away, and the stall is where the
    operator wants to stand.
    """
    require_csrf()
    shop = queries.offline_shop(pid)
    if not shop:
        return {"ok": False, "error": "Ta postać nie ma otwartego straganu."}
    try:
        name = commands.warp_operator_to(shop["x"], shop["y"])
        return {"ok": True, "name": name}
    except commands.CommandError as error:
        return {"ok": False, "error": str(error)}
    except Exception:
        return {"ok": False, "error": "Nie udało się połączyć z bazą lub kolejką gry."}


@bp.post("/player/<int:pid>/gm")
@login_required
def set_gm(pid):
    """Grant or revoke the in-game admin commands for this character."""
    require_csrf()
    try:
        flash(gm.set_rank(pid, (request.form.get("rank") or "").strip()))
    except ValueError as error:
        flash(str(error), "error")
    except Exception:
        flash("Nie udało się zmienić rangi. Sprawdź, czy baza odpowiada.", "error")
    return redirect(url_for("characters.profile", pid=pid))
