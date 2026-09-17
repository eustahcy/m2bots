"""Global bot rankings."""
from flask import Blueprint, render_template, request

from ..queries import rankings as queries
from ..security import login_required

bp = Blueprint("rankings", __name__)


@bp.route("/rankings")
@login_required
def index():
    kind = request.args.get("type", "level")
    if kind not in queries.KINDS:
        # An old bookmark for a ranking that no longer exists lands on the
        # level ladder rather than on a 404.
        kind = "level"
    sort_by = request.args.get("sort", "avg") if kind == "weapon30" else "avg"
    if sort_by not in queries.WEAPON30_SORTS:
        sort_by = "avg"
    ranking = queries.decorate(queries.ranking(kind, sort_by), kind)
    return render_template("rankings.html", kinds=queries.KINDS, kind=kind,
                           ranking=ranking, sorts=queries.WEAPON30_SORTS, sort_by=sort_by,
                           includes_players=queries.includes_players())
