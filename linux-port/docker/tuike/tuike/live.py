"""The world as the running cores describe it, right now.

Every game core writes a playerbot_status.tsv into its channel directory and
rewrites it every second or so. That file - not the database - is the only
place a bot's current position, goal and action exist; the database only knows
where it was when it last saved. A file nobody has touched for a while is a
core that stopped, so it is ignored rather than shown as a frozen world.
"""
from pathlib import Path
from time import time

import pymysql

from . import cache, config, db, settings
from .gamedata import bots as botdata
from .gamedata.maps import MAP_BOUNDS
from .text import game_text

# pid, personality, ambition, role, in_party, goal, action, updated_ms,
# map_index, x, y, hp, max_hp, status - fourteen columns after the header row.
STATUS_COLUMNS = 14


def _status_files():
    pattern = config.STATUS_GLOB.lstrip("/")
    return Path("/").glob(pattern)


def statuses():
    """Every bot a live core is currently reporting, keyed by character id."""
    result = {}
    now = time()
    for path in _status_files():
        try:
            if now - path.stat().st_mtime > config.STATUS_MAX_AGE_SECONDS:
                continue
            lines = path.read_text(encoding="cp1250", errors="replace").splitlines()
        except OSError:
            continue
        for line in lines[1:]:
            values = line.split("\t", STATUS_COLUMNS - 1)
            if len(values) != STATUS_COLUMNS:
                continue
            try:
                result[int(values[0])] = {
                    "personality": int(values[1]), "ambition": int(values[2]),
                    "role": int(values[3]), "in_party": bool(int(values[4])),
                    "goal": int(values[5]), "action": int(values[6]),
                    "updated_ms": int(values[7]), "map_index": int(values[8]),
                    "x": int(values[9]), "y": int(values[10]),
                    "hp": int(values[11]), "max_hp": int(values[12]),
                    "status": values[13],
                }
            except ValueError:
                continue
    return result


def positions():
    """(map_index, x, y) per character - what the collector stores."""
    return {pid: (state["map_index"], state["x"], state["y"]) for pid, state in statuses().items()}


def map_counts():
    """How many bots are on each map, busiest first."""
    counts = {}
    for state in statuses().values():
        counts[state["map_index"]] = counts.get(state["map_index"], 0) + 1
    return [
        {"map_index": index, "character_count": count}
        for index, count in sorted(counts.items(), key=lambda item: -item[1])
    ]


def _earlier_positions(ids, minutes):
    """Where these bots were `minutes` ago, from the collector's snapshots."""
    if not ids:
        return {}
    marks = ",".join(["%s"] * len(ids))
    try:
        prior = db.rows(
            f"""SELECT s.pid, s.map_index, s.x, s.y
                FROM {config.POSITION_SNAPSHOT_TABLE} s
                JOIN (SELECT pid, MAX(captured_at) AS captured_at
                      FROM {config.POSITION_SNAPSHOT_TABLE}
                      WHERE captured_at <= NOW() - INTERVAL %s MINUTE
                      GROUP BY pid) old
                  ON old.pid = s.pid AND old.captured_at = s.captured_at
                WHERE s.pid IN ({marks})""",
            (minutes, *ids),
        )
    except pymysql.MySQLError:
        return {}
    return {row["pid"]: row for row in prior}


def _looks_stuck(before, now, threshold_squared):
    """Same map, barely moved, and not standing still on purpose."""
    if not before or before["map_index"] != now["map_index"]:
        return False
    moved = (before["x"] - now["x"]) ** 2 + (before["y"] - now["y"]) ** 2
    if moved >= threshold_squared:
        return False
    return not botdata.is_stationary(now.get("status"), now.get("action"))


def bots(current_settings=None):
    """Live bots enriched with their database row and readable labels.

    Only bots on a map the panel can place are returned - a character in a
    dungeon instance the panel has no bounds for cannot be drawn anywhere
    meaningful, and showing it at 0,0 is worse than leaving it out.
    """
    live = statuses()
    if not live:
        return []
    ids = list(live)
    marks = ",".join(["%s"] * len(ids))
    roster = db.rows(
        f"""SELECT p.id, p.name, p.level, p.job, p.horse_level
            FROM player.player p
            LEFT JOIN account.account a ON a.id = p.account_id
            WHERE p.id IN ({marks})
              AND (LEFT(a.login, 10) = 'playerbot_' OR p.name LIKE 'bot%%')""",
        ids,
    )
    minutes = settings.stuck_minutes(current_settings)
    earlier = _earlier_positions(ids, minutes)
    result = []
    for bot in roster:
        state = live.get(bot["id"])
        if not state or state["map_index"] not in MAP_BOUNDS:
            continue
        result.append({
            **bot, **state,
            "personality_label": botdata.label("personality", state["personality"]),
            "ambition_label": botdata.label("ambition", state["ambition"]),
            "goal_label": botdata.label("goal", state["goal"]),
            "action_label": botdata.label("action", state["action"]),
            "stuck": _looks_stuck(earlier.get(bot["id"]), state, botdata.STUCK_DISTANCE_SQUARED),
            # The free-text status is diagnostic and can be stale; the action
            # number is the authoritative core state.
            "fighting_metin": (state.get("goal") == botdata.METIN_GOAL
                               and state.get("action") == botdata.FIGHT_ACTION),
        })
    return result


def shopkeeper_ids():
    """Characters the cores currently report as running a market stall."""
    return [pid for pid, state in statuses().items()
            if state.get("action") == botdata.SHOPKEEPER_ACTION]


def summary(roster):
    """The world numbers the dashboard puts above the map."""
    count = len(roster)
    levels = [int(bot.get("level") or 0) for bot in roster]
    horses = [int(bot.get("horse_level") or 0) for bot in roster]
    return {
        "bots": count,
        "average_level": round(sum(levels) / count, 1) if count else 0,
        "max_level": max(levels, default=0),
        "party_bots": sum(1 for bot in roster if bot.get("in_party")),
        "stuck_bots": sum(1 for bot in roster if bot.get("stuck")),
        "horse_average": round(sum(horses) / count, 1) if count else 0,
        "horse_max": max(horses, default=0),
    }


# --- the map's own feed -------------------------------------------------------
# The dashboard map used to ask for every bot in the world, with its name, goal
# and labels, every one and a half seconds - two thirds of a megabyte per tick
# per open tab, redrawn from scratch. What a moving map actually needs each
# tick is a position; a name and a level change far more slowly than that.
DIRECTORY_SECONDS = 30
STUCK_SECONDS = 15
# Flags, so a row is five numbers and a name rather than a dozen keys.
FLAG_PARTY, FLAG_STUCK, FLAG_METIN = 1, 2, 4


@cache.ttl(DIRECTORY_SECONDS)
def _directory():
    """Who the Playerbots are: id -> (name, level). Rarely changes."""
    rows = db.rows(
        "SELECT p.id, p.name, p.level FROM player.player p"
        " LEFT JOIN account.account a ON a.id = p.account_id"
        " WHERE LEFT(a.login, 10) = 'playerbot_' OR p.name LIKE 'bot%%'"
    )
    return {row["id"]: (game_text(row["name"]), int(row["level"] or 0)) for row in rows}


@cache.ttl(STUCK_SECONDS)
def _stuck_ids(minutes):
    """Bots that have not moved in `minutes` and are not idling on purpose.

    Its own query against the collector's snapshots, so the map's fast path
    does not run it on every tick.
    """
    current = statuses()
    earlier = _earlier_positions(list(current), minutes)
    return frozenset(
        pid for pid, state in current.items()
        if _looks_stuck(earlier.get(pid), state, botdata.STUCK_DISTANCE_SQUARED)
    )


def map_snapshot(map_index, current_settings=None):
    """Everything the live map needs for one map, plus the world's totals.

    Positions arrive as percentages of the map picture: the browser gets a
    number it can use directly, and the bounds table stays on the server.
    """
    current = statuses()
    directory = _directory()
    stuck = _stuck_ids(settings.stuck_minutes(current_settings))
    bounds = MAP_BOUNDS.get(int(map_index or 0))
    rows, counts = [], {}
    levels, party, world = [], 0, 0
    for pid, state in current.items():
        known = directory.get(pid)
        if not known or state["map_index"] not in MAP_BOUNDS:
            continue
        name, level = known
        world += 1
        levels.append(level)
        if state["in_party"]:
            party += 1
        counts[state["map_index"]] = counts.get(state["map_index"], 0) + 1
        if not bounds or state["map_index"] != int(map_index):
            continue
        flags = (FLAG_PARTY if state["in_party"] else 0) | (FLAG_STUCK if pid in stuck else 0)
        if state.get("goal") == botdata.METIN_GOAL and state.get("action") == botdata.FIGHT_ACTION:
            flags |= FLAG_METIN
        rows.append([
            pid,
            round(max(0.0, min(100.0, (state["x"] - bounds[0]) * 100 / bounds[2])), 2),
            round(max(0.0, min(100.0, (state["y"] - bounds[1]) * 100 / bounds[3])), 2),
            level, flags, name,
        ])
    return {
        "map": int(map_index or 0),
        "bots": rows,
        "summary": {
            "bots": world,
            "average_level": round(sum(levels) / len(levels), 1) if levels else 0,
            "max_level": max(levels, default=0),
            "party_bots": party,
            "stuck_bots": len(stuck),
            "maps": [{"map_index": index, "count": count}
                     for index, count in sorted(counts.items(), key=lambda pair: -pair[1])],
        },
    }
