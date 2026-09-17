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
TOP_PLAYERS = 5

# Lines for the banner, a different one each day. Written for this panel, not quoted from
# the game - short, in the register of a Metin2 loading screen.
BANNER_QUOTES = (
    "Każdy Metin zaczyna się od pierwszego uderzenia.",
    "Świat nie śpi - boty właśnie ulepszają kolejny miecz.",
    "Najlepszy ekwipunek to ten, który przetrwał +9.",
    "W Pustyni Yongbi nikt nie pyta o drogę dwa razy.",
    "Królestwa rosną w siłę, gdy handel kwitnie na straganach.",
)


def _last_restart_label():
    """When the server last came back, in the operator's own time zone."""
    stamp = world.core_started_at() or spool.rate_status().get("time")
    try:
        return datetime.fromtimestamp(int(stamp)).strftime("%d.%m.%Y, %H:%M:%S")
    except (TypeError, ValueError, OSError):
        return "Brak danych"


def _sparkline(values, points=16):
    """Bucket a series into a few bars, each as a share of the tallest."""
    values = [float(value or 0) for value in values]
    if len(values) < 2:
        return []
    size = max(1, len(values) // points)
    buckets = [sum(values[i:i + size]) / len(values[i:i + size])
               for i in range(0, len(values), size)][-points:]
    low, high = min(buckets), max(buckets)
    span = (high - low) or 1
    # Never flat on the floor: a quiet stretch still reads as a bar.
    return [round(18 + (value - low) * 82 / span) for value in buckets]


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
    yang_series, yang_delta = world.yang_change()
    presence = world.bot_presence()
    return render_template(
        "dashboard.html",
        totals=world.totals(),
        bot_count=world.bot_count(),
        telemetry=world.telemetry_now(),
        map_load=world.named_map_load(),
        top=top,
        top_players=top[:TOP_PLAYERS],
        leader_id=top[0]["id"] if top else None,
        quick_rankings=rankings.quick_rankings(top),
        includes_players=rankings.includes_players(),
        summary=summary,
        status=world.server_status(),
        recent_logins=world.recent_logins(),
        kpi={
            "yang_bars": _sparkline(yang_series),
            "yang_delta": yang_delta,
            "presence_bars": _sparkline(presence),
            "presence_peak": max(presence, default=0),
        },
        banner_quote=BANNER_QUOTES[datetime.now().toordinal() % len(BANNER_QUOTES)],
        live_maps=DRAWABLE_MAP_OPTIONS,
        level_filters=LEVEL_FILTERS,
        heat_types=world.HEAT_TYPES,
    )
