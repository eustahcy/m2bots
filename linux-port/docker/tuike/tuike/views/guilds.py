"""Guilds."""
from flask import Blueprint, abort, render_template, request

from ..queries import guilds as queries
from ..security import login_required

bp = Blueprint("guilds", __name__)


@bp.route("/guilds")
@login_required
def roster():
    query = request.args.get("q", "").strip()
    return render_template("guilds/roster.html", guilds=queries.roster(query), query=query)


@bp.route("/guild/<int:guild_id>")
@login_required
def detail(guild_id):
    guild = queries.detail(guild_id)
    if not guild:
        abort(404)
    return render_template("guilds/detail.html", guild=guild,
                           members=queries.members(guild_id, guild.get("leader_id")))
