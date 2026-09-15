"""Durable mass item grants.

Tuike never writes into player.item. Items are created by the game itself: the
panel records who should receive what, a worker hands a few names at a time to
the queue the in-game web_admin quest drains, and the quest's answer comes back
through the same queue. That way an item is always created by the engine, with
its own inventory and stacking rules, and a grant survives a panel restart.
"""
import json
import time

import pymysql

from . import config, db, settings
from .gamedata.characters import RIDING_SKILL_OFFSET

# 200, because that is a full stack and all the engine can hand over at once.
# pc.give_item2 reads the count as an int and passes it to
# CHARACTER::AutoGiveItem(DWORD, BYTE, ...) - one byte: 256 becomes zero, 300
# becomes 44, 65535 becomes 255. Nothing reported it, because a non-zero item
# id looked like success, so a batch finished as "granted" with fewer items
# than were asked for.
MAX_ITEM_COUNT = 200
# How many names may sit in the game's queue at once. The quest answers one
# recipient at a time.
MAX_PENDING = 10

JOBS = (("", "Każda klasa"), ("0", "Wojownik"), ("1", "Ninja"), ("2", "Sura"), ("3", "Szaman"))

# What the quest can answer, in the operator's language.
TERMINAL = {
    "done": "Nadano",
    "has_item": "Już posiada",
    "full": "Brak miejsca w ekwipunku",
    "failed": "Gra nie mogła utworzyć przedmiotu",
    "bad_args": "Nieprawidłowy VNUM lub ilość",
    "no_skill": "Warunek nie jest już spełniony",
    "gone": "Postać nie istnieje",
    "qty_too_big": "Ilość ponad 200 — gra nie wyda tego za jednym razem",
    "partial": "Wydano mniej, niż proszono (sprawdź plecak)",
    "cancelled": "Anulowano",
    "review": "Wymaga sprawdzenia",
    "unknown_cmd": "Quest wymaga aktualizacji",
}
LABELS = {"waiting": "Czeka na wysłanie", "queued": "Przekazano aktywnej postaci", **TERMINAL}

# The worker runs in its own container and the page could not tell whether it
# was alive: a batch that stopped partway looked exactly like a dead worker, a
# queue nobody in game was reading, and a cohort of bots that were simply
# offline. The worker stamps every tick; the page reads the stamp, the last
# error, and the age of the oldest task in each state, so it can say which.
HEARTBEAT_KEY = "grants_heartbeat"
LAST_ERROR_KEY = "grants_last_error"
HEARTBEAT_STALE = 15   # three ticks of three seconds, with room to spare
QUEUE_STALE = 45       # a queue row older than this has nobody in game reading it

# A grant for an offline character sits in the queue for up to a minute before
# it is withdrawn, comes back two minutes later, and while it waits it holds
# one of MAX_PENDING's places. Ten offline names at the head of the queue
# therefore block every online one behind them, and a batch appears to stop.
# The collector's snapshot says who is actually in the world: online recipients
# take the fast lane and offline probes get only a few places, so the queue
# always moves.
SNAPSHOT_MAX_AGE = 15 * 60
OFFLINE_PROBES_PER_TICK = 2
OFFLINE_MAX_PENDING = 4
# How long the quest may hold a row before the worker stops waiting on it.
QUEUE_ABANDON_SECONDS = 60
QUEUE_REVIEW_SECONDS = 120
OFFLINE_RETRY_MINUTES = 2
HISTORY_LIMIT = 1000
PREVIEW_TTL_SECONDS = 900

CRITERIA_FIELDS = (
    ("min_level", "Minimalny poziom", 120, "p.level >= %s", 1, "Lv ≥ {}"),
    ("max_level", "Maksymalny poziom", 120, "p.level <= %s", 1, "Lv ≤ {}"),
    ("min_horse", "Minimalny poziom konia", 30, "p.horse_level >= %s", 1, "Koń ≥ {}"),
    # playtime is stored in minutes and asked for in hours.
    ("min_playtime", "Minimalny czas gry", 100000, "p.playtime >= %s", 60, "Czas ≥ {} h"),
    ("min_riding", "Minimalne jeździectwo", 30,
     f"ORD(SUBSTRING(p.skill_level,{RIDING_SKILL_OFFSET + 1},1)) >= %s", 1, "Jeździectwo ≥ {}"),
)


class GrantError(ValueError):
    """Something the operator can fix by changing the form."""


# --- reading the form -------------------------------------------------------
def number(raw, label, maximum, allow_empty=True):
    raw = (raw or "").strip()
    if allow_empty and not raw:
        return None
    try:
        value = int(raw)
    except ValueError:
        raise GrantError(f"{label}: wpisz liczbę całkowitą.")
    if not 0 <= value <= maximum:
        raise GrantError(f"{label}: dozwolony zakres to 0–{maximum}.")
    return value


def criteria_from(values):
    """The recipient conditions, with everything left blank simply absent."""
    job = values.get("job", "")
    if job not in dict(JOBS):
        raise GrantError("Nieprawidłowa klasa postaci.")
    criteria = {name: number(values.get(name), label, maximum)
                for name, label, maximum, _sql, _scale, _text in CRITERIA_FIELDS}
    criteria["job"] = int(job) if job else None
    if (criteria["min_level"] is not None and criteria["max_level"] is not None
            and criteria["min_level"] > criteria["max_level"]):
        raise GrantError("Minimalny poziom nie może być wyższy od maksymalnego.")
    return {key: value for key, value in criteria.items() if value is not None}


def criteria_text(criteria):
    labels = [text.format(criteria[name])
              for name, _label, _max, _sql, _scale, text in CRITERIA_FIELDS if name in criteria]
    if "job" in criteria:
        labels.append(dict(JOBS)[str(criteria["job"])])
    return " · ".join(labels) or "Bez warunków"


def stored_criteria(grant):
    """A grant's saved conditions, or None when the row is unreadable."""
    try:
        value = json.loads(grant["criteria"] or "{}")
        return value if isinstance(value, dict) else None
    except (TypeError, ValueError):
        return None


# --- finding recipients -----------------------------------------------------
def _has_item_sql(alias="p"):
    """True when this character already owns the item, anywhere it can be."""
    return (
        "EXISTS(SELECT 1 FROM player.item i WHERE i.vnum = %s AND i.count > 0 AND"
        f" ((i.owner_id = {alias}.id AND i.window IN ('INVENTORY','EQUIPMENT'))"
        f"  OR (i.owner_id = {alias}.account_id AND i.window IN ('SAFEBOX','MALL'))))"
    )


def candidates(cursor, vnum, criteria, only_missing, player_id=None):
    """Who currently matches. Re-checked at send time, not only at preview."""
    conditions, params = [], []
    for name, _label, _max, sql, scale, _text in CRITERIA_FIELDS:
        if name in criteria:
            conditions.append(sql)
            params.append(criteria[name] * scale)
    if "job" in criteria:
        conditions.append("MOD(p.job,4) = %s")
        params.append(criteria["job"])
    # The tool is explicitly for Playerbots. A level or horse filter on its own
    # would also match an administrator or an ordinary player's character.
    conditions.append("(LEFT(a.login,10) = 'playerbot_' OR p.name LIKE 'bot%%')")
    if only_missing:
        conditions.append("NOT " + _has_item_sql())
        params.append(vnum)
    if player_id is not None:
        conditions.append("p.id = %s")
        params.append(player_id)
    cursor.execute(
        "SELECT p.id, p.name, p.level, p.horse_level, p.playtime, MOD(p.job,4) AS job,"
        f" ORD(SUBSTRING(p.skill_level,{RIDING_SKILL_OFFSET + 1},1)) AS riding"
        " FROM player.player p LEFT JOIN account.account a ON a.id = p.account_id"
        " WHERE " + " AND ".join(conditions) + " ORDER BY p.id",
        params,
    )
    return cursor.fetchall()


# --- what the page reports --------------------------------------------------
def worker_stats(cursor):
    """Is the worker alive, what is waiting, and for how long. Ages in seconds."""
    stats = {"alive": False, "heartbeat_age": None, "last_error": "", "counts": {},
             "oldest_waiting": None, "oldest_queued": None, "pending": 0,
             "oldest_pending": None, "queue_stale": False}
    cursor.execute(
        f"SELECT name, value FROM {config.SETTINGS_TABLE} WHERE name IN (%s, %s)",
        (HEARTBEAT_KEY, LAST_ERROR_KEY),
    )
    for row in cursor.fetchall():
        if row["name"] == HEARTBEAT_KEY:
            try:
                age = max(0, int(time.time()) - int(row["value"]))
            except (TypeError, ValueError):
                age = None
            stats["heartbeat_age"] = age
            stats["alive"] = age is not None and age <= HEARTBEAT_STALE
        else:
            stats["last_error"] = row["value"] or ""
    cursor.execute(
        f"""SELECT status, COUNT(*) AS n, MAX(TIMESTAMPDIFF(SECOND,updated,NOW())) AS oldest
            FROM {config.GRANTS_TABLE} GROUP BY status"""
    )
    for row in cursor.fetchall():
        stats["counts"][row["status"]] = int(row["n"])
        if row["status"] in ("waiting", "queued"):
            stats[f"oldest_{row['status']}"] = int(row["oldest"] or 0)
    # The in-game side: rows the quest has not taken yet. One older than
    # QUEUE_STALE means nothing in game is reading the queue at all - nobody
    # has logged in since the server started, or the quest did not compile.
    cursor.execute(
        "SELECT COUNT(*) AS n, MAX(TIMESTAMPDIFF(SECOND,created,NOW())) AS oldest"
        f" FROM {config.GAME_QUEUE_TABLE} WHERE status = 'pending'"
    )
    row = cursor.fetchone() or {}
    stats["pending"] = int(row.get("n") or 0)
    stats["oldest_pending"] = int(row["oldest"]) if row.get("oldest") is not None else None
    stats["queue_stale"] = stats["oldest_pending"] is not None and stats["oldest_pending"] > QUEUE_STALE
    return stats


def history(cursor, limit=HISTORY_LIMIT):
    cursor.execute(f"SELECT * FROM {config.GRANTS_TABLE} ORDER BY id DESC LIMIT {limit}")
    rows = cursor.fetchall()
    for grant in rows:
        grant["criteria_text"] = criteria_text(stored_criteria(grant) or {})
        grant["status_label"] = LABELS.get(grant["status"], grant["status"])
    return rows


# --- the worker -------------------------------------------------------------
def _online_ids(cursor):
    """Bots the collector last saw in the world, or None if it has not looked
    recently - then nobody is preferred and the plain order applies."""
    try:
        cursor.execute(
            "SELECT MAX(captured_at) AS at, TIMESTAMPDIFF(SECOND,MAX(captured_at),NOW()) AS age"
            f" FROM {config.POSITION_SNAPSHOT_TABLE}"
        )
        row = cursor.fetchone() or {}
        if row.get("at") is None or int(row.get("age") or 0) > SNAPSHOT_MAX_AGE:
            return None
        cursor.execute(
            f"SELECT pid FROM {config.POSITION_SNAPSHOT_TABLE} WHERE captured_at = %s", (row["at"],))
        return {int(entry["pid"]) for entry in cursor.fetchall()}
    except pymysql.MySQLError:
        return None


def _pick_waiting(cursor, capacity, pending=0):
    """The waiting rows this tick may hand to the game, online first."""
    if capacity <= 0:
        return []
    online = _online_ids(cursor)
    if online is None:
        cursor.execute(
            f"SELECT * FROM {config.GRANTS_TABLE} WHERE status = 'waiting' AND next_try <= NOW()"
            " ORDER BY next_try, id LIMIT %s FOR UPDATE", (capacity,))
        return list(cursor.fetchall())
    # A human's character is never in the bot snapshot, so it counts as "maybe
    # online" and takes the fast lane: the quest answers for it within seconds
    # when it is in game, and the sweep retires it when it is not.
    bots_sql = ("SELECT p.id FROM player.player p JOIN account.account a ON a.id = p.account_id"
                " WHERE a.login LIKE 'playerbot_%%'")
    marks = ",".join(["%s"] * len(online)) if online else "NULL"
    cursor.execute(
        f"SELECT * FROM {config.GRANTS_TABLE} WHERE status = 'waiting' AND next_try <= NOW()"
        f" AND (player_id IN ({marks}) OR player_id NOT IN ({bots_sql}))"
        " ORDER BY next_try, id LIMIT %s FOR UPDATE",
        tuple(sorted(online)) + (capacity,),
    )
    picked = list(cursor.fetchall())
    # The cap is on how many probes may be out at once, not on how fast they
    # are issued, so the fast lane always keeps room.
    left = min(capacity - len(picked), OFFLINE_PROBES_PER_TICK, OFFLINE_MAX_PENDING - pending)
    if left > 0:
        taken = [grant["id"] for grant in picked]
        exclusion = f"AND id NOT IN ({','.join(['%s'] * len(taken))}) " if taken else ""
        cursor.execute(
            f"SELECT * FROM {config.GRANTS_TABLE} WHERE status = 'waiting' AND next_try <= NOW() "
            + exclusion + "ORDER BY next_try, id LIMIT %s FOR UPDATE",
            tuple(taken) + (left,),
        )
        picked.extend(cursor.fetchall())
    return picked


def _settle_queued(cursor):
    """Turn the quest's answers into final states, and retry offline names."""
    cursor.execute(
        f"""SELECT g.*, q.status AS queue_status, TIMESTAMPDIFF(SECOND,q.created,NOW()) AS age
            FROM {config.GRANTS_TABLE} g
            LEFT JOIN {config.GAME_QUEUE_TABLE} q ON q.id = g.queue_id
            WHERE g.status = 'queued' FOR UPDATE"""
    )
    for grant in cursor.fetchall():
        status, age = grant["queue_status"], grant["age"] or 0
        if status == "pending" and age > QUEUE_ABANDON_SECONDS:
            cursor.execute(
                f"UPDATE {config.GAME_QUEUE_TABLE} SET status='cancelled'"
                " WHERE id=%s AND status='pending'", (grant["queue_id"],))
            if cursor.rowcount:
                status = "player_offline"
        if status == "pending":
            continue
        # The quest marks a row it is working on with a w-prefixed state.
        if status and status.startswith("w"):
            if age < QUEUE_REVIEW_SECONDS:
                continue
            status = "review"
        if status == "player_offline":
            cursor.execute(
                f"UPDATE {config.GRANTS_TABLE} SET status='waiting', queue_id=NULL,"
                f" next_try=NOW()+INTERVAL {OFFLINE_RETRY_MINUTES} MINUTE, updated=NOW()"
                " WHERE id=%s", (grant["id"],))
        else:
            cursor.execute(
                f"UPDATE {config.GRANTS_TABLE} SET status=%s, updated=NOW() WHERE id=%s",
                (status if status in TERMINAL else "review", grant["id"]))


def _dispatch_waiting(cursor):
    """Hand as many waiting grants to the game as the queue has room for."""
    cursor.execute(f"SELECT COUNT(*) AS n FROM {config.GAME_QUEUE_TABLE} WHERE status='pending'")
    pending = int(cursor.fetchone()["n"])
    for grant in _pick_waiting(cursor, max(0, MAX_PENDING - pending), pending):
        criteria = stored_criteria(grant)
        matched = candidates(cursor, grant["vnum"], criteria, bool(grant["only_missing"]),
                             grant["player_id"]) if criteria is not None else []
        if not matched:
            cursor.execute("SELECT id FROM player.player WHERE id=%s", (grant["player_id"],))
            status = "gone" if not cursor.fetchone() else (
                "has_item" if grant["only_missing"] else "no_skill")
            cursor.execute(f"UPDATE {config.GRANTS_TABLE} SET status=%s, updated=NOW() WHERE id=%s",
                           (status, grant["id"]))
            continue
        command = "BULK_MISSING" if grant["only_missing"] else "BULK_ITEM"
        cursor.execute(
            f"INSERT INTO {config.GAME_QUEUE_TABLE} (player_name,cmd,arg1,arg2)"
            " VALUES (%s,%s,%s,%s)",
            (matched[0]["name"], command, str(grant["vnum"]), str(grant["quantity"])),
        )
        cursor.execute(
            f"UPDATE {config.GRANTS_TABLE} SET status='queued', queue_id=%s, updated=NOW()"
            " WHERE id=%s", (cursor.lastrowid, grant["id"]))


def tick(connection):
    """One pass: settle what the game answered, then send what fits."""
    with connection.cursor() as cursor:
        cursor.execute("SELECT GET_LOCK(%s, 0) AS acquired", (config.GRANTS_LOCK,))
        if cursor.fetchone()["acquired"] != 1:
            return
        try:
            connection.begin()
            _settle_queued(cursor)
            _dispatch_waiting(cursor)
            connection.commit()
        except Exception:
            connection.rollback()
            raise
        finally:
            cursor.execute("SELECT RELEASE_LOCK(%s)", (config.GRANTS_LOCK,))


def beat(error=""):
    """One stamp per tick, whatever the tick did.

    It uses its own connection so that a tick which failed halfway - and rolled
    back - still leaves a fresh stamp. A dead worker and a failing one are
    different problems and the page has to be able to say which.
    """
    settings.stamp(HEARTBEAT_KEY, int(time.time()))
    settings.stamp(LAST_ERROR_KEY, error)


# --- recording a batch ------------------------------------------------------
def record(cursor, batch, recipients, vnum, quantity, criteria, only_missing):
    """Write one row per recipient. The caller holds the lock and the
    transaction, and has just re-read the recipients inside it."""
    criteria_json = json.dumps(criteria, separators=(",", ":"))
    cursor.executemany(
        f"""INSERT INTO {config.GRANTS_TABLE}
            (batch, player_id, player_name, vnum, quantity, criteria, only_missing)
            VALUES (%s,%s,%s,%s,%s,%s,%s)""",
        [(batch, row["id"], row["name"], vnum, quantity, criteria_json, only_missing)
         for row in recipients],
    )


def cancel_batch(batch):
    """Withdraw everything in a batch that has not been sent to the game yet."""
    return db.execute(
        f"UPDATE {config.GRANTS_TABLE} SET status='cancelled', updated=NOW()"
        " WHERE status='waiting' AND batch=%s",
        (batch,),
    )
