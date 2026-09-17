"""The maps this Playerbots world actually runs, and where they are.

Village names come from the engine's own quests: new_quest_lv52 reads the first
villages as Yongan / Joan / Pyongmoo by kingdom, and new_quest_lv7 names the
second ones Jayang, Bokjung and Bakra.
"""

MAP_NAMES = {
    1: "Shinsoo M1 — Yongan", 3: "Shinsoo M2 — Jayang", 4: "Ziemia Klanu Shinsoo",
    5: "Loch Małp Shinsoo", 44: "Ziemia Klanu Jinno", 45: "Loch Małp Jinno",
    21: "Chunjo M1 — Joan", 23: "Chunjo M2 — Bokjung",
    24: "Ziemia Klanu Chunjo", 25: "Łatwy Loch Małp",
    41: "Jinno M1 — Pyongmoo", 43: "Jinno M2 — Bakra",
    61: "Góra Sohan", 63: "Pustynia Yongbi", 64: "Dolina Orków", 104: "Loch Pająków V1",
    65: "Świątynia Hwang", 71: "Loch Pająków V2",
    108: "Loch Małp Normalny", 109: "Loch Małp Trudny",
    67: "Las", 68: "Czerwony Las", 66: "Wieża Demonów",
}

# (origin x, origin y, width, height) in world units. A bot's screen position is
# ((x - origin_x) / width, (y - origin_y) / height), and the picture's aspect
# ratio is width / height - so the drawing never needs a second table.
MAP_BOUNDS = {
    1: (409600, 896000, 102400, 128000), 3: (307200, 819200, 102400, 102400),
    4: (128000, 0, 51200, 51200), 5: (768000, 435200, 76800, 76800),
    41: (921600, 204800, 102400, 128000), 43: (819200, 204800, 102400, 102400),
    44: (230400, 0, 51200, 51200), 45: (921600, 435200, 76800, 76800),
    21: (0, 102400, 102400, 128000), 23: (102400, 204800, 102400, 102400),
    24: (179200, 0, 51200, 51200), 25: (844800, 435200, 76800, 76800),
    61: (358400, 153600, 153600, 153600), 63: (204800, 486400, 153600, 153600),
    64: (256000, 665600, 153600, 153600), 104: (51200, 486400, 76800, 76800),
    65: (537600, 51200, 102400, 102400), 71: (665600, 435200, 102400, 102400),
    108: (128000, 640000, 76800, 76800), 109: (128000, 716800, 76800, 76800),
    # BasePosition and MapSize x 25600 from each map's own Setting.txt - the
    # formula that reproduces Chunjo M1's (0, 102400, 102400, 128000) above.
    # No picture ships for these three, so they are tracked (map history, the
    # heat-map buckets) but never drawn; the Demon Tower is a private dungeon
    # instance anyway, where live dots would mean nothing.
    67: (281600, 0, 51200, 51200), 68: (1049600, 0, 76800, 76800),
    66: (128000, 793600, 76800, 76800),
}

# A map is drawable only where there is a picture of it. Spider Dungeon V2
# shares V1's artwork - same layout, different difficulty.
MAP_BACKGROUNDS = {
    21: "chunjo-m1.png", 23: "chunjo-m2.png", 24: "guild-map-02.png",
    25: "easy-monkey.png", 61: "mount-sohan.png", 63: "yongbi-desert.png",
    64: "orc-valley.png", 65: "hwang-temple.png", 71: "spider-dungeon-v1.png",
    104: "spider-dungeon-v1.png", 108: "medium-monkey.webp", 109: "hard-monkey.webp",
}

TRACKED_MAP_OPTIONS = tuple((index, MAP_NAMES[index]) for index in MAP_BOUNDS)
# The live map and both heat maps offer exactly the maps they can draw, in the
# order a player meets them.
DRAWABLE_MAP_ORDER = (21, 23, 24, 25, 108, 109, 61, 63, 64, 65, 104, 71)
DRAWABLE_MAP_OPTIONS = tuple(
    (index, MAP_NAMES[index]) for index in DRAWABLE_MAP_ORDER if index in MAP_BACKGROUNDS
)

MAP_RESPAWN_OPTIONS = (
    (1, "Shinsoo M1 — Yongan"), (3, "Shinsoo M2 — Jayang"), (21, "Chunjo M1 — Joan"),
    (23, "Chunjo M2 — Bokjung"), (41, "Jinno M1 — Pyongmoo"), (43, "Jinno M2 — Bakra"),
    (4, "Ziemia Klanu Shinsoo"), (24, "Ziemia Klanu Chunjo"), (44, "Ziemia Klanu Jinno"),
    (5, "Loch Małp Shinsoo"), (45, "Loch Małp Jinno"),
    (25, "Łatwy Loch Małp"), (61, "Góra Sohan"), (63, "Pustynia Yongbi"), (64, "Dolina Orków"),
    (104, "Loch Pająków V1"), (71, "Loch Pająków V2"),
    (108, "Loch Małp Normalny"), (109, "Loch Małp Trudny"),
)
# Monkey Dungeons and both Spider Dungeons ship no stone.txt, so only their mob
# respawns can be configured. The explicit allowlist also protects the helper
# from being asked for a file that does not exist.
MAP_STONE_RESPAWN_IDS = frozenset(
    index for index, _name in MAP_RESPAWN_OPTIONS if index not in {5, 25, 45, 71, 104, 108, 109}
)


def map_name(index):
    """Name only maps this world runs; anything else says so plainly."""
    try:
        index = int(index or 0)
    except (TypeError, ValueError):
        return "Poza aktywnym światem"
    return MAP_NAMES.get(index, f"Poza aktywnym światem (mapa #{index})")


def map_aspect(index):
    """width / height of the map picture, as a CSS aspect-ratio string."""
    bound = MAP_BOUNDS.get(int(index or 0))
    return f"{bound[2]} / {bound[3]}" if bound else "1 / 1"


def map_at(x, y):
    """Which map a logged world coordinate belongs to, or None."""
    for index, (origin_x, origin_y, width, height) in MAP_BOUNDS.items():
        if origin_x <= x < origin_x + width and origin_y <= y < origin_y + height:
            return index
    return None
