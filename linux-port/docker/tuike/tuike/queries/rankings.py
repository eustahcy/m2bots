"""Global bot rankings.

Every ranking returns rows shaped the same way - id, name, level, gold and a
`detail` string the table prints - so one template draws all of them.
"""
from .. import cache, db, engine, live, settings
from ..gamedata import bots as botdata
from ..gamedata import characters as chardata
from ..text import game_text, hours, thousands

LIMIT = 100
# A refine-success percentage needs a sample: without a floor, a bot whose one
# and only refine succeeded sits at 100% on top of the ladder for ever.
REFINE_RATE_MIN_ATTEMPTS = 20
# The engine logs a failed refine only as the burn - REMOVE (REFINE FAIL). The
# plain 'REFINE FAIL' row exists in the schema and has zero entries, so a rate
# built on it shows every character at 100%.
REFINE_OK, REFINE_BURNED = "REFINE SUCCESS", "REMOVE (REFINE FAIL)"


def includes_players():
    """The /manage switch: rank real players beside the bots, or bots only."""
    return settings.read().get("rankings_include_players") == "1"


def scope(alias="p"):
    """Who a ranking counts, as SQL for whichever alias the query uses.

    Bots only by default. With real players let in, the installer's seeded GM
    characters are kept out explicitly - they would otherwise top every ladder
    on their first day and bury anyone actually playing.
    """
    if not includes_players():
        return engine.bot_predicate(alias)
    ref = f"{alias}." if alias else ""
    names = ",".join("'" + name.replace("'", "''") + "'" for name in engine.SEEDED_CHARACTERS)
    return f"{ref}name NOT IN ({names})"

KINDS = {
    "level": "Poziom",
    "weapon": "Broń",
    "armor": "Zbroja",
    "weapon30": "Broń 30 Lv",
    "plus9": "Przedmiot +9",
    "gold": "Yang",
    "items": "Przedmioty",
    "horse": "Koń",
    "biologist": "Biolog",
    "skills": "Umiejętności",
    "shops": "Otwarte stragany",
    "playtime": "Czas gry",
    "bosses": "Bossy",
    "refine_rate": "Skuteczność ulepszeń",
}
# A hunting ranking used to sit here. On this engine line levelup.quest lives
# in quest/_unused, no kill hook fires and the counter stays at zero for every
# bot, so the page was a hundred rows of "Ukończone do Lv 0".

WEAPON30_SORTS = {
    "avg": "Średnie obrażenia",
    "skill": "Obrażenia umiejętności",
    "upgrade": "Poziom ulepszenia",
}
# The six level-30 weapon series, one per weapon kind: each runs base+0..+9.
WEAPON30_RANGES = ((290, 299), (1170, 1179), (2150, 2159),
                   (3210, 3219), (5110, 5119), (7160, 7169))
# EWearPositions: 4 is the weapon hand, 0 the body armour.
SLOT_WEAPON, SLOT_BODY = 4, 0


def _attribute_value(attr_id, alias="i"):
    """SQL for "the value of attribute N on this item, or 0 if it has none".

    An item stores seven (type, value) pairs in seven column pairs, so finding
    one attribute means looking in all seven. -999 marks "not this slot" so
    GREATEST picks the real value, and a GREATEST of nothing but -999 means the
    item does not carry the attribute at all.
    """
    slots = ",".join(
        f"CASE WHEN {alias}.attrtype{i}={attr_id} THEN {alias}.attrvalue{i} ELSE -999 END"
        for i in range(7)
    )
    return f"IF(GREATEST({slots})=-999, 0, GREATEST({slots}))"


SKILL_DAMAGE_SQL = _attribute_value(engine.ATTR_SKILL_DAMAGE)
AVG_DAMAGE_SQL = _attribute_value(engine.ATTR_AVG_DAMAGE)


def _equipped_item_ranking(slot):
    """Whoever wears the highest-numbered, most-upgraded thing in this slot."""
    return db.rows(
        f"""SELECT p.id, p.name, p.level, p.gold, i.vnum,
              COALESCE(ip.locale_name, CONCAT('VNUM ', i.vnum)) AS detail
            FROM player.player p
            LEFT JOIN player.item i
              ON i.owner_id = p.id AND i.window = 'EQUIPMENT' AND i.pos = {slot}
            LEFT JOIN player.item_proto ip ON ip.vnum = i.vnum
            WHERE {scope()}
            ORDER BY MOD(COALESCE(i.vnum, 0), 10) DESC, i.vnum DESC, p.level DESC
            LIMIT {LIMIT}"""
    )


def _weapon30(sort_by):
    """The level-30 weapons, by whichever of their two damage lines was asked.

    avg_damage reads APPLY_NORMAL_HIT_DAMAGE_BONUS and skill_damage reads
    APPLY_SKILL_DAMAGE_BONUS - the names common/length.h gives 72 and 71. They
    are easy to swap and the mistake is invisible: the column headings still
    look plausible, and ORDER BY quietly picks the wrong hundred rows, so
    re-sorting in Python afterwards cannot put it right.
    """
    ranges = " OR ".join(f"(i.vnum BETWEEN {low} AND {high})" for low, high in WEAPON30_RANGES)
    order = {
        "avg": "avg_damage DESC, skill_damage DESC, p.level DESC",
        "skill": "skill_damage DESC, avg_damage DESC, p.level DESC",
        "upgrade": "MOD(i.vnum,10) DESC, avg_damage DESC, skill_damage DESC, p.level DESC",
    }[sort_by]
    return db.rows(
        f"""SELECT p.id, p.name, p.level, p.gold, i.vnum,
              COALESCE(ip.locale_name, CONCAT('VNUM ', i.vnum)) AS item_name,
              {SKILL_DAMAGE_SQL} AS skill_damage,
              {AVG_DAMAGE_SQL} AS avg_damage
            FROM player.item i
            JOIN player.player p ON p.id = i.owner_id
            LEFT JOIN player.item_proto ip ON ip.vnum = i.vnum
            WHERE {scope()} AND ({ranges})
            ORDER BY {order} LIMIT {LIMIT}"""
    )


def biologist_missions():
    """The known Biologist tasks, plus any the game has since created rows for."""
    names = set(botdata.BIOLOGIST_FALLBACK_MISSIONS)
    try:
        discovered = db.rows(
            "SELECT DISTINCT szName FROM player.quest WHERE szName REGEXP %s",
            (botdata.BIOLOGIST_QUEST_PATTERN,))
        names.update(row["szName"] for row in discovered if row.get("szName"))
    except Exception:
        # A world whose quest table is unreadable still gets the classic scale.
        pass

    def order(name):
        digits = "".join(character for character in name if character.isdigit())
        return (int(digits or 0), name)

    return tuple(sorted(names, key=order))


def _biologist():
    missions = biologist_missions()
    marks = ",".join(["%s"] * len(missions))
    return db.rows(
        f"""SELECT p.id, p.name, p.level, p.gold,
              COUNT(DISTINCT q.szName) AS score,
              CONCAT(COUNT(DISTINCT q.szName), ' / {len(missions)} misji') AS detail
            FROM player.player p
            LEFT JOIN player.quest q
              ON q.dwPID = p.id AND q.szName IN ({marks})
             AND q.szState = '__status' AND q.lValue = %s
            WHERE {scope()}
            GROUP BY p.id ORDER BY score DESC, p.level DESC LIMIT {LIMIT}""",
        (*missions, botdata.BIOLOGIST_COMPLETE_STATE),
    )


def _skills():
    """Every bot with a profession, scored by its best skill - not by level.

    Sorting by level first answers a different question: a level-30 bot with a
    mastered skill stood behind four hundred level-50s with none and never
    appeared. skill_level is a blob, so the scoring happens in Python and the
    whole set has to come back.
    """
    roster = db.rows(
        f"""SELECT p.id, p.name, p.level, p.gold, p.job, p.skill_group, p.skill_level
            FROM player.player p WHERE {scope()} AND p.skill_group > 0"""
    )
    for bot in roster:
        skills = chardata.parse_skills(bot.get("skill_level"), bot.get("job"), bot.get("skill_group"))
        best = chardata.best_skill(skills)
        bot["score"] = (chardata.skill_tier(best["rank"]), best["level"]) if best else (0, 0)
        bot["detail"] = f"{best['name']} · {best['rank']}" if best else "Brak rozwiniętych umiejętności"
        bot.pop("skill_level", None)
    return sorted(roster, key=lambda bot: (bot["score"], bot["level"]), reverse=True)[:LIMIT]


def _shops():
    """Bots the cores currently report standing behind an open stall."""
    ids = live.shopkeeper_ids()
    if not ids:
        return []
    marks = ",".join(["%s"] * len(ids))
    return db.rows(
        f"""SELECT p.id, p.name, p.level, p.gold, 'Stragan otwarty' AS detail
            FROM player.player p WHERE p.id IN ({marks})
            ORDER BY p.level DESC LIMIT {LIMIT}""",
        ids,
    )


@cache.ttl(300)
def _refine_rate_rows(scope_sql):
    return db.rows(
        f"""SELECT p.id, p.name, p.level, p.gold,
              ROUND(100 * SUM(l.how = %s) / COUNT(*), 1) AS score,
              SUM(l.how = %s) AS succeeded, COUNT(*) AS attempts
            FROM log.log l JOIN player.player p ON p.id = l.who
            WHERE {scope_sql} AND l.how IN (%s, %s)
            GROUP BY p.id, p.name HAVING COUNT(*) >= {REFINE_RATE_MIN_ATTEMPTS}
            ORDER BY score DESC, attempts DESC, p.level DESC LIMIT {LIMIT}""",
        (REFINE_OK, REFINE_OK, REFINE_OK, REFINE_BURNED),
    )


def _refine_rate():
    """Share of refines that worked, all-time, for anyone with enough tries.

    All-time and not windowed, so the number matches the one on the
    character's own page. Remembered for five minutes: it groups every refine
    ever logged, and the dashboard carousel asks for it on each visit.
    """
    rows = _refine_rate_rows(scope())
    for row in rows:
        row["detail"] = (f"{row.get('score')}% ({int(row.get('succeeded') or 0)}"
                         f"/{int(row.get('attempts') or 0)} ulepszeń)")
    return rows


def _log_count(how, days, unit):
    """How often something happened to each bot, from the indexed log columns."""
    return db.rows(
        f"""SELECT p.id, p.name, p.level, p.gold, COUNT(*) AS score,
              CONCAT(COUNT(*), ' {unit} · {days} dni') AS detail
            FROM log.log l JOIN player.player p ON p.id = l.who
            WHERE {scope()} AND l.how = %s AND l.time >= NOW() - INTERVAL {days} DAY
            GROUP BY p.id, p.name ORDER BY score DESC, p.level DESC, p.name
            LIMIT {LIMIT}""",
        (how,),
    )


def _simple(order_by, score_column, detail_sql):
    return db.rows(
        f"""SELECT p.id, p.name, p.level, p.gold, {score_column} AS score,
              {detail_sql} AS detail
            FROM player.player p WHERE {scope()}
            ORDER BY {order_by} LIMIT {LIMIT}"""
    )


def ranking(kind, sort_by="avg"):
    """One ranking, by name. Unknown names fall back to the level ladder."""
    if kind == "gold":
        return _simple("p.gold DESC, p.level DESC", "p.gold", "CONCAT(FORMAT(p.gold,0),' Yang')")
    if kind == "playtime":
        return _simple("p.playtime DESC, p.level DESC", "p.playtime", "CONCAT(FLOOR(p.playtime/60),' h')")
    if kind == "horse":
        return _simple("p.horse_level DESC, p.level DESC", "p.horse_level", "CONCAT('Koń Lv ',p.horse_level)")
    if kind == "weapon":
        return _equipped_item_ranking(SLOT_WEAPON)
    if kind == "armor":
        return _equipped_item_ranking(SLOT_BODY)
    if kind == "weapon30":
        return _weapon30(sort_by if sort_by in WEAPON30_SORTS else "avg")
    if kind == "bosses":
        return _log_count("BOSS_KILL", 7, "zabitych bossów")
    if kind == "biologist":
        return _biologist()
    if kind == "skills":
        return _skills()
    if kind == "shops":
        return _shops()
    if kind == "refine_rate":
        return _refine_rate()
    if kind == "items":
        return db.rows(
            f"""SELECT p.id, p.name, p.level, p.gold, COUNT(i.id) AS score,
                  CONCAT(COUNT(i.id), ' przedmiotów') AS detail
                FROM player.player p
                LEFT JOIN player.item i ON i.owner_id = p.id AND i.window = 'INVENTORY'
                WHERE {scope()} GROUP BY p.id ORDER BY score DESC, p.level DESC LIMIT {LIMIT}"""
        )
    if kind == "plus9":
        # What counts as equipment is decided by item_proto, not by vnum size:
        # "below 12000" was meant to exclude materials and excluded every
        # shield (13xxx) and all the jewellery with them. Types 1 and 2 are
        # exactly the set whose upgrade chain runs base+0..+9.
        # Carried items only: in SAFEBOX owner_id is an ACCOUNT id, so a +9 in
        # someone's storage was credited to whichever character happened to
        # share that number.
        return db.rows(
            f"""SELECT p.id, p.name, p.level, p.gold, i.vnum,
                  COALESCE(ip.locale_name, CONCAT('VNUM ', i.vnum)) AS detail
                FROM player.item i
                JOIN player.player p ON p.id = i.owner_id
                LEFT JOIN player.item_proto ip ON ip.vnum = i.vnum
                WHERE {scope()} AND ip.type IN (1,2) AND MOD(i.vnum,10) = 9
                  AND i.window IN ('EQUIPMENT', 'INVENTORY')
                ORDER BY i.vnum DESC, p.level DESC LIMIT {LIMIT}"""
        )
    return _simple("p.level DESC, p.exp DESC", "p.level", "'Poziom'")


def decorate(rows, kind):
    """Add kingdom, class and experience to a ranking, and finish its detail."""
    ids = [row["id"] for row in rows]
    if ids:
        marks = ",".join(["%s"] * len(ids))
        # player_index.empire, because that is the column the core reads when
        # it decides where a bot lives; the account's own copy was left at one
        # kingdom for the whole cohort.
        facts = db.rows(
            f"SELECT p.id, p.level, p.exp, p.job, {engine.EMPIRE_EXPR} AS empire"
            " FROM player.player p"
            " LEFT JOIN player.player_index pi ON pi.id = p.account_id"
            " LEFT JOIN account.account a ON a.id = p.account_id"
            f" WHERE p.id IN ({marks})",
            ids,
        )
        by_id = {row["id"]: row for row in facts}
        for row in rows:
            fact = by_id.get(row["id"], {})
            row["empire"] = fact.get("empire", 0)
            row["job"] = fact.get("job", 0)
            row["experience"] = chardata.experience_progress(fact.get("level"), fact.get("exp"))
    for row in rows:
        row.setdefault("experience", {"percent": 0})
        if kind == "weapon30":
            row["detail"] = (
                f"Średnie obrażenia: {int(row.get('avg_damage') or 0)}% · "
                f"Obrażenia umiejętności: {int(row.get('skill_damage') or 0)}% · "
                f"{game_text(row.get('item_name'))}")
        else:
            row["detail"] = game_text(row.get("detail"))
    return rows


# --- the dashboard's condensed versions -------------------------------------
QUICK_SIZE = 10


def _quick(title, subtitle, rows, value):
    return {
        "title": title, "subtitle": subtitle,
        "items": [{"id": row["id"], "name": row["name"], "value": value(row)}
                  for row in rows[:QUICK_SIZE]],
    }


def quick_rankings(top_by_level):
    """The eight small ladders the dashboard rotates through."""
    boards = [
        _quick("Poziom", "najwyższe poziomy", top_by_level, lambda row: f"Lv {row['level']}"),
        _quick("Czas gry", "najdłużej w świecie", ranking("playtime"),
               lambda row: f"{hours(row.get('playtime') or row.get('score'))} h"),
        _quick("Yang", "najwięcej przy postaci", ranking("gold"),
               lambda row: thousands(row.get("gold"))),
        _quick("Broń 30 Lv", "średnie / umiejętności", ranking("weapon30"),
               lambda row: f"Śr. {int(row.get('avg_damage') or 0)}% · "
                           f"Um. {int(row.get('skill_damage') or 0)}%"),
        _quick("Metiny", "rozbite · ostatnie 7 dni", _log_count("STONE_KILL", 7, "szt."),
               lambda row: f"{int(row['score'])} szt."),
        _quick("Bossy", "zabite · ostatnie 7 dni", ranking("bosses"),
               lambda row: f"{int(row['score'])} szt."),
        _quick("Ryby", "wyłowione · ostatnie 7 dni", _fishing(),
               lambda row: f"{int(row['score'])} szt."),
        _quick("Skuteczność ulepszeń", f"min. {REFINE_RATE_MIN_ATTEMPTS} prób", _refine_rate(),
               lambda row: f"{row['score']}%"),
    ]
    _attach_jobs(boards)
    return boards


def _fishing():
    return db.rows(
        f"""SELECT p.id, p.name, p.level, COUNT(*) AS score
            FROM log.log l JOIN player.player p ON p.id = l.who
            WHERE {scope()} AND l.time >= NOW() - INTERVAL 7 DAY
              AND (l.what LIKE '%%ryb%%' OR l.what LIKE '%%fish%%')
            GROUP BY p.id, p.name ORDER BY score DESC, p.name LIMIT {QUICK_SIZE}"""
    )


def _attach_jobs(boards):
    """One query for the class portraits every small ladder needs."""
    ids = {item["id"] for board in boards for item in board["items"]}
    if not ids:
        return
    marks = ",".join(["%s"] * len(ids))
    jobs = {row["id"]: row["job"]
            for row in db.rows(f"SELECT id, job FROM player.player WHERE id IN ({marks})", list(ids))}
    for board in boards:
        for item in board["items"]:
            item["job"] = jobs.get(item["id"], 0)


def top_by_level(limit=QUICK_SIZE):
    """The level ladder the dashboard opens on, with live positions folded in."""
    rows = db.rows(
        "SELECT id, name, level, exp, job, map_index, playtime FROM player.player"
        f" WHERE {scope('')} ORDER BY level DESC, exp DESC LIMIT {limit}"
    )
    current = live.statuses()
    for bot in rows:
        if bot["id"] in current:
            bot["map_index"] = current[bot["id"]]["map_index"]
    return rows


def global_leader_id():
    """The single highest bot in the world, highlighted wherever it appears."""
    return db.one(
        f"SELECT id FROM player.player WHERE {engine.BOT_IS_BARE}"
        " ORDER BY level DESC, exp DESC LIMIT 1"
    ).get("id")
