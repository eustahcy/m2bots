"""The one part of Tuike a player may see.

Everything else in this panel is behind the operator's password. These pages
are the opposite: no login, no account data, no addresses, no telemetry - the
state of the world, the ladders and what the stalls are selling, which is what
a player asks Discord about anyway.

They are off until the operator turns them on in Zarządzanie, and every query
behind them is cached, because this is the only surface a stranger can make
the panel work.
"""
from flask import Blueprint, abort, render_template

from .. import cache, settings, spool
from ..queries import economy, rankings, world

bp = Blueprint("public", __name__, url_prefix="/serwer")

BOARDS = (("level", "Poziom"), ("playtime", "Czas gry"), ("bosses", "Bossy"),
          ("refine_rate", "Skuteczność ulepszeń"))
BOARD_SIZE = 10
SHOP_ITEMS = 40


def _enabled():
    return settings.read().get("public_page") == "1"


def _guard():
    """A page nobody has published answers as if it did not exist."""
    if not _enabled():
        abort(404)


@cache.ttl(30)
def _world():
    status = world.server_status()
    return {
        "status": status,
        "rates": spool.read_rates(),
        "started_at": status.get("started_at", 0),
    }


@cache.ttl(120)
def _boards():
    """The public ladders: a name and a number, nothing else about a player."""
    boards = []
    for kind, title in BOARDS:
        rows = rankings.decorate(rankings.ranking(kind)[:BOARD_SIZE], kind)
        boards.append({"kind": kind, "title": title, "rows": rows})
    return boards


@bp.route("/")
def index():
    _guard()
    current = settings.read()
    return render_template(
        "public/index.html",
        world=_world(),
        boards=_boards(),
        events=world.news_events()[-12:],
        brand=current.get("panel_name", "Metin2"),
    )


@bp.route("/stragany")
def shops():
    _guard()
    current = settings.read()
    return render_template(
        "public/shops.html",
        overview=economy.market_overview(),
        top_items=economy.top_traded_items(limit=SHOP_ITEMS),
        trades=economy.recent_trades(limit=SHOP_ITEMS),
        brand=current.get("panel_name", "Metin2"),
    )
