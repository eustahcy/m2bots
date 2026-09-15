"""Map activity over time, and the weekly season."""
from flask import Blueprint, render_template

from ..gamedata.maps import DRAWABLE_MAP_OPTIONS
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
