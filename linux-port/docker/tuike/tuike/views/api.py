"""The JSON the pages poll.

Every endpoint here is behind the same login as the page that uses it, returns
{"ok": true, ...}, and is cheap enough to be called every second and a half.
"""
from datetime import datetime
from urllib.parse import quote

from flask import Blueprint, abort, request, url_for

from .. import live, settings, spool, updater
from ..gamedata import characters as chardata
from ..gamedata import items as itemdata
from ..gamedata.maps import MAP_BOUNDS, MAP_NAMES
from ..queries import economy, rankings, world
from ..security import login_required, protected_admin

bp = Blueprint("api", __name__, url_prefix="/api")


def _item_icon_url(vnum):
    icon = itemdata.icon_file(vnum)
    return url_for("static", filename=f"icons/{quote(icon)}") if icon else None


def _portrait_url(job):
    return url_for("static", filename=f"class-portraits/{chardata.class_profile(job)['portrait']}")


@bp.route("/live-bots")
@login_required
def live_bots():
    """Every placeable bot, with the bounds the page needs to draw them."""
    return {
        "ok": True,
        "updated_at": int(datetime.now().timestamp() * 1000),
        "maps": MAP_NAMES,
        "bounds": MAP_BOUNDS,
        "leader_id": rankings.global_leader_id(),
        "bots": live.bots(),
    }


@bp.route("/news")
@login_required
def news():
    return {"ok": True, "events": world.news_events()}


@bp.route("/system")
@login_required
def system():
    return {"ok": True, "system": world.telemetry_now()}


@bp.route("/manage-status")
@login_required
def manage_status():
    """Everything the console and the dashboard refresh without a reload."""
    update_state = updater.status()
    update_state["protected"] = protected_admin()
    restart = spool.restart_progress()
    return {
        "ok": True,
        "restart": restart,
        "helper": spool.settings_helper_status(),
        "updater": update_state,
        "rates": spool.read_rates(),
        "bots": len(live.bots(settings.read())),
        "maps": world.named_map_load(),
        # The dashboard prints this beside the map, so it is lifted out of the
        # restart block rather than being dug out of it by the page.
        "last_restart_time": restart.get("last_restart_time") or spool.rate_status().get("time", ""),
    }


@bp.route("/status")
@login_required
def status():
    """Is the world up, and how much of it - for the badge in the sidebar."""
    return {"ok": True, **world.server_status()}


@bp.route("/items")
@login_required
def items():
    """Live item lookup for the give-item field on a character's page."""
    found = economy.search(request.args.get("q", ""))
    for item in found:
        item["icon"] = _item_icon_url(item["vnum"])
        item["type_label"] = itemdata.type_label(item.get("type"))
    return {"ok": True, "items": found}


@bp.route("/economy")
@login_required
def economy_live():
    """The cheap half of the economy tab: indexed lookups, refreshed often."""
    overview = economy.market_overview()
    trades = economy.recent_trades()
    for trade in trades:
        trade["icon"] = _item_icon_url(trade["vnum"])
        trade["item_url"] = url_for("economy.item", vnum=trade["vnum"])
        trade["time"] = trade["time"].strftime("%Y-%m-%d %H:%M:%S") if trade.get("time") else ""
        if trade.get("buyer_name"):
            trade["buyer_url"] = url_for("characters.profile", pid=trade["buyer_id"])
            trade["buyer_portrait"] = _portrait_url(trade.get("buyer_job"))
        if trade.get("seller_name"):
            trade["seller_url"] = url_for("characters.profile", pid=trade["seller_id"])
            trade["seller_portrait"] = _portrait_url(trade.get("seller_job"))
    return {"ok": True, "overview": overview, "trades": trades}


@bp.route("/economy/market")
@login_required
def economy_market():
    """The expensive half: a full-log aggregation, refreshed far less often."""
    top_items = economy.top_traded_items()
    shops = economy.active_shops()
    for item in top_items:
        item["icon"] = _item_icon_url(item["vnum"])
        item["item_url"] = url_for("economy.item", vnum=item["vnum"])
    for shop in shops:
        shop["shop_url"] = url_for("economy.shop", owner_id=shop["owner"])
    return {"ok": True, "top_items": top_items, "shops": shops}


@bp.route("/heat-events")
@login_required
def heat_events():
    kind = request.args.get("type", "deaths").strip().lower()
    if kind not in world.HEAT_TYPES:
        abort(400, "Nieznany rodzaj zdarzenia.")
    return {
        "ok": True,
        "type": kind,
        "label": world.HEAT_LABELS[kind],
        "events": world.heat_events(kind),
        "bounds": MAP_BOUNDS,
    }
