"""The world as a whole: totals, the news ticker, the season and the heat maps."""
import re
import time
from datetime import datetime, timedelta
from pathlib import Path

import pymysql

from .. import cache, config, db, engine, live
from ..gamedata import characters as chardata
from ..gamedata.maps import MAP_BOUNDS, TRACKED_MAP_OPTIONS, map_at, map_name
from ..text import game_text, hex_text

# Characters the world totals should not count as wealth: the installer's test
# accounts alone carry two billion Yang nobody earned.
EXCLUDED_FROM_YANG = engine.SEEDED_CHARACTERS


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
    # Rows arrive newest first. Keep the newest NEWS_KEEP and hand them back in
    # the order they happened; slicing from the end kept the oldest instead.
    return list(reversed(events[:NEWS_KEEP]))


# --- the season -------------------------------------------------------------
SEASON_CACHE_SECONDS = 600
SEASON_POINTS = {"metins": 150, "bosses": 500, "refine7": 200}
SEASON_SIZE = 30
_season_cache = {"at": 0.0, "weekly": [], "records": {}}


def _season_counts():
    """The three counted events, as SQL, keyed by the name each one gets."""
    tiers = " OR ".join(f"l.hint LIKE '%%+{tier}'" for tier in (7, 8, 9))
    return {
        "metins": "SUM(l.how='STONE_KILL')",
        "bosses": "SUM(l.how='BOSS_KILL')",
        "refine7": f"SUM(l.how='REFINE SUCCESS' AND ({tiers}))",
    }


def _season_sums():
    """The three event counts, spelled once for the ranking and the records."""
    return ", ".join(f"{sql} AS {name}" for name, sql in _season_counts().items())


def season():
    """This week's ladder and the world's records, cached for ten minutes.

    Only the three indexed event types are counted: anything else means a scan
    of the busiest table in the world on every page load.
    """
    if time.time() - _season_cache["at"] < SEASON_CACHE_SECONDS:
        return _season_cache["weekly"], _season_cache["records"]
    # The sums are repeated in ORDER BY rather than referred to by their
    # aliases: MariaDB refuses an aggregate alias inside an expression there
    # ("Reference 'metins' not supported"), which took the whole page down.
    counts = _season_counts()
    score = " + ".join(
        f"COALESCE({counts[column]},0) * {points}" for column, points in SEASON_POINTS.items())
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
             "bots": len(live.statuses()), "started_at": core_started_at()}
    _status_cache.update(at=now, value=value)
    return dict(value)


def core_started_at():
    """When the running game cores started, as a Unix time, or 0.

    Each core writes its pid file once, at start, and never again - so the
    newest one is when the world last came up, whoever restarted it. The rate
    spool only knows about restarts this panel queued itself.
    """
    stamps = []
    for path in Path("/").glob(config.CORE_PID_GLOB.lstrip("/")):
        try:
            stamps.append(path.stat().st_mtime)
        except OSError:
            continue
    return int(max(stamps)) if stamps else 0


# --- the dashboard ------------------------------------------------------------
ACTIVITY_HOURS = 24
RECENT_LOGINS = 6


def _hour_labels(hours=ACTIVITY_HOURS):
    """The last `hours` whole hours, oldest first, as 'HH:00' keyed by the hour."""
    now = datetime.now().replace(minute=0, second=0, microsecond=0)
    return [now - timedelta(hours=offset) for offset in range(hours - 1, -1, -1)]


def _per_hour(rows, slots, value="value"):
    found = {str(row.get("hour")): float(row.get(value) or 0) for row in rows}
    return [round(found.get(slot.strftime("%Y-%m-%d %H"), 0), 1) for slot in slots]


@cache.ttl(240)
def activity():
    """A day of logins, bots in the world and stall sales, hour by hour.

    Every source here is cheap on purpose: loginlog is indexed on time, the
    bot count comes from the collector's own map snapshots, and the trade log
    is bounded by a one-day window. log.log is not asked - it has no time
    index, and a per-hour count of drops over it would scan the whole table.
    """
    slots = _hour_labels()
    hour = "DATE_FORMAT({column}, '%%Y-%%m-%%d %%H')"
    logins, bots, sales = [], [], []
    try:
        logins = db.rows(
            f"""SELECT {hour.format(column='time')} AS hour, COUNT(*) AS value
                FROM log.loginlog
                WHERE type = 'LOGIN' AND time >= NOW() - INTERVAL {ACTIVITY_HOURS} HOUR
                GROUP BY hour""")
    except pymysql.MySQLError:
        pass
    try:
        # One snapshot is a sum over maps; an hour holds up to twelve of them,
        # and their average is how many bots that hour really had.
        bots = db.rows(
            f"""SELECT hour, AVG(total) AS value FROM (
                  SELECT {hour.format(column='captured_at')} AS hour, captured_at,
                         SUM(character_count) AS total
                  FROM {config.MAP_SNAPSHOT_TABLE}
                  WHERE captured_at >= NOW() - INTERVAL {ACTIVITY_HOURS} HOUR
                  GROUP BY captured_at) AS snapshots
                GROUP BY hour""")
    except pymysql.MySQLError:
        pass
    try:
        sales = db.rows(
            f"""SELECT {hour.format(column='time')} AS hour, COUNT(*) AS value
                FROM log.ikarusshop_log
                WHERE what = 'BUY_ITEM' AND time >= NOW() - INTERVAL {ACTIVITY_HOURS} HOUR
                GROUP BY hour""")
    except pymysql.MySQLError:
        pass
    return {
        "labels": [slot.strftime("%H:00") for slot in slots],
        "logins": _per_hour(logins, slots),
        "bots": _per_hour(bots, slots),
        "sales": _per_hour(sales, slots),
    }


def recent_logins(limit=RECENT_LOGINS):
    """The newest logins, bots and players alike, marked which is which."""
    try:
        rows = db.rows(
            f"""SELECT l.time, p.id, p.name, p.level, p.job,
                  {engine.BOT_IS} AS is_bot
                FROM log.loginlog l JOIN player.player p ON p.id = l.pid
                WHERE l.type = 'LOGIN'
                ORDER BY l.time DESC LIMIT {int(limit)}""")
    except pymysql.MySQLError:
        return []
    for row in rows:
        row["name"] = game_text(row.get("name"))
        row["is_bot"] = bool(row.get("is_bot"))
    return rows


def yang_change():
    """Yang in circulation now against the collector's reading a day ago.

    Returns (series, percent). The series is the last day in collector
    samples, for the sparkline; the percent is None until a day of history
    exists, rather than a made-up zero.
    """
    try:
        series = db.rows(
            f"""SELECT value FROM {config.METRIC_SNAPSHOT_TABLE}
                WHERE metric = 'total_yang' AND captured_at >= NOW() - INTERVAL 1 DAY
                ORDER BY captured_at""")
    except pymysql.MySQLError:
        return [], None
    values = [int(row.get("value") or 0) for row in series]
    if len(values) < 2 or not values[0]:
        return values, None
    return values, round((values[-1] - values[0]) * 100 / values[0], 1)


def bot_presence():
    """Bots in the world across the last day, one reading per collector pass."""
    try:
        rows = db.rows(
            f"""SELECT SUM(character_count) AS total FROM {config.MAP_SNAPSHOT_TABLE}
                WHERE captured_at >= NOW() - INTERVAL 1 DAY
                GROUP BY captured_at ORDER BY captured_at""")
    except pymysql.MySQLError:
        return []
    return [int(row.get("total") or 0) for row in rows]


def shops_by_map():
    """Open stalls per map, busiest first - they only stand in the villages."""
    try:
        rows = db.rows(
            "SELECT map AS map_index, COUNT(*) AS shop_count"
            " FROM player.ikashop_offlineshop GROUP BY map ORDER BY shop_count DESC")
    except pymysql.MySQLError:
        return []
    for row in rows:
        row["name"] = map_name(row["map_index"])
    return rows


__all__ = [
    "totals", "bot_count", "news_events", "season", "map_history", "current_map_load",
    "heat_events", "telemetry", "telemetry_now", "named_map_load", "server_status",
    "core_started_at", "activity", "recent_logins", "yang_change", "bot_presence", "shops_by_map",
    "HEAT_TYPES", "HEAT_LABELS", "MAP_BOUNDS",
]
