"""What the panel may ask the game to do to one character.

These are the commands the in-game web_admin quest understands. The presets
exist so that the common cases are one click rather than a coordinate the
operator has to look up.
"""

# (emoji, name, "x y"). Coordinates are world units, the same ones the quest
# passes to pc.warp.
WARP_LOCATIONS = (
    ("🏯", "Miasto Shinsoo", "474300 954800"),
    ("🏮", "Miasto Chunjo", "65900 155600"),
    ("⛩️", "Miasto Jinno", "963500 279700"),
    ("🏝️", "Bokjung (M2)", "145500 240000"),
    ("⚔️", "Dolina Orków", "270400 739900"),
    ("🏜️", "Pustynia Yongbi", "221900 502700"),
    ("❄️", "Góra Sohan", "375200 174900"),
    ("🔥", "Ognista Ziemia", "597800 622200"),
)

# (emoji, name, percent). The quest applies it as an affect for SPEED_DURATION.
SPEED_PRESETS = (
    ("🚶", "Normalna (zdejmij)", 0),
    ("🏃", "Szybka (+30%)", 30),
    ("💨", "Bardzo szybka (+60%)", 60),
    ("⚡", "Błyskawiczna (+100%)", 100),
)
SPEED_DURATION_SECONDS = 3600

GOLD_PRESETS = (
    ("💰 1 milion", 1_000_000),
    ("💰 10 milionów", 10_000_000),
    ("💰 100 milionów", 100_000_000),
    ("👑 1 miliard", 1_000_000_000),
)

# Ready-made multipliers, aimed at a quiet world where questing is the point.
# (name, description, exp, drop, yang)
RATE_PRESETS = (
    ("Zwykłe", "tak, jak zaprojektowano grę", 100, 100, 100),
    ("Spokojne", "łańcuch questów niesie, gra zostaje grą", 300, 200, 200),
    ("Szybkie", "dla kogoś, kto zna grę i chce zobaczyć koniec", 1000, 500, 500),
)

# The game master ranks, in the order the game grades them. The strings are not
# ours to choose: common.gmlist.mAuthority is an ENUM, and a value the server
# does not recognise is dropped in silence when it reads the list - which looks
# exactly like the rank being granted and then not working. WIZARD exists in
# the server's C++ but not in the ENUM, so it cannot be offered.
#
# LOW_WIZARD is first because it is the one to hand out: it carries the
# everyday commands and not the ones that rewrite the world.
GM_RANKS = (
    ("LOW_WIZARD", "Pomocnik", "codzienne komendy"),
    ("GOD", "Game master", "prawie wszystko"),
    ("HIGH_WIZARD", "Starszy game master", "wszystko poza własnością serwera"),
    ("IMPLEMENTOR", "Właściciel", "każda istniejąca komenda"),
)
GM_RANK_SET = frozenset(rank for rank, _name, _note in GM_RANKS)
GM_RANK_NAMES = {rank: name for rank, name, _note in GM_RANKS}

# The languages the server files ship in. The panel only asks; m2-lang in the
# game container swaps the four files and restarts the cores, because they read
# them while they boot and never again.
GAME_LANGUAGES = (
    ("en", "English"), ("de", "Deutsch"), ("tr", "Türkçe"),
    ("fr", "Français"), ("es", "Español"), ("it", "Italiano"),
    ("pt", "Português"), ("nl", "Nederlands"), ("pl", "Polski"),
    ("ro", "Română"), ("hu", "Magyar"), ("cz", "Čeština"),
    ("gr", "Ελληνικά"), ("dk", "Dansk"), ("ru", "Русский"),
)
GAME_LANGUAGE_NAMES = dict(GAME_LANGUAGES)
DEFAULT_GAME_LANGUAGE = "en"


def gm_rank_label(rank):
    return GM_RANK_NAMES.get(rank, rank or "Zwykły gracz")
