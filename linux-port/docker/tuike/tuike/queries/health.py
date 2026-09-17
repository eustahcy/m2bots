"""Is the bot world actually working?

A Playerbots regression is quiet: nobody logs in and finds an error page, the
bots simply stop doing something. The pułap-M1 case took days to notice, and
only because someone read the database by hand. These are the cheap questions
whose answers say "something changed" - bots the cores stopped reporting, bots
standing still, bots with nothing in their hands, and how the world looked
before and after the last restart.

Every query here either rides an index or reads the collector's own snapshots.
"""
import pymysql

from .. import cache, config, db, engine, live, settings
from ..gamedata import bots as botdata
from ..gamedata.maps import TRACKED_MAP_OPTIONS, map_name
from ..text import game_text

STUCK_LIST = 25
DEATH_HOURS = 24
# Around the restart: how long a window on each side is worth comparing.
COMPARE_HOURS = 6
LEVEL_BANDS = ((1, 9), (10, 19), (20, 34), (35, 49), (50, 999))


@cache.ttl(60)
def gear_gaps():
    """Bots wearing no weapon or no body armour, and bots still at level 1.

    A bot that never armed itself is the first thing to break when the gear
    rules change, and it cannot be seen on the map at all.
    """
    missing = db.one(
        f"""SELECT
              SUM(NOT EXISTS (SELECT 1 FROM player.item i
                  WHERE i.owner_id = p.id AND i.window = 'EQUIPMENT' AND i.pos = 4)) AS no_weapon,
              SUM(NOT EXISTS (SELECT 1 FROM player.item i
                  WHERE i.owner_id = p.id AND i.window = 'EQUIPMENT' AND i.pos = 0)) AS no_armour,
              SUM(p.level <= 1) AS never_started,
              COUNT(*) AS total
            FROM player.player p WHERE {engine.BOT_IS}"""
    )
    return {key: int(missing.get(key) or 0)
            for key in ("no_weapon", "no_armour", "never_started", "total")}


@cache.ttl(60)
def registered_count():
    """How many bots the seed registry vouches for, whatever the cores report."""
    try:
        return int(db.scalar("SELECT COUNT(*) FROM common.playerbot_seed_state", default=0) or 0)
    except pymysql.MySQLError:
        # A world that has never run the playerbot migrations has no registry.
        return 0


@cache.ttl(120)
def deaths(hours=DEATH_HOURS):
    """Bot deaths in the last day, by cause - the how column is indexed."""
    row = db.one(
        """SELECT SUM(how = 'DEAD_BY_NPC') AS by_monster, SUM(how = 'DEAD_BY_PC') AS by_player
           FROM log.log
           WHERE how IN ('DEAD_BY_NPC', 'DEAD_BY_PC') AND time >= NOW() - INTERVAL %s HOUR""",
        (int(hours),),
    )
    return {key: int(value or 0) for key, value in row.items()}


def stuck_bots(limit=STUCK_LIST):
    """The bots the panel believes are wedged, with where they are standing."""
    ids = live._stuck_ids(settings.stuck_minutes())
    if not ids:
        return []
    current = live.statuses()
    directory = live._directory()
    rows = []
    for pid in ids:
        state = current.get(pid)
        known = directory.get(pid)
        if not state or not known:
            continue
        rows.append({
            "id": pid, "name": known[0], "level": known[1],
            "map_index": state["map_index"], "map_name": map_name(state["map_index"]),
            "x": state["x"], "y": state["y"],
            "status": game_text(state.get("status")) or botdata.label("action", state.get("action")),
        })
    rows.sort(key=lambda row: (row["map_name"], -row["level"]))
    return rows[:limit]


def population():
    """Where the bots are and how they are spread across levels.

    Read from the cores' own status files, so it says what the world looks
    like this second rather than what the database last saved.
    """
    current = live.statuses()
    directory = live._directory()
    maps = {index: {"name": name, "count": 0, "bands": [0] * len(LEVEL_BANDS), "levels": []}
            for index, name in TRACKED_MAP_OPTIONS}
    for pid, state in current.items():
        known = directory.get(pid)
        entry = maps.get(state["map_index"])
        if not known or not entry:
            continue
        level = known[1]
        entry["count"] += 1
        entry["levels"].append(level)
        for position, (low, high) in enumerate(LEVEL_BANDS):
            if low <= level <= high:
                entry["bands"][position] += 1
                break
    rows = []
    for entry in maps.values():
        if not entry["count"]:
            continue
        levels = entry["levels"]
        rows.append({
            "name": entry["name"], "count": entry["count"],
            "average": round(sum(levels) / len(levels), 1),
            "lowest": min(levels), "highest": max(levels),
            "bands": [round(number * 100 / entry["count"]) for number in entry["bands"]],
        })
    rows.sort(key=lambda row: -row["count"])
    empty = [entry["name"] for entry in maps.values() if not entry["count"]]
    return rows, empty


@cache.ttl(120)
def around_restart(started_at, hours=COMPARE_HOURS):
    """How busy the world was before the cores last started, and since.

    The collector writes one row per map every few minutes; averaging those
    either side of the restart is the closest thing to "did the update change
    anything" this panel can answer without instrumenting the game.
    """
    if not started_at:
        return {}
    try:
        row = db.one(
            f"""SELECT
                  AVG(CASE WHEN captured_at < FROM_UNIXTIME(%s) THEN total END) AS before,
                  AVG(CASE WHEN captured_at >= FROM_UNIXTIME(%s) THEN total END) AS after,
                  SUM(captured_at >= FROM_UNIXTIME(%s)) AS samples_after
                FROM (SELECT captured_at, SUM(character_count) AS total
                      FROM {config.MAP_SNAPSHOT_TABLE}
                      WHERE captured_at >= FROM_UNIXTIME(%s) - INTERVAL %s HOUR
                        AND captured_at <= FROM_UNIXTIME(%s) + INTERVAL %s HOUR
                      GROUP BY captured_at) AS windowed""",
            (started_at, started_at, started_at, started_at, int(hours), started_at, int(hours)),
        )
    except pymysql.MySQLError:
        return {}
    before = float(row.get("before") or 0)
    after = float(row.get("after") or 0)
    return {
        "before": round(before),
        "after": round(after),
        "samples_after": int(row.get("samples_after") or 0),
        "change": round(after - before),
        "percent": round((after - before) * 100 / before, 1) if before else None,
    }
