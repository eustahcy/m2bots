"""Guilds and their members."""
from .. import db

LIST_LIMIT = 250

# Guild columns and the leader's, shared by the list and the detail page so the
# two never drift. The member count needs a GROUP BY over all of them.
GUILD_COLUMNS = """
    g.id, g.name, g.level, g.exp, g.sp, g.win, g.draw, g.loss, g.ladder_point, g.gold,
    leader.id AS leader_id, leader.name AS leader_name, leader.level AS leader_level
"""
GUILD_GROUPING = """
    g.id, g.name, g.level, g.exp, g.sp, g.win, g.draw, g.loss, g.ladder_point, g.gold,
    leader.id, leader.name, leader.level
"""
GUILD_FROM = """
    FROM player.guild g
    LEFT JOIN player.player leader ON leader.id = g.master
    LEFT JOIN player.guild_member gm ON gm.guild_id = g.id
"""


def roster(query=""):
    where, params = "", []
    if query:
        where = "WHERE g.name LIKE %s OR leader.name LIKE %s"
        params = [f"%{query}%", f"%{query}%"]
    return db.rows(
        f"""SELECT {GUILD_COLUMNS}, COUNT(gm.pid) AS member_count
            {GUILD_FROM} {where}
            GROUP BY {GUILD_GROUPING}
            ORDER BY g.level DESC, member_count DESC, g.name ASC LIMIT {LIST_LIMIT}""",
        params,
    )


def detail(guild_id):
    return db.one(
        f"""SELECT {GUILD_COLUMNS}, COUNT(gm.pid) AS member_count
            {GUILD_FROM} WHERE g.id = %s
            GROUP BY {GUILD_GROUPING}""",
        (guild_id,),
    )


def members(guild_id, leader_id):
    """The roll, leader first, then by rank and level."""
    return db.rows(
        """SELECT gm.pid, gm.grade, gm.is_general, gm.offer,
             p.name, p.level, p.job, p.map_index, p.playtime
           FROM player.guild_member gm
           LEFT JOIN player.player p ON p.id = gm.pid
           WHERE gm.guild_id = %s
           ORDER BY (gm.pid = %s) DESC, gm.grade ASC, p.level DESC, p.name ASC""",
        (guild_id, leader_id or 0),
    )


def bot_guild_count(bot_predicate):
    """How many guilds a bot leads - one number for the dashboard."""
    try:
        return db.one(
            "SELECT COUNT(*) AS count FROM player.guild g"
            f" JOIN player.player p ON p.id = g.master WHERE {bot_predicate}"
        ).get("count", 0)
    except Exception:
        return 0
