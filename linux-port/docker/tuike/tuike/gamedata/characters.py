"""Classes, kingdoms, honour, skills and experience - the character itself."""
import json

from .. import config

JOB_NAMES = ("Wojownik", "Ninja", "Sura", "Szaman")

# Race IDs live in player.player.job. 0-3 keep the classic class/gender pair;
# 4-7 are the alternate client portraits and character models.
CLASS_PROFILES = {
    0: {"name": "Wojownik", "gender": "Mężczyzna", "portrait": "warrior_m.bmp"},
    4: {"name": "Wojownik", "gender": "Kobieta", "portrait": "warrior_w.bmp"},
    1: {"name": "Ninja", "gender": "Kobieta", "portrait": "assassin_w.bmp"},
    5: {"name": "Ninja", "gender": "Mężczyzna", "portrait": "assassin_m.bmp"},
    2: {"name": "Sura", "gender": "Mężczyzna", "portrait": "sura_m.bmp"},
    6: {"name": "Sura", "gender": "Kobieta", "portrait": "sura_w.bmp"},
    3: {"name": "Szaman", "gender": "Kobieta", "portrait": "shaman_w.bmp"},
    7: {"name": "Szaman", "gender": "Mężczyzna", "portrait": "shaman_m.bmp"},
}
# The order live-map JavaScript indexes by raw job id.
PORTRAIT_BY_JOB = tuple(CLASS_PROFILES[job]["portrait"] for job in sorted(CLASS_PROFILES))

EMPIRES = {
    1: {"name": "Shinsoo", "flag": "shinsoo.png"},
    2: {"name": "Chunjo", "flag": "chunjo.png"},
    3: {"name": "Jinno", "flag": "jinno.png"},
}

# Creating a game master: the starting stats, the starting place, and which
# race id a class/gender pair maps to.
GM_JOB_OPTIONS = ((0, "Wojownik"), (1, "Ninja"), (2, "Sura"), (3, "Szaman"))
GM_GENDER_OPTIONS = (("classic", "Klasyczna dla klasy"), ("male", "Mężczyzna"), ("female", "Kobieta"))
GM_RACE_BY_CLASS_GENDER = {
    (0, "classic"): 0, (1, "classic"): 1, (2, "classic"): 2, (3, "classic"): 3,
    (0, "male"): 0, (0, "female"): 4,
    (1, "male"): 5, (1, "female"): 1,
    (2, "male"): 2, (2, "female"): 6,
    (3, "male"): 7, (3, "female"): 3,
}
# (st, ht, dx, iq, hp, mp) per class, and (x, y, map_index) per kingdom.
GM_JOB_STARTS = {0: (6, 4, 3, 3, 600, 200), 1: (4, 3, 6, 3, 650, 200),
                 2: (5, 3, 3, 6, 650, 200), 3: (3, 5, 3, 5, 700, 200)}
GM_EMPIRE_STARTS = {1: (469300, 964200, 1), 2: (55700, 157900, 21), 3: (969600, 278400, 41)}
# A plain nickname, or one bracketed prefix in front of one: [GM]Seban.
GM_NAME_PATTERN = r"(?:[A-Za-z0-9_]{2,24}|\[[A-Za-z0-9_]{1,6}\][A-Za-z0-9_]{2,16})"
AUTHORITIES = ("PLAYER", "LOW_WIZARD", "GOD", "HIGH_WIZARD", "IMPLEMENTOR")

# Exact vnum/name pairs as the client's icon pack numbers them. Keyed by
# (class, skill group).
SKILLS = {
    (0, 1): ((1, "Trzystronne Cięcie"), (2, "Wir Miecza"), (3, "Berserk"), (4, "Aura Miecza"), (5, "Szarża")),
    (0, 2): ((16, "Duchowe Uderzenie"), (17, "Tąpnięcie"), (18, "Uderzenie Miecza"), (19, "Silne Ciało"), (20, "Walnięcie")),
    (1, 1): ((31, "Zasadzka"), (32, "Szybki Atak"), (33, "Wirujący Sztylet"), (34, "Krycie się"), (35, "Trująca Chmura")),
    (1, 2): ((46, "Powtarzalny Strzał"), (47, "Deszcz Strzał"), (48, "Ognista Strzała"), (49, "Bezszelestny Chód"), (50, "Trująca Strzała")),
    (2, 1): ((61, "Uderzenie Palcem"), (62, "Smoczy Wir"), (63, "Czarowane Ostrze"), (64, "Strach"), (65, "Czarowana Zbroja"), (66, "Rozproszenie Magii")),
    (2, 2): ((76, "Mroczne Uderzenie"), (77, "Ogniste Uderzenie"), (78, "Ognisty Duch"), (79, "Mroczna Ochrona"), (80, "Duchowy Cios"), (81, "Mroczna Sfera")),
    (3, 1): ((91, "Latający Talizman"), (92, "Strzelający Smok"), (93, "Smoczy Skowyt"), (94, "Błogosławieństwo"), (95, "Odbicie"), (96, "Pomoc Smoka")),
    (3, 2): ((106, "Błyskawiczny Rzut"), (107, "Przywołanie Błyskawicy"), (108, "Burzowy Szpon"), (109, "Leczenie"), (110, "Zwinność"), (111, "Zwiększenie Ataku")),
}
SKILL_NAMES = {vnum: name for skill_set in SKILLS.values() for vnum, name in skill_set}
# player.skill_level is a blob of six bytes per skill vnum: the first is the
# mastery type, the second the level.
SKILL_STRIDE = 6
# Riding is skill 130, so its level byte is at 130 * 6 + 1 = 781 (SUBSTRING is
# one-based, hence 782 in SQL).
RIDING_SKILL_OFFSET = 130 * SKILL_STRIDE + 1

# EWearPositions from Server/common/length.h. The database stores these offsets
# directly in the EQUIPMENT window, not the client's offset + 90.
EQUIPMENT_SLOTS = {
    0: "body", 1: "head", 2: "foots", 3: "wrist", 4: "weapon",
    5: "neck", 6: "ear", 7: "unique1", 8: "unique2", 9: "arrow",
    10: "shield", 23: "belt",
}
# The order the profile draws them in, so the template carries no list.
EQUIPMENT_LAYOUT = ("weapon", "body", "head", "foots", "wrist", "neck",
                    "ear", "shield", "unique1", "unique2", "arrow", "belt")
# Client uiinventory.py: page I begins at slot 0 and page II at slot 45.
INVENTORY_PAGE_SIZE = 45
INVENTORY_COLUMNS = 5

# The alignment bands, in tenths of a point as the core stores them.
HONOUR_BANDS = (
    (12000, "Rycerski", "knightly"), (8000, "Szlachetny", "noble"),
    (4000, "Dobry", "good"), (1000, "Przyjazny", "friendly"),
    (0, "Neutralny", "neutral"), (-3999, "Agresywny", "aggressive"),
    (-7999, "Nieuczciwy", "dishonest"), (-11999, "Złośliwy", "malicious"),
    (-20000, "Okrutny", "cruel"),
)

try:
    EXP_LEVELS = json.loads((config.DATA_DIR / "exp_levels.json").read_text(encoding="utf-8"))
except (OSError, ValueError):
    EXP_LEVELS = [0]


def class_profile(job):
    try:
        return CLASS_PROFILES.get(int(job), CLASS_PROFILES[0])
    except (TypeError, ValueError):
        return CLASS_PROFILES[0]


def job_name(job):
    return class_profile(job)["name"]


def empire_info(empire):
    try:
        return EMPIRES.get(int(empire), {"name": "—", "flag": ""})
    except (TypeError, ValueError):
        return {"name": "—", "flag": ""}


def skill_rank(master_type, level):
    """The in-game label: a number, then M1-M10, G1-G10 and finally P."""
    master_type, level = int(master_type or 0), int(level or 0)
    if master_type >= 3 or level >= 40:
        return "P"
    if master_type == 2 or level >= 30:
        return f"G{max(1, level - 29)}"
    if master_type == 1 or level >= 20:
        return f"M{max(1, level - 19)}"
    return str(level)


def skill_tier(rank):
    """3 for Perfect, 2 for Grand, 1 for Master, 0 for an ordinary level."""
    if rank == "P":
        return 3
    if rank.startswith("G"):
        return 2
    if rank.startswith("M"):
        return 1
    return 0


def parse_skills(raw, job, group):
    """Read the skill blob into the six skills of this class and group."""
    if isinstance(raw, memoryview):
        raw = raw.tobytes()
    if isinstance(raw, str):
        raw = raw.encode("latin1", "ignore")
    raw = raw or b""
    result = []
    for vnum, name in SKILLS.get((int(job or 0) % 4, int(group or 0)), ()):
        offset = vnum * SKILL_STRIDE
        master = raw[offset] if offset < len(raw) else 0
        level = raw[offset + 1] if offset + 1 < len(raw) else 0
        if not level:
            continue
        # The icon pack has master artwork in *_m.png and uses it for every
        # mastered stage (M, G and P); there are no *_p.png files.
        result.append({
            "vnum": vnum, "name": name, "level": level, "master_type": master,
            "rank": skill_rank(master, level),
            "icon_suffix": "_m" if master >= 1 or level >= 20 else "",
        })
    return result


def best_skill(skills):
    """The skill a ranking should judge a character by."""
    return max(skills, key=lambda skill: (skill_tier(skill["rank"]), skill["level"]), default=None)


def experience_progress(level, exp):
    """How far into its level a character is, against the client's own table."""
    level, exp = int(level or 0), max(0, int(exp or 0))
    required = int(EXP_LEVELS[min(max(level, 0), len(EXP_LEVELS) - 1)] or 0)
    return {
        "current": exp,
        "required": required,
        "percent": min(100, round(exp * 100 / required, 1)) if required else 100,
    }


def honour_rank(value):
    """The core stores alignment in tenths; return the in-game value and class."""
    points = int(float(value or 0) / 10)
    for threshold, title, css in HONOUR_BANDS:
        if points >= threshold:
            return {"points": points, "title": title, "css": css}
    return {"points": points, "title": "Okrutny", "css": "cruel"}
