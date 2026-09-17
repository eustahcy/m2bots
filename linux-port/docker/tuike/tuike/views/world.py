"""Map activity over time, the weekly season, and whether the bots are well."""
from datetime import datetime

from flask import Blueprint, render_template

from .. import live
from ..gamedata.maps import DRAWABLE_MAP_OPTIONS
from ..queries import health
from ..queries import world as queries
from ..security import login_required

bp = Blueprint("world", __name__)


@bp.route("/maps")
@login_required
def maps():
    return render_template("world/maps.html",
                           chart=queries.map_history(),
                           current=queries.current_map_load(),
                           heat_maps=DRAWABLE_MAP_OPTIONS,
                           heat_types=queries.HEAT_TYPES,
                           heat_labels=queries.HEAT_LABELS)


@bp.route("/season")
@login_required
def season():
    weekly, records = queries.season()
    return render_template("world/season.html", weekly=weekly, records=records,
                           points=queries.SEASON_POINTS)


@bp.route("/zdrowie")
@login_required
def health_check():
    """Whether the bot world is working, and what changed at the last restart."""
    started_at = queries.core_started_at()
    rows, empty = health.population()
    return render_template(
        "world/health.html",
        gaps=health.gear_gaps(),
        registered=health.registered_count(),
        live_bots=len(live.statuses()),
        deaths=health.deaths(),
        stuck=health.stuck_bots(),
        population=rows,
        empty_maps=empty,
        bands=health.LEVEL_BANDS,
        restart=health.around_restart(started_at),
        started_at=started_at,
        started_label=(datetime.fromtimestamp(started_at).strftime("rdzenie wstały %d.%m.%Y, %H:%M")
                       if started_at else "nie ustalono startu rdzeni"),
    )
