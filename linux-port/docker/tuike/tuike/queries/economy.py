"""How much of everything exists, and the catalogue of what can exist.

The economy pages read the collector's snapshots rather than counting live:
summing player.item across a populated world is a table scan nobody wants on a
page load, and the interesting question is the trend anyway.

Trades and shops are different: log.ikarusshop_log and the live
ikashop_offlineshop/item rows are already the full record (an append-only log,
a table of what is open right now), so there is nothing for a collector to
snapshot - these read the source directly, bounded by LIMIT and a time window.
"""
import json
from datetime import datetime, timedelta

from .. import cache, config, db
from ..gamedata import items as itemdata
from ..gamedata.characters import SKILL_NAMES
from ..text import game_text

ITEM_LIST_LIMIT = 500
TREND_DAYS = 7
ITEM_HISTORY_DAYS = 14

TRADE_LIMIT = 40
TOP_ITEMS_LIMIT = 12
TOP_ITEMS_DAYS = 7
SHOPS_PREVIEW_LIMIT = 8
SHOPS_LIMIT = 300
SHOP_ITEMS_LIMIT = 200
ITEM_TRADE_LIMIT = 20
ITEM_LISTING_LIMIT = 30
# A plain Skill Book is one vnum whatever it teaches; see queries.characters.
SKILL_BOOK_VNUMS = (50300,)


def latest_capture():
    return db.one(f"SELECT MAX(captured_at) AS captured_at FROM {config.ITEM_SNAPSHOT_TABLE}").get("captured_at")


def stock(captured_at, query=""):
    """Everything in the world at one capture, most plentiful first."""
    if not captured_at:
        return []
    items = db.rows(
        f"""SELECT s.vnum, s.amount,
              COALESCE(p.locale_name, CONCAT('VNUM ', s.vnum)) AS item_name
            FROM {config.ITEM_SNAPSHOT_TABLE} s
            LEFT JOIN player.item_proto p ON p.vnum = s.vnum
            WHERE s.captured_at = %s ORDER BY s.amount DESC""",
        (captured_at,),
    )
    for item in items:
        item["item_name"] = game_text(item["item_name"])
    if query:
        needle = query.lower()
        items = [item for item in items
                 if needle in item["item_name"].lower() or needle == str(item["vnum"])]
    return items[:ITEM_LIST_LIMIT]


def yang_trend():
    """Yang in circulation over the last week, as the chart wants it."""
    return db.rows(
        f"""SELECT DATE_FORMAT(captured_at, '%%m-%%d %%H:%%i') AS captured_at, value
            FROM {config.METRIC_SNAPSHOT_TABLE}
            WHERE metric = 'total_yang' AND captured_at >= NOW() - INTERVAL {TREND_DAYS} DAY
            ORDER BY captured_at"""
    )


def item_history(vnum):
    """One item's count over the last fortnight."""
    return db.rows(
        f"""SELECT DATE_FORMAT(captured_at, '%%m-%%d %%H:%%i') AS captured_at, amount
            FROM {config.ITEM_SNAPSHOT_TABLE}
            WHERE vnum = %s AND captured_at >= NOW() - INTERVAL {ITEM_HISTORY_DAYS} DAY
            ORDER BY captured_at""",
        (vnum,),
    )


def item_name(vnum):
    item = db.one(
        "SELECT vnum, COALESCE(locale_name, CONCAT('VNUM ', vnum)) AS item_name"
        " FROM player.item_proto WHERE vnum = %s",
        (vnum,),
    ) or {"vnum": vnum, "item_name": f"VNUM {vnum}"}
    item["item_name"] = game_text(item["item_name"])
    return item


# --- the Ikarus Shop market: who is selling what, and who bought what -------
# The native offline-shop system (ikashop) is the only stall system live on
# this build (see playerbot_market.h): a shop is a row in
# player.ikashop_offlineshop, its stock is player.item rows with
# window='IKASHOP_OFFLINESHOP', and every completed sale is logged to
# log.ikarusshop_log with what='BUY_ITEM' - buyer (who), seller (shop_owner),
# vnum, count and the yang actually paid. That log is the only place a real
# buyer<->seller<->price record exists; nothing here is inferred.


def _shop_price(raw):
    """A stall's per-item price, from item.ikashop_data.

    The game writes this column as a small JSON object - {"yang": N, "cheque":
    N, "expiration": ts} - via CreateShopCacheUpdateItemQuery. An item with no
    price set yet (row just inserted, price not confirmed) leaves it blank.
    """
    if not raw:
        return None
    try:
        return json.loads(raw).get("yang")
    except (TypeError, ValueError, AttributeError):
        return None


def market_overview():
    """Headline numbers for the top of the economy tab."""
    shops = db.scalar("SELECT COUNT(*) FROM player.ikashop_offlineshop", default=0)
    listed = db.scalar(
        "SELECT COUNT(*) FROM player.item WHERE window = 'IKASHOP_OFFLINESHOP'", default=0)
    trades = db.one(
        "SELECT COUNT(*) AS count, COALESCE(SUM(yang),0) AS yang FROM log.ikarusshop_log"
        " WHERE what = 'BUY_ITEM' AND time >= NOW() - INTERVAL 1 DAY"
    )
    return {
        "shops": shops,
        "listed": listed,
        "trades_24h": trades.get("count", 0),
        "turnover_24h": trades.get("yang", 0),
    }


def recent_trades(limit=TRADE_LIMIT):
    """The freshest completed sales, newest first.

    Ordered by id rather than time: id is the log's AUTO_INCREMENT primary
    key, so it is already in insertion (chronological) order and, unlike
    time, it is indexed - no filesort over an unbounded log table.
    """
    trades = db.rows(
        f"""SELECT l.id, l.time, l.vnum, l.count, l.yang,
              buyer.id AS buyer_id, buyer.name AS buyer_name, buyer.job AS buyer_job,
              seller.id AS seller_id, seller.name AS seller_name, seller.job AS seller_job,
              COALESCE(p.locale_name, CONCAT('VNUM ', l.vnum)) AS item_name
            FROM log.ikarusshop_log l
            LEFT JOIN player.player buyer ON buyer.id = l.who
            LEFT JOIN player.player seller ON seller.id = l.shop_owner
            LEFT JOIN player.item_proto p ON p.vnum = l.vnum
            WHERE l.what = 'BUY_ITEM'
            ORDER BY l.id DESC LIMIT {limit}"""
    )
    for trade in trades:
        trade["item_name"] = game_text(trade["item_name"])
    return trades


def top_traded_items(days=TOP_ITEMS_DAYS, limit=TOP_ITEMS_LIMIT):
    """What actually moves on the stalls, by yang turnover, over one window."""
    items = db.rows(
        f"""SELECT l.vnum, COUNT(*) AS sales, SUM(l.count) AS units, SUM(l.yang) AS turnover,
              ROUND(SUM(l.yang) / GREATEST(SUM(l.count),1)) AS avg_price,
              COALESCE(p.locale_name, CONCAT('VNUM ', l.vnum)) AS item_name
            FROM log.ikarusshop_log l
            LEFT JOIN player.item_proto p ON p.vnum = l.vnum
            WHERE l.what = 'BUY_ITEM' AND l.time >= NOW() - INTERVAL {int(days)} DAY
            GROUP BY l.vnum ORDER BY turnover DESC LIMIT {limit}"""
    )
    for item in items:
        item["item_name"] = game_text(item["item_name"])
    return items


def active_shops(limit=SHOPS_PREVIEW_LIMIT):
    """Currently open stalls, most stocked first."""
    shops = db.rows(
        f"""SELECT s.owner, s.name AS shop_name, s.map, s.is_premium,
              p.name AS owner_name, p.job AS owner_job,
              COUNT(i.id) AS item_count
            FROM player.ikashop_offlineshop s
            JOIN player.player p ON p.id = s.owner
            LEFT JOIN player.item i ON i.owner_id = s.owner AND i.window = 'IKASHOP_OFFLINESHOP'
            GROUP BY s.owner ORDER BY item_count DESC LIMIT {limit}"""
    )
    return shops


def shop(owner_id):
    """One stall's stock, priced. (owner, items) - owner is {} if it closed."""
    owner = db.one(
        "SELECT s.owner, s.name AS shop_name, s.map, s.is_premium,"
        " p.name AS owner_name, p.job AS owner_job"
        " FROM player.ikashop_offlineshop s JOIN player.player p ON p.id = s.owner"
        " WHERE s.owner = %s", (owner_id,))
    if not owner:
        return {}, []
    items = db.rows(
        f"""SELECT i.id, i.vnum, i.count, i.ikashop_data,
              COALESCE(p.locale_name, CONCAT('VNUM ', i.vnum)) AS item_name
            FROM player.item i LEFT JOIN player.item_proto p ON p.vnum = i.vnum
            WHERE i.owner_id = %s AND i.window = 'IKASHOP_OFFLINESHOP'
            ORDER BY i.pos LIMIT {SHOP_ITEMS_LIMIT}""",
        (owner_id,),
    )
    for item in items:
        item["item_name"] = game_text(item["item_name"])
        item["price"] = _shop_price(item.pop("ikashop_data", None))
    return owner, items


def item_trades(vnum, limit=ITEM_TRADE_LIMIT):
    """One item's own recent sale history, for its detail page."""
    trades = db.rows(
        f"""SELECT l.id, l.time, l.count, l.yang,
              buyer.id AS buyer_id, buyer.name AS buyer_name,
              seller.id AS seller_id, seller.name AS seller_name
            FROM log.ikarusshop_log l
            LEFT JOIN player.player buyer ON buyer.id = l.who
            LEFT JOIN player.player seller ON seller.id = l.shop_owner
            WHERE l.what = 'BUY_ITEM' AND l.vnum = %s
            ORDER BY l.id DESC LIMIT {limit}""",
        (vnum,),
    )
    return trades


def item_listings(vnum, limit=ITEM_LISTING_LIMIT):
    """Who is selling this item right now, and for how much."""
    listings = db.rows(
        f"""SELECT i.id, i.count, i.ikashop_data, i.owner_id, p.name AS owner_name
            FROM player.item i JOIN player.player p ON p.id = i.owner_id
            WHERE i.vnum = %s AND i.window = 'IKASHOP_OFFLINESHOP'
            ORDER BY i.id LIMIT {limit}""",
        (vnum,),
    )
    for listing in listings:
        listing["price"] = _shop_price(listing.pop("ikashop_data", None))
    return listings


# --- the item catalogue -----------------------------------------------------
def catalogue(query="", item_type=""):
    """Every prototype the server knows, filtered. No paging: the page filters
    the rendered list in the browser, which is instant and survives a typo."""
    where, params = [], []
    if query:
        where.append("(p.vnum = %s OR p.locale_name LIKE %s OR p.name LIKE %s)")
        params += [int(query) if query.isdigit() else -1, f"%{query}%", f"%{query}%"]
    if str(item_type).isdigit():
        where.append("p.type = %s")
        params.append(int(item_type))
    predicate = (" WHERE " + " AND ".join(where)) if where else ""
    total = db.one(f"SELECT COUNT(*) AS count FROM player.item_proto p{predicate}", params).get("count", 0)
    records = db.rows(
        "SELECT p.vnum, p.name, p.locale_name, p.type, p.subtype, p.size, p.gold, p.shop_buy_price"
        f" FROM player.item_proto p{predicate} ORDER BY p.vnum",
        params,
    )
    for item in records:
        item["name"] = game_text(item.get("locale_name") or item.get("name"))
    return records, total


def catalogue_types():
    """The type tabs, each with a count and an icon borrowed from a member."""
    types = db.rows(
        "SELECT type, COUNT(*) AS count, MIN(vnum) AS icon_vnum"
        " FROM player.item_proto GROUP BY type ORDER BY type"
    )
    for category in types:
        category["label"] = itemdata.type_label(category["type"])
    return types


SEARCH_LIMIT = 25


def search(query):
    """Live item lookup for the give-item field: a few matches, named and iconed."""
    query = (query or "").strip()
    if len(query) < 2 and not query.isdigit():
        return []
    rows = db.rows(
        "SELECT p.vnum, p.name, p.locale_name, p.type, p.size FROM player.item_proto p"
        " WHERE p.vnum = %s OR p.locale_name LIKE %s OR p.name LIKE %s"
        # An exact vnum first, then names that start with what was typed.
        " ORDER BY (p.vnum = %s) DESC, (p.locale_name LIKE %s) DESC, p.vnum"
        f" LIMIT {SEARCH_LIMIT}",
        (int(query) if query.isdigit() else -1, f"%{query}%", f"%{query}%",
         int(query) if query.isdigit() else -1, f"{query}%"),
    )
    for item in rows:
        item["name"] = game_text(item.get("locale_name") or item.get("name"))
        item.pop("locale_name", None)
    return rows


def prototype(vnum):
    """One prototype, for the grant tool's "is this item giveable" check."""
    return db.one(
        "SELECT vnum, locale_name, type, size FROM player.item_proto WHERE vnum = %s",
        (vnum,),
    )


# --- the stalls page: totals, tempo, skill books ------------------------------
# log.ikarusshop_log is indexed on its id and not on time. Every windowed query
# below first narrows to the newest rows by id - a primary-key range - and only
# then filters by time, so a day's window does not read the whole history.
RECENT_TRADE_ROWS = 200000
PULSE_HOURS = 24
BOOK_SALES_DAYS = 7


def _recent_rows():
    return (f"l.id > (SELECT GREATEST(COALESCE(MAX(id), 0) - {RECENT_TRADE_ROWS}, 0)"
            " FROM log.ikarusshop_log)")


@cache.ttl(120)
def trade_totals():
    """Every stall sale ever logged, and the last day's share of them."""
    row = db.one(
        f"""SELECT COUNT(*) AS total,
              (SELECT COUNT(*) FROM log.ikarusshop_log l
                 WHERE {_recent_rows()} AND l.what = 'BUY_ITEM'
                   AND l.time >= NOW() - INTERVAL 1 DAY) AS last_day
            FROM log.ikarusshop_log WHERE what = 'BUY_ITEM'"""
    )
    return {"total": int(row.get("total") or 0), "last_day": int(row.get("last_day") or 0)}


@cache.ttl(240)
def sales_pulse():
    """Sales per hour over a day, with the average price per unit sold.

    The average is over everything sold that hour, so it moves with the mix
    of goods as much as with prices - it says whether the market is buying
    cheap or dear, not what one item costs.
    """
    rows = db.rows(
        f"""SELECT DATE_FORMAT(l.time, '%%Y-%%m-%%d %%H') AS hour, COUNT(*) AS sales,
              ROUND(SUM(l.yang) / GREATEST(SUM(l.count), 1)) AS avg_price
            FROM log.ikarusshop_log l
            WHERE {_recent_rows()} AND l.what = 'BUY_ITEM'
              AND l.time >= NOW() - INTERVAL {PULSE_HOURS} HOUR
            GROUP BY hour ORDER BY hour"""
    )
    by_hour = {str(row["hour"]): row for row in rows}
    now = datetime.now().replace(minute=0, second=0, microsecond=0)
    slots = [now - timedelta(hours=offset) for offset in range(PULSE_HOURS - 1, -1, -1)]
    pulse = {"labels": [], "sales": [], "avg_price": []}
    for slot in slots:
        row = by_hour.get(slot.strftime("%Y-%m-%d %H"), {})
        pulse["labels"].append(slot.strftime("%H:00"))
        pulse["sales"].append(int(row.get("sales") or 0))
        pulse["avg_price"].append(int(row.get("avg_price") or 0) or None)
    return pulse


@cache.ttl(300)
def skill_books():
    """Which skills are on sale as books right now, and which have sold.

    A plain Skill Book is always vnum 50300; the skill it teaches is socket0.
    Offers still carry their item row. A sold book keeps its row only until
    the buyer reads it, so the sold column counts the books still readable -
    a floor, and labelled as one on the page.
    """
    listed = db.rows(
        f"""SELECT i.socket0 AS skill, COUNT(*) AS offers, SUM(i.count) AS units, i.ikashop_data
            FROM player.item i
            WHERE i.window = 'IKASHOP_OFFLINESHOP' AND i.vnum IN ({",".join(str(v) for v in SKILL_BOOK_VNUMS)})
            GROUP BY i.socket0, i.ikashop_data"""
    )
    sold = db.rows(
        f"""SELECT i.socket0 AS skill, COUNT(*) AS sales, ROUND(AVG(l.yang / GREATEST(l.count, 1))) AS avg_price
            FROM log.ikarusshop_log l JOIN player.item i ON i.id = l.itemid
            WHERE {_recent_rows()} AND l.what = 'BUY_ITEM'
              AND l.vnum IN ({",".join(str(v) for v in SKILL_BOOK_VNUMS)})
              AND l.time >= NOW() - INTERVAL {BOOK_SALES_DAYS} DAY
            GROUP BY i.socket0"""
    )
    books = {}

    def entry(skill):
        skill = int(skill or 0)
        return books.setdefault(skill, {
            "skill": skill, "name": SKILL_NAMES.get(skill, f"Umiejętność #{skill}"),
            "offers": 0, "cheapest": None, "sales": 0, "avg_price": None,
        })

    for row in listed:
        book = entry(row.get("skill"))
        book["offers"] += int(row.get("units") or row.get("offers") or 0)
        price = _shop_price(row.get("ikashop_data"))
        if price and (book["cheapest"] is None or int(price) < book["cheapest"]):
            book["cheapest"] = int(price)
    for row in sold:
        book = entry(row.get("skill"))
        book["sales"] = int(row.get("sales") or 0)
        book["avg_price"] = int(row.get("avg_price") or 0) or None
    return sorted((book for book in books.values() if book["skill"]),
                  key=lambda book: (book["sales"], book["offers"]), reverse=True)
