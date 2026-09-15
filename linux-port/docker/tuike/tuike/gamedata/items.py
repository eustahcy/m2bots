"""Item prototypes, icons and the bonus lines a tooltip prints.

The engine numbers a bonus line differently depending on the build. On r40250
an item's attrtype is already an APPLY_* id. On mt2009 it is a POINT_* id and
has to go through POINT_TO_APPLY first - and thirty of those points have no
APPLY id at all, so they get a pseudo id of 1000 + point purely so the label
table can name them. Without that the panel printed "Bonus #139".
"""
import json

from .. import config

# EItemTypes, in order, from Server/common/item_length.h.
ITEM_TYPE_NAMES = (
    "ITEM_NONE", "ITEM_WEAPON", "ITEM_ARMOR", "ITEM_USE", "ITEM_AUTOUSE",
    "ITEM_MATERIAL", "ITEM_SPECIAL", "ITEM_TOOL", "ITEM_LOTTERY", "ITEM_ELK",
    "ITEM_METIN", "ITEM_CONTAINER", "ITEM_FISH", "ITEM_ROD", "ITEM_RESOURCE",
    "ITEM_CAMPFIRE", "ITEM_UNIQUE", "ITEM_SKILLBOOK", "ITEM_QUEST", "ITEM_POLYMORPH",
    "ITEM_TREASURE_BOX", "ITEM_TREASURE_KEY", "ITEM_SKILLFORGET", "ITEM_GIFTBOX",
    "ITEM_PICK", "ITEM_HAIR", "ITEM_TOTEM", "ITEM_BLEND", "ITEM_COSTUME", "ITEM_DS",
    "ITEM_SPECIAL_DS", "ITEM_EXTRACT", "ITEM_SECONDARY_COIN", "ITEM_RING", "ITEM_BELT",
    "ITEM_PET", "ITEM_MEDIUM", "ITEM_GACHA", "ITEM_SOUL", "ITEM_PASSIVE",
)
ITEM_TYPE_WEAPON, ITEM_TYPE_ARMOR = 1, 2
# Types the ordinary inventory cannot hold, so mass grants refuse them.
UNGRANTABLE_ITEM_TYPES = frozenset({0, 10, 29, 30})

# APPLY_* id -> (Polish label, unit). Grouped the way common/length.h groups
# them, so a new engine line can be diffed against the header.
APPLY_LABELS = {
    # 1-16: the core attributes and the speeds.
    1: ("Maks. PŻ", ""), 2: ("Maks. PM", ""), 3: ("Witalność", ""), 4: ("Inteligencja", ""),
    5: ("Siła", ""), 6: ("Zręczność", ""), 7: ("Szybkość ataku", "%"), 8: ("Szybkość ruchu", "%"),
    9: ("Szybkość zaklęcia", "%"), 10: ("Regeneracja PŻ", "%"), 11: ("Regeneracja PM", "%"),
    12: ("Odporność na truciznę", "%"), 13: ("Szansa na omdlenie", "%"),
    14: ("Szansa na spowolnienie", "%"), 15: ("Szansa na cios krytyczny", "%"),
    16: ("Szansa na przeszywający", "%"),
    # 17-23: attack value and the "strong against" family.
    17: ("Wartość ataku", ""), 18: ("Silny przeciw ludziom", "%"), 19: ("Silny przeciw zwierzętom", "%"),
    20: ("Silny przeciw orkom", "%"), 21: ("Silny przeciw mistykom", "%"),
    22: ("Silny przeciw nieumarłym", "%"), 23: ("Silny przeciw diabłom", "%"),
    # 24-29: leech, burn and the two evasions.
    24: ("Kradzież PŻ", "%"), 25: ("Kradzież PM", "%"), 26: ("Spalenie PM", "%"),
    27: ("Odzyskanie PM po obrażeniach", "%"), 28: ("Szansa na blok", "%"),
    29: ("Szansa na unik strzał", "%"),
    # 30-43: the resistances, the two reflects and the kill rewards.
    30: ("Odporność na miecze", "%"), 31: ("Odporność na broń dwuręczną", "%"),
    32: ("Odporność na sztylety", "%"), 33: ("Odporność na dzwony", "%"),
    34: ("Odporność na wachlarze", "%"), 35: ("Odporność na strzały", "%"),
    36: ("Odporność na ogień", "%"), 37: ("Odporność na błyskawice", "%"),
    38: ("Odporność na magię", "%"), 39: ("Odporność na wiatr", "%"),
    40: ("Odbicie obrażeń fizycznych", "%"), 41: ("Odbicie klątwy", "%"),
    42: ("Skrócenie trucia", "%"), 43: ("Odzyskanie PM po zabiciu", "%"),
    # 44-53: the bonuses a player actually shops for.
    44: ("Bonus doświadczenia", "%"), 45: ("Bonus Yang", "%"), 46: ("Bonus dropu przedmiotów", "%"),
    47: ("Bonus mikstur", "%"), 48: ("Odzyskanie PŻ po zabiciu", "%"),
    49: ("Odporność na omdlenie", ""), 50: ("Odporność na spowolnienie", ""),
    51: ("Odporność na przewrócenie", ""), 52: ("Bonus umiejętności", "%"),
    53: ("Zasięg łuku", "%"),
    # 54-64: the flat attack/defence lines and the class counters.
    54: ("Wartość ataku", ""), 55: ("Wartość obrony", ""), 56: ("Magiczna wartość ataku", ""),
    57: ("Magiczna wartość obrony", ""), 58: ("Szansa na klątwę", "%"),
    59: ("Maks. wytrzymałość", ""), 60: ("Silny przeciw wojownikom", "%"),
    61: ("Silny przeciw ninja", "%"), 62: ("Silny przeciw surom", "%"),
    63: ("Silny przeciw szamanom", "%"), 64: ("Silny przeciw potworom", "%"),
    # 70-91: the modern lines. 71 is APPLY_SKILL_DAMAGE_BONUS and 72 is
    # APPLY_NORMAL_HIT_DAMAGE_BONUS - common/length.h says so in its own
    # comments, and the weapon ranking sorts by exactly these two.
    70: ("Maks. PŻ", "%"), 71: ("Obrażenia umiejętności", "%"), 72: ("Średnie obrażenia", "%"),
    73: ("Odporność na umiejętności", "%"), 74: ("Odporność na średnie obrażenia", "%"),
    75: ("Bonus doświadczenia", "%"), 76: ("Bonus dropu", "%"), 77: ("Kradzież PŻ", "%"),
    78: ("Odporność na wojowników", "%"), 79: ("Odporność na ninja", "%"),
    80: ("Odporność na sury", "%"), 81: ("Odporność na szamanów", "%"),
    82: ("Energia", "%"), 83: ("Wartość obrony", ""), 84: ("Bonus atrybutów kostiumu", "%"),
    85: ("Magiczny atak", "%"), 86: ("Atak fizyczny i magiczny", "%"),
    87: ("Odporność na lód", "%"), 88: ("Odporność na ziemię", "%"), 89: ("Odporność na mrok", "%"),
    90: ("Odporność na cios krytyczny", "%"), 91: ("Odporność na przeszywający", "%"),
    # 1138-1168: mt2009 points the engine applies directly, with no APPLY id of
    # their own. See POINT_TO_APPLY below for where the 1000 offset comes from.
    1138: ("Terror", "%"), 1139: ("Regeneracja wytrzymałości", "%"),
    1140: ("Atak sztyletem przeciw potworom", ""), 1141: ("Wartość ataku przeciw potworom", ""),
    1142: ("Odporność na potwory", "‰"), 1143: ("Pochłanianie obrażeń", "%"),
    1144: ("Pochłanianie obrażeń od potworów", "%"), 1145: ("Przełamanie odporności na ogłuszenie", ""),
    1146: ("Przełamanie klątwy świątyni", ""), 1147: ("Czas trwania umiejętności", "%"),
    1148: ("Silny przeciw potworom z Doliny Orków", "%"), 1149: ("Silny przeciw Metinom", "%"),
    1150: ("Silny przeciw bossom", "%"), 1151: ("Magiczny atak przeciw potworom", "%"),
    1152: ("Przełamanie odporności na miecz", "%"), 1153: ("Przełamanie odporności na broń dwuręczną", "%"),
    1154: ("Przełamanie odporności na sztylet", "%"), 1155: ("Przełamanie odporności na dzwonek", "%"),
    1156: ("Przełamanie odporności na wachlarz", "%"), 1157: ("Przełamanie odporności na łuk", "%"),
    1158: ("Szansa na zbieranie", "%"), 1159: ("Szansa na naukę", "%"),
    1160: ("Odporność na ludzi", "%"), 1161: ("Magiczny atak", ""),
    1162: ("Szansa na podpalenie", "%"), 1163: ("Zamiana obrażeń na PE", "%"),
    1164: ("Szansa na rzadki łup", "%"), 1165: ("Magiczna wartość ataku przeciw potworom", ""),
    1166: ("Szansa na unieruchomienie", "%"), 1167: ("Atak specjalny", ""),
    1168: ("Kara za śmierć", "%"),
}

# mt2009 POINT_* -> APPLY_*. Points 138-168 have no APPLY id, so they map to
# 1000 + point and APPLY_LABELS names them there.
POINT_TO_APPLY = {
    6: 1, 8: 2, 13: 3, 15: 4, 12: 5, 14: 6, 17: 7, 19: 8, 21: 9, 32: 10, 33: 11,
    37: 12, 38: 13, 39: 14, 40: 15, 41: 16, 43: 17, 44: 18, 45: 19, 46: 20, 47: 21,
    48: 22, 63: 23, 64: 24, 65: 25, 66: 26, 67: 27, 68: 28, 69: 29, 70: 30, 71: 31,
    72: 32, 73: 33, 74: 34, 75: 35, 76: 36, 77: 37, 78: 38, 79: 39, 81: 41, 82: 42,
    83: 43, 84: 44, 85: 45, 86: 46, 87: 47, 88: 48, 89: 49, 90: 50, 28: 51, 34: 52,
    95: 53, 96: 54, 22: 55, 23: 56, 42: 57, 10: 58, 54: 59, 55: 60, 56: 61, 57: 62,
    53: 63, 114: 64, 115: 65, 116: 66, 117: 67, 118: 68, 119: 69, 120: 70, 121: 71,
    122: 72, 123: 73, 124: 74, 125: 75, 126: 76, 59: 78, 60: 79, 61: 80, 62: 81,
    128: 82, 16: 83, 130: 84, 131: 85, 132: 86, 133: 87, 134: 88, 135: 89, 136: 90,
    137: 91,
    **{point: 1000 + point for point in range(138, 169)},
}

# The client's own prototypes and icon names, extracted from the game files.
try:
    ITEM_DEFS = json.loads((config.DATA_DIR / "item_defs.json").read_text(encoding="utf-8"))
except (OSError, ValueError):
    ITEM_DEFS = {}
try:
    ITEM_ICONS = json.loads((config.DATA_DIR / "item_icons.json").read_text(encoding="utf-8"))
except (OSError, ValueError):
    ITEM_ICONS = {}


def icon_file(vnum):
    """The icon file for a vnum, falling back to its +0 base."""
    try:
        value = int(vnum)
    except (TypeError, ValueError):
        return None
    # Most upgrade series reuse one client icon for +0 through +9.
    return ITEM_ICONS.get(str(value)) or ITEM_ICONS.get(str(value - value % 10))


def apply_text(apply_type, value, mt2009):
    """One tooltip line: 'Wartość ataku +12' or 'Średnie obrażenia +5%'."""
    try:
        key = int(apply_type or 0)
    except (TypeError, ValueError):
        return ""
    if mt2009:
        key = POINT_TO_APPLY.get(key, key)
    name, suffix = APPLY_LABELS.get(key, (f"Bonus #{apply_type}", ""))
    try:
        value = int(value or 0)
    except (TypeError, ValueError):
        value = 0
    return f"{name} {value:+d}{suffix}"


def base_stats(vnum):
    """The fixed properties the in-game tooltip prints above the bonuses."""
    try:
        proto = ITEM_DEFS.get(str(int(vnum or 0)), {})
    except (TypeError, ValueError):
        return []
    if not proto:
        return []
    stats = []
    item_type = int(proto.get("type") or 0)
    level = int(proto.get("level") or 0)
    if level:
        stats.append(f"Wymagany poziom: {level}")

    def value(index):
        return int(proto.get(f"value{index}") or 0)

    if item_type == ITEM_TYPE_WEAPON:
        # Physical damage is value3/value4, magical value1/value2.
        attack_min, attack_max = value(3), value(4)
        magic_min, magic_max = value(1), value(2)
        if attack_min or attack_max:
            stats.append(f"Wartość ataku: {attack_min}–{attack_max}" if attack_min != attack_max
                         else f"Wartość ataku: {attack_max}")
        if magic_min or magic_max:
            stats.append(f"Wartość magicznego ataku: {magic_min}–{magic_max}" if magic_min != magic_max
                         else f"Wartość magicznego ataku: {magic_max}")
    elif item_type == ITEM_TYPE_ARMOR:
        defense = value(1)
        if defense:
            stats.append(f"Wartość obrony: {defense}")
    return stats


def type_label(index):
    try:
        index = int(index)
    except (TypeError, ValueError):
        return "ITEM_UNKNOWN"
    return ITEM_TYPE_NAMES[index] if 0 <= index < len(ITEM_TYPE_NAMES) else f"ITEM_TYPE_{index}"
