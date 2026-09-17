"""What exists in the world, the market it trades on, and the catalogue of
what could."""
from flask import Blueprint, abort, render_template, request

from ..queries import economy as queries
from ..security import login_required

bp = Blueprint("economy", __name__)


@bp.route("/economy")
@login_required
def index():
    query = request.args.get("q", "").strip()
    captured_at = queries.latest_capture()
    return render_template("economy/index.html",
                           captured_at=captured_at,
                           items=queries.stock(captured_at, query),
                           trend=queries.yang_trend(),
                           overview=queries.market_overview(),
                           trades=queries.recent_trades(),
                           top_items=queries.top_traded_items(),
                           shops=queries.active_shops(),
                           query=query)


@bp.route("/economy/shops")
@login_required
def shops():
    return render_template("economy/shops.html",
                           shops=queries.active_shops(queries.SHOPS_LIMIT),
                           overview=queries.market_overview(),
                           totals=queries.trade_totals(),
                           books=queries.skill_books())


@bp.route("/economy/shop/<int:owner_id>")
@login_required
def shop(owner_id):
    owner, items = queries.shop(owner_id)
    if not owner:
        abort(404)
    return render_template("economy/shop.html", owner=owner, items=items)


@bp.route("/economy/item/<int:vnum>")
@login_required
def item(vnum):
    return render_template("economy/item.html",
                           item=queries.item_name(vnum),
                           history=queries.item_history(vnum),
                           trades=queries.item_trades(vnum),
                           listings=queries.item_listings(vnum))


@bp.route("/items")
@login_required
def catalogue():
    query = request.args.get("q", "").strip()
    item_type = request.args.get("type", "").strip()
    items, total = queries.catalogue(query, item_type)
    return render_template("economy/catalogue.html",
                           items=items, total=total,
                           types=queries.catalogue_types(),
                           selected_type=item_type, query=query)
