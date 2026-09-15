"""The world as a whole: totals, the news ticker, the season and the heat maps."""
import re
import time

import pymysql

from .. import config, db, engine, live
from ..gamedata import characters as chardata
from ..gamedata.maps import MAP_BOUNDS, TRACKED_MAP_OPTIONS, map_at, map_name
from ..text import game_text, hex_text

# Characters the world totals should not count as wealth.
EXCLUDED_FROM_YANG = ("[SA]Admin", "Test")


def totals():
    """The four headline numbers, in one round trip."""
    marks = ",".join(["%s"] * len(EXCLUDED_FROM_YANG))
    return db.one(
        f"""SELECT
              (SELECT COUNT(*) FROM player.player) AS characters,
              (SELECT COUNT(*) FROM account.account) AS accounts,
              (SELECT COUNT(*) FROM player.item) AS item_stacks,
              (SELECT COALESCE(SUM(gold),0) FROM player.player
                 WHERE name NOT IN ({marks})) AS yang""",
        EXCLUDED_FROM_YANG,
    )


def bot_count():
    return db.one(
        f"SELECT COUNT(*) AS count FROM player.player p WHERE {engine.BOT_IS}"
    ).get("count", 0)


# --- the news ticker --------------------------------------------------------
NEWS_WINDOW_HOURS = 12
NEWS_SCAN_LIMIT = 900
NEWS_KEEP = 30
# A refine below +7 is not news: a busy server produces thousands a minute.
NEWS_REFINE_TIERS = ("+7", "+8", "+9")
PEARL_MARKER = "małż"


def news_events():
    """Rare achievements from the game's own log, with stable ids.

    The filtering happens in SQL, before the limit. Taking the newest 900 rows
    first and sifting them in Python made rare achievements vanish from the
    feed altogether on any server with real traffic.
    """
    tier_clause = " OR ".join(f"l.hint LIKE '%%{tier}%%'" for tier in NEWS_REFINE_TIERS)
    raw = db.rows(
        f"""SELECT l.time, l.how, l.hint, HEX(l.hint) AS hint_hex, l.what, l.who, p.name,
              HEX(proto.locale_name) AS item_name_hex
            FROM log.log l
            JOIN player.player p ON p.id = l.who
            LEFT JOIN player.item i ON i.id = l.what
            LEFT JOIN player.item_proto proto ON proto.vnum = i.vnum
            WHERE l.time >= NOW() - INTERVAL {NEWS_WINDOW_HOURS} HOUR
              AND (
                (l.how = 'REFINE SUCCESS' AND ({tier_clause}))
                OR l.how = 'SKILLUP'
                OR (l.how = 'GET' AND LOWER(CONVERT(l.hint USING utf8mb4))
                    COLLATE utf8mb4_general_ci LIKE %s)
              )
            ORDER BY l.time DESC LIMIT {NEWS_SCAN_LIMIT}""",
        (f"%{PEARL_MARKER}%",),
    )
    events, seen = [], set()
    for row in raw:
        # `how` is VARBINARY on mt2009 and arrives as bytes; str() of that is
        # "b'GET'" and matches nothing below.
        how, name = game_text(row.get("how")), game_text(row.get("name"))
        # log.log's hint column is declared big5 while the engine writes CP1250
        # into it, so letting the driver decode gives mojibake for anything past
        # ASCII. HEX() returns the untouched bytes, which really are CP1250.
        hint = hex_text(row.get("hint_hex")) or game_text(row.get("hint"))
        key = f"{how}:{row.get('who')}:{row.get('what')}:{row.get('time')}"
        if key in seen or not name:
            continue
        message, tier = None, 0
        if how == "REFINE SUCCESS":
            match = re.search(r"\+([789])(?:\s|$)", hint)
            if match:
                tier = int(match.group(1))
                item = hex_text(row.get("item_name_hex")) or hint.strip()
                message = f"{name} ulepszył {item} na +{tier}"
        elif how == "SKILLUP":
            match = re.search(r"SkillUp:\s+\S+\s+(\d+)\s+(\d+)\s+(\d+)", hint)
            if match:
                vnum, master, level = map(int, match.groups())
                rank = chardata.skill_rank(master, level)
                # M1 happens to everyone; anything above it is worth saying.
                if (rank.startswith("M") and rank != "M1") or rank.startswith("G") or rank == "P":
                    skill = chardata.SKILL_NAMES.get(vnum, f"umiejętność #{vnum}")
                    message = f"{name} rozwinął {skill} na {rank}"
        elif how == "GET" and PEARL_MARKER in hint.casefold():
            message = f"{name} znalazł Małż podczas połowu"
        if not message:
            continue
        seen.add(key)
        stamp = row.get("time")
        events.append({
            "key": key,
            "time": stamp.strftime("%H:%M") if hasattr(stamp, "strftime") else str(stamp)[11:16],
            "message": message,
            "refine_tier": tier,
        })
    return list(reversed(events[-NEWS_KEEP:]))


# --- the season -------------------------------------------------------------
SEASON_CACHE_SECONDS = 600
SEASON_POINTS = {"metins": 150, "bosses": 500, "refine7": 200}
SEASON_SIZE = 30
_season_cache = {"at": 0.0, "weekly": [], "records": {}}


def _season_sums():
    """The three event counts, spelled once for the ranking and the records."""
    tiers = " OR ".join(f"l.hint LIKE '%%+{tier}'" for tier in (7, 8, 9))
    return (
        "SUM(l.how='STONE_KILL') AS metins,"
        " SUM(l.how='BOSS_KILL') AS bosses,"
        f" SUM(l.how='REFINE SUCCESS' AND ({tiers})) AS refine7"
    )


def season():
    """This week's ladder and the world's records, cached for ten minutes.

    Only the three indexed event types are counted: anything else means a scan
    of the busiest table in the world on every page load.
    """
    if time.time() - _season_cache["at"] < SEASON_CACHE_SECONDS:
        return _season_cache["weekly"], _season_cache["records"]
    score = " + ".join(
        f"COALESCE({column},0) * {points}" for column, points in SEASON_POINTS.items())
    weekly = db.rows(
        f"""SELECT p.id, p.name, p.level, {_season_sums()}
            FROM log.log l JOIN player.player p ON p.id = l.who
            WHERE l.time >= NOW() - INTERVAL 7 DAY AND {engine.BOT_IS}
              AND l.how IN ('STONE_KILL','BOSS_KILL','REFINE SUCCESS')
            GROUP BY p.id, p.name, p.level
            ORDER BY ({score}) DESC, p.level DESC LIMIT {SEASON_SIZE}"""
    )
    for row in weekly:
        row["points"] = sum(int(row.get(column) or 0) * points
                            for column, points in SEASON_POINTS.items())
    records = db.one(
        f"""SELECT {_season_sums()},
              (SELECT MAX(level) FROM player.player) AS level
            FROM log.log l
            WHERE l.time >= NOW() - INTERVAL 7 DAY
              AND l.how IN ('STONE_KILL','BOSS_KILL','REFINE SUCCESS')"""
    )
    _season_cache.update(at=time.time(), weekly=weekly, records=records)
    return weekly, records


# --- map activity -----------------------------------------------------------
MAP_HISTORY_HOURS = 24


def map_history():
    """A day of per-map population, shaped for a multi-series line chart."""
    raw = db.rows(
        f"""SELECT DATE_FORMAT(captured_at, '%%m-%%d %%H:%%i') AS label,
              map_index, character_count
            FROM {config.MAP_SNAPSHOT_TABLE}
            WHERE captured_at >= NOW() - INTERVAL {MAP_HISTORY_HOURS} HOUR
            ORDER BY captured_at ASC"""
    )
    labels, series = [], {index: {} for index, _name in TRACKED_MAP_OPTIONS}
    for row in raw:
        index = int(row["map_index"] or 0)
        if index not in series:
            continue
        if row["label"] not in labels:
            labels.append(row["label"])
        series[index][row["label"]] = int(row["character_count"] or 0)
    return {
        "labels": labels,
        "series": [
            {"id": index, "name": name, "data": [series[index].get(label, 0) for label in labels]}
            for index, name in TRACKED_MAP_OPTIONS
        ],
    }


def current_map_load():
    """Live population per tracked map, including the empty ones."""
    counts = {int(row["map_index"]): row["character_count"] for row in live.map_counts()}
    return [
        {"map_index": index, "name": name, "character_count": counts.get(index, 0)}
        for index, name in TRACKED_MAP_OPTIONS
    ]


# --- heat maps --------------------------------------------------------------
HEAT_TYPES = {"deaths": "DEAD_BY_NPC", "metins": "STONE_KILL", "bosses": "BOSS_KILL"}
HEAT_LABELS = {"deaths": "zgonów botów", "metins": "rozbitych Metinów", "bosses": "zabitych bossów"}
HEAT_WINDOW_HOURS = 24
HEAT_LIMIT = 4000


def heat_events(kind):
    """Logged events with real coordinates, bucketed into the map they fell on."""
    how = HEAT_TYPES.get(kind)
    if not how:
        return []
    raw = db.rows(
        """SELECT l.x, l.y, l.time, p.name
           FROM log.log l LEFT JOIN player.player p ON p.id = l.who
           WHERE l.type = 'CHARACTER' AND l.how = %s
             AND l.time >= NOW() - INTERVAL %s HOUR
           ORDER BY l.time DESC LIMIT %s""",
        (how, HEAT_WINDOW_HOURS, HEAT_LIMIT),
    )
    events = []
    for event in raw:
        index = map_at(event["x"], event["y"])
        if index is None:
            continue
        events.append({
            "map_index": index, "x": event["x"], "y": event["y"],
            "time": event["time"].isoformat() if hasattr(event["time"], "isoformat") else str(event["time"]),
            "name": game_text(event.get("name")),
        })
    return events


# --- host telemetry ---------------------------------------------------------
TELEMETRY_HOURS = 24


def telemetry():
    """A day of CPU, memory and disk, newest last."""
    try:
        return db.rows(
            f"""SELECT DATE_FORMAT(captured_at, '%%H:%%i') AS label,
                  cpu_percent, ram_percent, ram_used_mb, ram_total_mb,
                  disk_percent, disk_used_mb, disk_total_mb
                FROM {config.SYSTEM_SNAPSHOT_TABLE}
                WHERE captured_at >= NOW() - INTERVAL {TELEMETRY_HOURS} HOUR
                ORDER BY captured_at"""
        )
    except pymysql.MySQLError:
        return []


def telemetry_now():
    """The newest sample, or {} in the first minutes of a fresh installation
    before the collector has written one - which is not an error to raise."""
    try:
        return db.one(f"SELECT * FROM {config.SYSTEM_SNAPSHOT_TABLE} ORDER BY captured_at DESC LIMIT 1")
    except pymysql.MySQLError:
        return {}


def named_map_load():
    """Live map counts with their names attached, busiest first."""
    rows = live.map_counts()
    for row in rows:
        row["name"] = map_name(row["map_index"])
    return rows


# --- is the world running ----------------------------------------------------
_status_cache = {"at": 0.0, "value": None}
STATUS_CACHE_SECONDS = 10


def server_status():
    """Whether the game answers, and how much of it is up.

    Two ports, because they come up in order: the auth server accepts logins
    before the world is ready, and an operator watching a restart wants to see
    that rather than a flat "off".
    """
    from .. import spool

    now = time.time()
    if _status_cache["value"] and now - _status_cache["at"] < STATUS_CACHE_SECONDS:
        return dict(_status_cache["value"])
    auth = spool.port_open(config.GAME_LOGIN_PORT)
    world = spool.port_open(config.GAME_WORLD_PORT)
    if auth and world:
        state, label = "up", "Serwer działa"
    elif auth or world:
        state, label = "partial", "Serwer się podnosi"
    else:
        state, label = "down", "Serwer nie odpowiada"
    value = {"state": state, "label": label, "auth": auth, "world": world,
             "bots": len(live.statuses())}
    _status_cache.update(at=now, value=value)
    return dict(value)


__all__ = [
    "totals", "bot_count", "news_events", "season", "map_history", "current_map_load",
    "heat_events", "telemetry", "telemetry_now", "named_map_load",
    "HEAT_TYPES", "HEAT_LABELS", "MAP_BOUNDS",
]
