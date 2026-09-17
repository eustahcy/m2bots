"""Characters: the roster, one profile, and everything hanging off it."""
import json
import re
from pathlib import Path

from .. import config, db, engine, live
from ..gamedata import bots as botdata
from ..gamedata import characters as chardata
from ..gamedata import items as itemdata
from ..text import game_text, hex_text, thousands

ROSTER_LIMIT = 250
LOG_LIMIT = 60
ITEM_LIMIT = 250
SAFEBOX_LIMIT = 180

# Every column a tooltip needs, for both the item and its prototype.
ITEM_COLUMNS = """
    i.id, i.vnum, i.count, i.window, i.pos,
    i.socket0, i.socket1, i.socket2,
    i.attrtype0, i.attrvalue0, i.attrtype1, i.attrvalue1, i.attrtype2, i.attrvalue2,
    i.attrtype3, i.attrvalue3, i.attrtype4, i.attrvalue4, i.attrtype5, i.attrvalue5,
    i.attrtype6, i.attrvalue6,
    p.applytype0, p.applyvalue0, p.applytype1, p.applyvalue1, p.applytype2, p.applyvalue2,
    p.size AS item_size,
    COALESCE(p.locale_name, CONCAT('VNUM ', i.vnum)) AS item_name
"""


def roster(query=""):
    """The searchable character list, live positions folded in."""
    sql = ("SELECT id, name, level, job, map_index, gold, playtime, last_play "
           "FROM player.player")
    params = []
    if query:
        sql += " WHERE name LIKE %s OR id = %s"
        params = [f"%{query}%", int(query) if query.isdigit() else -1]
    sql += f" ORDER BY level DESC, exp DESC LIMIT {ROSTER_LIMIT}"
    characters = db.rows(sql, params)
    current = live.statuses()
    for character in characters:
        state = current.get(character["id"])
        character["map_live"] = bool(state)
        if state:
            character["map_index"] = state["map_index"]
    return characters


def profile(pid):
    """One character's database row, or {} when there is no such id."""
    return db.one(
        "SELECT p.id, p.account_id, p.name, p.level, p.job, p.exp, p.gold, p.hp, p.mp,"
        " p.x, p.y, p.horse_level, p.alignment, p.st, p.ht, p.dx, p.iq,"
        " p.stat_point, p.skill_point, p.skill_group, p.skill_level, p.map_index, p.playtime,"
        " p.last_play, a.cash,"
        f" {engine.EMPIRE_EXPR} AS empire"
        " FROM player.player p"
        " LEFT JOIN account.account a ON a.id = p.account_id"
        " LEFT JOIN player.player_index pi ON pi.id = p.account_id"
        " WHERE p.id = %s",
        (pid,),
    )


# Every ordinary Skill Book is vnum 50300, whichever skill it teaches; the skill
# lives in socket0. Read as a soul stone, that socket pulled some unrelated
# item's name and bonuses into the book's tooltip.
SKILL_BOOK_VNUMS = frozenset({50300})


def display_name(vnum, name, socket0=0):
    """An item's name, with a Skill Book's actual skill spelled out."""
    name = game_text(name) or f"VNUM {vnum}"
    if int(vnum or 0) in SKILL_BOOK_VNUMS:
        skill = chardata.SKILL_NAMES.get(int(socket0 or 0))
        if skill:
            return f"{name}: {skill}"
    return name


def _decorate_item(item):
    item["item_name"] = display_name(item["vnum"], item["item_name"], item.get("socket0"))
    item["item_size"] = max(1, min(3, int(item.get("item_size") or 1)))
    item["base_stats"] = itemdata.base_stats(item["vnum"])
    # An item's own three prototype bonuses first, then the seven rolled ones.
    item["bonuses"] = [
        itemdata.apply_text(item.get(f"applytype{i}"), item.get(f"applyvalue{i}"), engine.IS_MT2009)
        for i in range(3)
        if item.get(f"applytype{i}") and item.get(f"applyvalue{i}")
    ] + [
        itemdata.apply_text(item.get(f"attrtype{i}"), item.get(f"attrvalue{i}"), engine.IS_MT2009)
        for i in range(7)
        if item.get(f"attrtype{i}") and item.get(f"attrvalue{i}")
    ]
    return item


def _socket_definitions(items):
    """Name and describe every soul stone socketed into any of these items."""
    vnums = sorted({
        int(item.get(f"socket{i}") or 0)
        for item in items for i in range(3)
        if int(item.get(f"socket{i}") or 0) > 0 and int(item["vnum"] or 0) not in SKILL_BOOK_VNUMS
    })
    if not vnums:
        return {}
    marks = ",".join(["%s"] * len(vnums))
    stones = db.rows(
        "SELECT vnum, COALESCE(locale_name, CONCAT('VNUM ', vnum)) AS item_name,"
        " applytype0, applyvalue0, applytype1, applyvalue1, applytype2, applyvalue2"
        f" FROM player.item_proto WHERE vnum IN ({marks})",
        vnums,
    )
    return {
        int(stone["vnum"]): {
            "name": game_text(stone["item_name"]),
            "bonuses": [
                itemdata.apply_text(stone.get(f"applytype{i}"), stone.get(f"applyvalue{i}"), engine.IS_MT2009)
                for i in range(3)
                if stone.get(f"applytype{i}") and stone.get(f"applyvalue{i}")
            ],
        }
        for stone in stones
    }


def belongings(pid, account_id):
    """Equipment, inventory and the account's safebox, ready to draw."""
    carried = db.rows(
        f"SELECT {ITEM_COLUMNS} FROM player.item i"
        " LEFT JOIN player.item_proto p ON p.vnum = i.vnum"
        f" WHERE i.owner_id = %s ORDER BY i.window, i.pos LIMIT {ITEM_LIMIT}",
        (pid,),
    )
    # The safebox belongs to the account, not the character, so it is keyed by
    # account_id - a different owner_id in the same table.
    stored = db.rows(
        f"SELECT {ITEM_COLUMNS} FROM player.item i"
        " LEFT JOIN player.item_proto p ON p.vnum = i.vnum"
        f" WHERE i.owner_id = %s AND i.window = 'SAFEBOX' ORDER BY i.pos LIMIT {SAFEBOX_LIMIT}",
        (account_id,),
    ) if account_id else []

    everything = [_decorate_item(item) for item in (*carried, *stored)]
    stones = _socket_definitions(everything)
    for item in everything:
        item["stones"] = [] if int(item["vnum"] or 0) in SKILL_BOOK_VNUMS else [
            stones[vnum] for vnum in (int(item.get(f"socket{i}") or 0) for i in range(3))
            if vnum in stones
        ]

    equipment, inventory = {}, []
    for item in carried:
        if item["window"] == "EQUIPMENT" and item["pos"] in chardata.EQUIPMENT_SLOTS:
            equipment[chardata.EQUIPMENT_SLOTS[item["pos"]]] = item
        elif item["window"] == "INVENTORY":
            page, slot = divmod(int(item["pos"] or 0), chardata.INVENTORY_PAGE_SIZE)
            row, column = divmod(slot, chardata.INVENTORY_COLUMNS)
            item.update(page=page, slot=slot, grid_row=row, grid_column=column)
            inventory.append(item)
    # The safebox pages the same way the backpack does. Without the page, an
    # item on page II was drawn over whatever sat in the same cell of page I.
    for item in stored:
        page, slot = divmod(int(item["pos"] or 0), chardata.INVENTORY_PAGE_SIZE)
        row, column = divmod(slot, chardata.INVENTORY_COLUMNS)
        item.update(page=page, slot=slot, grid_row=row, grid_column=column)
    return {
        "equipment": equipment,
        "inventory": inventory,
        "safebox": stored,
        "pages": max(1, max((item["page"] for item in inventory), default=0) + 1),
        "safebox_pages": max(1, max((item["page"] for item in stored), default=0) + 1),
    }


# The log rows that make an equipment history rather than noise: a bot writes
# thousands of GET, GET_GOLD and INFO_SOCKET rows, which buried the handful of
# events an operator actually opens this page for. (kind, label) - the kind is
# the colour of the marker beside the entry.
GEAR_HISTORY = {
    "REFINE SUCCESS": ("ok", "Ulepszenie udane"),
    "REMOVE (REFINE FAIL)": ("bad", "Spalone przy ulepszaniu"),
    "REFINE FISH_ROD SUCCESS": ("ok", "Wędka ulepszona"),
    "REFINE FISH_ROD FAIL": ("bad", "Wędka nieulepszona"),
    "PLAYERBOT_EQUIP": ("equip", "Założone"),
    "PLAYERBOT_BONUS_ADD": ("bonus", "Dodany bonus (Wzmocnienie)"),
    "PLAYERBOT_BONUS_CHANGE": ("bonus", "Zmienione bonusy (Zaczarowanie)"),
    "PLAYERBOT_BONUS_MARBLE": ("bonus", "Dodany 5. bonus (Marmur)"),
    "PLAYERBOT_GIFT_OUT": ("trade", "Podarowane"),
    "PLAYERBOT_GIFT_IN": ("trade", "Dostane w prezencie"),
    "PLAYERBOT_STALL_SOLD": ("trade", "Sprzedane na straganie"),
    "PLAYERBOT_SHOP_SELL": ("vendor", "Sprzedane handlarzowi"),
    "SHOP_BUY": ("trade", "Kupione na straganie"),
    "SAFEBOX PUT": ("storage", "Do magazynu"),
    "SAFEBOX GET": ("storage", "Z magazynu"),
    "EXCHANGE_TAKE": ("trade", "Z wymiany"),
    "EXCHANGE_GIVE": ("trade", "Oddane w wymianie"),
    "STONE_KILL": ("hunt", "Rozbity Metin"),
    "BOSS_KILL": ("hunt", "Zabity boss"),
}
# "290 x1 za 684000": vnum, count and the price the stall took.
SALE_HINT = re.compile(r"^(\d+)\s+x(\d+)\s+za\s+(\d+)")


def _gear_detail(how, hint, names):
    if how == "PLAYERBOT_GIFT_OUT":
        return f"→ {hint}"
    if how == "PLAYERBOT_GIFT_IN":
        return f"← {hint}"
    if how == "PLAYERBOT_STALL_SOLD":
        match = SALE_HINT.match(hint)
        return f"×{match.group(2)} za {thousands(match.group(3))} Yang" if match else ""
    if how == "PLAYERBOT_EQUIP":
        # "slot 4 zamiast 66": what the new piece replaced, when anything.
        parts = hint.split()
        if len(parts) >= 4 and parts[3].isdigit() and int(parts[3]) > 0:
            return f"zamiast {names.get(int(parts[3]), 'VNUM ' + parts[3])}"
        return ""
    if how in ("SAFEBOX PUT", "SAFEBOX GET"):
        parts = hint.rsplit(" ", 1)
        return f"×{parts[1]}" if len(parts) == 2 and parts[1].isdigit() and int(parts[1]) > 1 else ""
    return ""


def gear_history(pid):
    """The character's equipment story, newest first, in plain Polish."""
    hows = list(GEAR_HISTORY)
    marks = ",".join(["%s"] * len(hows))
    raw = db.rows(
        f"""SELECT l.time, l.how, l.hint, l.vnum, i.socket0,
              HEX(proto.locale_name) AS name_hex
            FROM log.log l
            LEFT JOIN player.item i ON i.id = l.what
            LEFT JOIN player.item_proto proto ON proto.vnum = l.vnum
            WHERE l.who = %s AND l.how IN ({marks})
            ORDER BY l.time DESC LIMIT {LOG_LIMIT}""",
        (pid, *hows),
    )
    replaced = set()
    for row in raw:
        row["how"], row["hint"] = game_text(row.get("how")), game_text(row.get("hint")).strip()
        parts = row["hint"].split()
        if row["how"] == "PLAYERBOT_EQUIP" and len(parts) >= 4 and parts[3].isdigit():
            replaced.add(int(parts[3]))
    names = {}
    if replaced:
        marks = ",".join(["%s"] * len(replaced))
        names = {int(row["vnum"]): game_text(row["item_name"]) for row in db.rows(
            "SELECT vnum, COALESCE(locale_name, CONCAT('VNUM ', vnum)) AS item_name"
            f" FROM player.item_proto WHERE vnum IN ({marks})", sorted(replaced))}
    history = []
    for row in raw:
        kind, label = GEAR_HISTORY.get(row["how"], ("other", row["how"]))
        vnum = int(row.get("vnum") or 0)
        history.append({
            "time": row["time"], "kind": kind, "label": label, "vnum": vnum,
            "item": display_name(vnum, hex_text(row.get("name_hex")), row.get("socket0")) if vnum else "",
            "detail": _gear_detail(row["how"], row["hint"], names),
        })
    return history


def recent_logs(pid):
    """The character's own raw tail of the game log, decoded."""
    logs = db.rows(
        "SELECT time, type, how, hint, what, vnum FROM log.log WHERE who = %s "
        f"ORDER BY time DESC LIMIT {LOG_LIMIT}",
        (pid,),
    )
    for entry in logs:
        for field in ("type", "how", "hint", "what"):
            entry[field] = game_text(entry.get(field))
    return logs


def stat_summary(pid):
    """Lifetime tallies from the log, for the numbers a player brags about.

    Only what the log really records: this engine writes no price on an NPC
    sale, so there is no "Yang earned from vendors" here to pretend about.
    """
    row = db.one(
        """SELECT SUM(how = 'BOSS_KILL') AS bosses, SUM(how = 'STONE_KILL') AS metins,
              SUM(how = 'DEAD_BY_NPC') AS deaths, SUM(how = 'DEAD_BY_PC') AS pvp_deaths,
              SUM(how = 'REFINE SUCCESS') AS refined, SUM(how = 'REMOVE (REFINE FAIL)') AS burned
            FROM log.log WHERE who = %s""",
        (pid,),
    )
    stats = {key: int(row.get(key) or 0)
             for key in ("bosses", "metins", "deaths", "pvp_deaths", "refined", "burned")}
    attempts = stats["refined"] + stats["burned"]
    stats["refine_rate"] = round(stats["refined"] * 100 / attempts, 1) if attempts else None
    # DEAD_BY_PC names the loser as who and the killer as what, so a win is
    # the same event read from the other side.
    stats["pvp_kills"] = int(db.scalar(
        "SELECT COUNT(*) FROM log.log WHERE how = 'DEAD_BY_PC' AND what = %s", (pid,), default=0) or 0)
    return stats


def ties(pid):
    """The people a character belongs with: a spouse and a guild."""
    marriage = db.one(
        """SELECT partner.id, partner.name FROM player.marriage m
            JOIN player.player partner ON partner.id = IF(m.pid1 = %s, m.pid2, m.pid1)
            WHERE (m.pid1 = %s OR m.pid2 = %s) AND m.is_married = 1 LIMIT 1""",
        (pid, pid, pid),
    )
    guild = db.one(
        """SELECT g.id, g.name, gg.name AS grade FROM player.guild_member gm
            JOIN player.guild g ON g.id = gm.guild_id
            LEFT JOIN player.guild_grade gg ON gg.guild_id = gm.guild_id AND gg.grade = gm.grade
            WHERE gm.pid = %s LIMIT 1""",
        (pid,),
    )
    return {
        "partner": {"id": marriage["id"], "name": game_text(marriage.get("name"))}
        if marriage.get("id") else None,
        "guild": {"id": guild["id"], "name": game_text(guild.get("name")),
                  "grade": game_text(guild.get("grade"))} if guild.get("id") else None,
    }


def offline_shop(pid):
    """What this character's stall sells, for how much, and where it stands.

    IkarusShop keeps no listing table of its own: the stall is a row in
    ikashop_offlineshop, its stock is player.item in the IKASHOP_OFFLINESHOP
    window, and the price is that item's ikashop_data JSON.
    """
    shop = db.one(
        "SELECT map, x, y, name, is_premium FROM player.ikashop_offlineshop WHERE owner = %s",
        (pid,),
    )
    if not shop:
        return None
    offers = db.rows(
        """SELECT i.id, i.vnum, i.count, i.pos, i.socket0, i.ikashop_data,
              COALESCE(p.locale_name, CONCAT('VNUM ', i.vnum)) AS item_name
            FROM player.item i LEFT JOIN player.item_proto p ON p.vnum = i.vnum
            WHERE i.owner_id = %s AND i.window = 'IKASHOP_OFFLINESHOP' ORDER BY i.pos""",
        (pid,),
    )
    for offer in offers:
        offer["item_name"] = display_name(offer["vnum"], offer["item_name"], offer.get("socket0"))
        try:
            offer["price"] = int(json.loads(offer.pop("ikashop_data", None) or "{}").get("yang") or 0)
        except (TypeError, ValueError, AttributeError):
            offer["price"] = 0
    return {
        "name": game_text(shop.get("name")) or "Bez nazwy",
        "map_index": int(shop.get("map") or 0),
        "x": int(shop.get("x") or 0), "y": int(shop.get("y") or 0),
        "is_premium": bool(shop.get("is_premium")),
        "offers": offers,
        "value": sum(offer["price"] * max(1, int(offer.get("count") or 1)) for offer in offers),
    }


# --- the core's own words about this character --------------------------------
LIVE_LOG_LINES = 120
# A syslog grows to tens of megabytes a day; only its tail is ever interesting,
# so that is all that is read.
LIVE_LOG_TAIL_BYTES = 512 * 1024


def live_log(name):
    """The newest syslog lines naming this character, across every core.

    Matched on word boundaries, so botgrom does not also collect botgrom2.
    """
    if not name:
        return []
    pattern = re.compile(r"(?<![A-Za-z0-9_])" + re.escape(name) + r"(?![A-Za-z0-9_])", re.IGNORECASE)
    matched = []
    for path in sorted(Path("/").glob(config.SYSLOG_GLOB.lstrip("/"))):
        try:
            with open(path, "rb") as handle:
                handle.seek(0, 2)
                size = handle.tell()
                handle.seek(max(0, size - LIVE_LOG_TAIL_BYTES))
                chunk = handle.read()
        except OSError:
            continue
        lines = chunk.decode("cp1250", "replace").splitlines()
        if size > LIVE_LOG_TAIL_BYTES:
            lines = lines[1:]  # the first line was cut in half by the seek
        matched.extend(line.rstrip() for line in lines if pattern.search(line))
    return matched[-LIVE_LOG_LINES:]


def enrich(character, state):
    """Fold the live status, derived bars and labels into a database row."""
    if state:
        character.update(state)
        character["live"] = True
        character["personality"] = botdata.label("personality", state.get("personality"))
        character["ambition"] = botdata.label("ambition", state.get("ambition"))
        character["goal"] = botdata.label("goal", state.get("goal"))
        # The free-text status is what the core wants to say about itself; the
        # numeric action is the fallback when it says nothing.
        character["activity"] = state.get("status") or botdata.label("action", state.get("action"))
    else:
        character.update({"live": False, "personality": "Postać offline",
                          "ambition": "—", "goal": "—", "activity": "—"})
    character["class_profile"] = chardata.class_profile(character.get("job"))
    character["job_name"] = character["class_profile"]["name"]
    character["experience"] = chardata.experience_progress(character.get("level"), character.get("exp"))
    character["honour"] = chardata.honour_rank(character.get("alignment"))
    # The live feed exposes exact max HP. The original schema never persisted
    # max MP, so an offline character shows a current-value bar until it is
    # next seen live.
    character["max_hp"] = max(int(character.get("max_hp") or 0), int(character.get("hp") or 0), 1)
    character["max_mp"] = max(int(character.get("max_mp") or 0), int(character.get("mp") or 0), 1)
    character["hp_percent"] = min(100, round(int(character.get("hp") or 0) * 100 / character["max_hp"], 1))
    character["mp_percent"] = min(100, round(int(character.get("mp") or 0) * 100 / character["max_mp"], 1))
    raw_skills = character.pop("skill_level", b"")
    character["skills"] = chardata.parse_skills(raw_skills, character.get("job"), character.get("skill_group"))
    character["passive_skills"] = chardata.parse_passive_skills(raw_skills)
    character["cash"] = int(character.get("cash") or 0)
    return character
