"""The front page: what the world looks like right now."""
from datetime import datetime

from flask import Blueprint, render_template

from .. import engine, live, settings, spool, updater
from ..gamedata.maps import DRAWABLE_MAP_OPTIONS
from ..queries import guilds as guild_queries
from ..queries import rankings, world
from ..security import login_required

bp = Blueprint("dashboard", __name__)

LEVEL_FILTERS = (("all", "Wszystkie"), ("1-9", "1–9"), ("10-19", "10–19"),
                 ("20-34", "20–34"), ("35-49", "35–49"), ("50+", "50+"))


def _last_restart_label():
    """When the server last came back, in the operator's own time zone."""
    stamp = spool.rate_status().get("time")
    try:
        return datetime.fromtimestamp(int(stamp)).strftime("%d.%m.%Y, %H:%M:%S")
    except (TypeError, ValueError, OSError):
        return "Brak danych"


@bp.route("/")
@login_required
def index():
    current = settings.read()
    roster = live.bots(current)
    summary = live.summary(roster)
    summary.update({
        "guilds": guild_queries.bot_guild_count(engine.BOT_IS),
        "last_restart": _last_restart_label(),
        "rates": spool.read_rates(),
        "release": updater.release_status(),
    })
    top = rankings.top_by_level()
    return render_template(
        "dashboard.html",
        totals=world.totals(),
        bot_count=world.bot_count(),
        telemetry=world.telemetry_now(),
        map_load=world.named_map_load(),
        top=top,
        leader_id=top[0]["id"] if top else None,
        quick_rankings=rankings.quick_rankings(top),
        summary=summary,
        live_maps=DRAWABLE_MAP_OPTIONS,
        level_filters=LEVEL_FILTERS,
        heat_types=world.HEAT_TYPES,
    )
