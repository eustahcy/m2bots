"""Characters: the roster, one profile, and everything hanging off it."""
from .. import db, engine, live
from ..gamedata import bots as botdata
from ..gamedata import characters as chardata
from ..gamedata import items as itemdata
from ..text import game_text

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
        f" {engine.EMPIRE_EXPR} AS empire"
        " FROM player.player p"
        " LEFT JOIN account.account a ON a.id = p.account_id"
        " LEFT JOIN player.player_index pi ON pi.id = p.account_id"
        " WHERE p.id = %s",
        (pid,),
    )


def _decorate_item(item):
    item["item_name"] = game_text(item["item_name"])
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
        if int(item.get(f"socket{i}") or 0) > 0
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
        item["stones"] = [
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
    for item in stored:
        slot = int(item["pos"] or 0) % chardata.INVENTORY_PAGE_SIZE
        row, column = divmod(slot, chardata.INVENTORY_COLUMNS)
        item.update(grid_row=row, grid_column=column)
    return {
        "equipment": equipment,
        "inventory": inventory,
        "safebox": stored,
        "pages": max(1, max((item["page"] for item in inventory), default=0) + 1),
    }


# A plain sentence and (when the row names an item) its icon, for the log
# entries worth reading at a glance rather than as raw type/how/hint fields.
# log.log carries its own vnum column, so no join back to player.item - whose
# row a refined or traded item may no longer have - is needed to show one.
GEAR_EVENT_LABELS = {
    "REFINE SUCCESS": "Ulepszył",
    "PLAYERBOT_EQUIP": "Założył",
    "PLAYERBOT_GIFT_OUT": "Podarował",
    "PLAYERBOT_GIFT_IN": "Otrzymał w prezencie",
    "PLAYERBOT_STALL_SOLD": "Sprzedał na straganie",
    "STONE_KILL": "Rozbił Metina",
    "BOSS_KILL": "Zabił bossa",
    "SKILLUP": "Rozwinął umiejętność",
}


def recent_logs(pid):
    """The character's own tail of the game log, decoded."""
    logs = db.rows(
        "SELECT time, type, how, hint, what, vnum FROM log.log WHERE who = %s "
        f"ORDER BY time DESC LIMIT {LOG_LIMIT}",
        (pid,),
    )
    for entry in logs:
        for field in ("type", "how", "hint", "what"):
            entry[field] = game_text(entry.get(field))
        entry["label"] = GEAR_EVENT_LABELS.get(entry["how"])
    return logs


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
    character["skills"] = chardata.parse_skills(
        character.pop("skill_level", b""), character.get("job"), character.get("skill_group"))
    return character
