"""Host telemetry: CPU, memory and disk over the last day."""
from flask import Blueprint, render_template

from ..queries import world as queries
from ..security import login_required

bp = Blueprint("system", __name__)


@bp.route("/system")
@login_required
def index():
    samples = queries.telemetry()
    return render_template("system.html", samples=samples,
                           current=samples[-1] if samples else {})
