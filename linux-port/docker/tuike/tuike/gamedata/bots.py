"""What a Playerbot is doing, and the knobs that change what it will do next."""

# EPlayerBotPersonality (playerbot_types.h): MERCHANT is 5 and WANDERER is 6.
# A table that stopped at 5 read the shopkeeper as a wanderer and did not read
# the five personalities added after him at all.
BOT_PERSONALITIES = {
    0: "Wytrwały poszukiwacz", 1: "Pogromca Metinów", 2: "Towarzysz drużyny",
    3: "Mistrz ekwipunku", 4: "Rozważny zbieracz", 5: "Handlarz", 6: "Wędrowiec",
    7: "Dropek Metinów", 8: "Dropek z M3", 9: "Dropek z M2", 10: "Dropek medali",
}
BOT_AMBITIONS = {
    0: "Poziom", 1: "Ekwipunek", 2: "Metiny", 3: "Koń", 4: "Biolog", 5: "Umiejętności",
}
BOT_GOALS = {
    0: "Zdobywanie poziomu", 1: "Przetrwanie", 2: "Wybór profesji", 3: "Zdobycie ekwipunku",
    4: "Uzupełnienie zapasów", 5: "Ulepszanie EQ", 6: "Rozwój umiejętności",
    7: "Polowanie na Metiny", 8: "Silne cele w PT", 9: "Misja Biologa",
    10: "Misja Polowania", 11: "Rozwój konia",
}
BOT_ACTIONS = {
    0: "Planuje następny ruch", 1: "Podróżuje", 2: "Walczy", 3: "Podnosi łup",
    4: "Regeneruje się", 5: "Wybiera profesję", 6: "Handluje", 7: "Ulepsza EQ",
    8: "Czyta KU", 9: "Wkłada KD", 10: "Organizuje PT", 11: "Robi Biologa",
    12: "Odwiedza Stajennego", 13: "Prowadzi stragan", 14: "Łowi ryby",
    15: "Przegląda stragany", 16: "Wabi potwory", 17: "Odpoczywa w mieście",
    18: "Kopie rudę",
}
LABEL_SETS = {
    "personality": BOT_PERSONALITIES, "ambition": BOT_AMBITIONS,
    "goal": BOT_GOALS, "action": BOT_ACTIONS,
}

# Actions where a bot stands still by choice: a market stall, a fishing rod,
# browsing stalls, an NPC counter, the blacksmith, the trainer, resting, mining.
# Without this every shopkeeper was flagged "possibly stuck" - and a flag based
# on the free-text status only ever caught the anglers.
STATIONARY_ACTIONS = frozenset({5, 6, 7, 13, 14, 15, 17, 18})
# Fishing shows up in the free-text status on cores that do not set the action.
STATIONARY_STATUS_MARKERS = ("łowi", "lowi", "ryb", "fishing", "czekam na branie")
# goal 7 + action 2 is a bot in a fight with a Metin stone.
METIN_GOAL, FIGHT_ACTION, SHOPKEEPER_ACTION = 7, 2, 13
# Two positions closer together than this (squared world units) count as "did
# not move" when the stuck check compares now with then.
STUCK_DISTANCE_SQUARED = 40000

# The live weight file the core re-reads every five seconds. 25 = rarely,
# 100 = the author's default, 250 = often.
AI_WEIGHT_KEYS = (
    ("RESTOCK", "Mikstury", "🧪"), ("REFINE", "Kowal", "🔨"),
    ("SKILL", "Księgi umiejętności", "📖"), ("HORSE", "Koń", "🐎"),
    ("BIOLOG", "Biolog", "🧬"), ("METIN", "Metiny", "🗿"),
    ("PARTY", "Grupy", "👥"), ("HUNTING", "Misje polowania", "🏹"),
    ("LEVEL", "Bicie potworów", "⚔️"), ("FISHING", "Wędkowanie", "🎣"),
    ("TRADE", "Stragany", "🏪"),
)
AI_WEIGHT_MIN, AI_WEIGHT_MAX, AI_WEIGHT_NEUTRAL = 25, 250, 100
# These share the weight file but are switches or direct settings, not weights.
AI_SWITCHES = ("CHAT", "BOOKS", "NIGHT")
AI_PERCENTAGES = ("SCRAP", "REST", "PROFIT", "KINGDOMPVP")
AI_PERMILLE = ("CHEST", "CHEST_STONE")
# A refine level (0-9), not a percentage or a permille - the point at which an
# ordinary weapon/armor spare stops being "too little refined to sell" and
# starts being real gear. Lower lists more of the bag as goods; higher keeps
# more of it in the bag or the scrap pile. None means the core's own default
# (PLAYERBOT_SHOP_MIN_GEAR_REFINE, currently +4) - see GetPlayerBotShopMinGearRefine.
AI_REFINE_FLOOR = ("SCRAPFLOOR",)
AI_REFINE_FLOOR_MIN, AI_REFINE_FLOOR_MAX = 0, 9
AI_LIVE_DEFAULTS = {"CHAT": 1, "BOOKS": 1, "NIGHT": 1, "SCRAP": 0, "REST": 100,
                    "PROFIT": 0, "KINGDOMPVP": 0, "CHEST": None, "CHEST_STONE": None,
                    "SCRAPFLOOR": None}
# What the form offers when the core has never written a value.
AI_CHEST_SUGGESTIONS = {"CHEST": 10, "CHEST_STONE": 300}
AI_REFINE_FLOOR_SUGGESTION = {"SCRAPFLOOR": 4}
AI_SPECIAL_KEYS = frozenset(AI_LIVE_DEFAULTS)

# player.quest rows the Biologist writes. lValue is this when a mission is done.
BIOLOGIST_COMPLETE_STATE = 557528158
BIOLOGIST_QUEST_PATTERN = "^(make_herb_lv[0-9]+|collect_quest_lv[0-9]+)$"
# The classic six plus the Orc Tooth task added in 1.29.10. The database lookup
# discovers later ones as soon as the game creates their rows; this list keeps
# the "x / y missions" scale right before anybody has started a new one.
BIOLOGIST_FALLBACK_MISSIONS = (
    "make_herb_lv4", "make_herb_lv7", "make_herb_lv10", "make_herb_lv15",
    "make_herb_lv20", "make_herb_lv25", "collect_quest_lv30",
)


def label(field, value):
    """Name a numeric live-status field, or show its number if unknown."""
    try:
        value = int(value or 0)
    except (TypeError, ValueError):
        value = 0
    return LABEL_SETS.get(field, {}).get(value, f"#{value}")


def is_stationary(status, action=None):
    """True when a bot is standing still on purpose rather than wedged."""
    try:
        if int(action or 0) in STATIONARY_ACTIONS:
            return True
    except (TypeError, ValueError):
        pass
    text = str(status or "").casefold()
    return any(marker in text for marker in STATIONARY_STATUS_MARKERS)
