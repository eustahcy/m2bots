"""The handful of places where the two engine lines disagree.

Everything that differs between mt2009 and r40250 is decided here once, as SQL
fragments the queries interpolate. Nothing else in Tuike asks which engine it
is looking at.
"""
from . import config

IS_MT2009 = config.ENGINE == "mt2009"

# An item's two damage lines. mt2009 stores POINT_* ids, so the numbers the
# weapon ranking must sort by are 121 and 122 there rather than 71 and 72.
ATTR_SKILL_DAMAGE = 121 if IS_MT2009 else 71
ATTR_AVG_DAMAGE = 122 if IS_MT2009 else 72

# Which kingdom a character belongs to. player_index.empire is what the core
# reads when it decides where a bot lives; on r40250 the account carries a copy
# worth falling back to, and on mt2009 account.account has no empire column at
# all (naming it refused every INSERT on the 2.x line).
EMPIRE_EXPR = "COALESCE(NULLIF(pi.empire,0),0)" if IS_MT2009 else "COALESCE(NULLIF(pi.empire,0),a.empire,0)"

# account.login is varchar(16) on mt2009 and varchar(30) on r40250. A longer
# one is "Data too long" from the database rather than a form error.
LOGIN_MAX_LENGTH = 16 if IS_MT2009 else 30

# player.player has no bank_value on mt2009.
PLAYER_HAS_BANK_VALUE = not IS_MT2009
ACCOUNT_HAS_EMPIRE = not IS_MT2009

# On the mt2009 line a rate is not a rewritten table but six event flags the
# engine multiplies by: rows of player.quest with dwPID = 0, read by the db
# core at boot and pushed to every game core. The game container has no
# database client, so the panel writes the rows and the restart it queues is
# what makes the cores read them.
RATE_NAMES = ("exp", "drop", "yang")
MT2009_RATE_FLAGS = {
    "exp": ("mob_exp", "mob_exp_buyer"),
    "drop": ("mob_item", "mob_item_buyer"),
    "yang": ("mob_gold", "mob_gold_buyer"),
}

# ---------------------------------------------------------------------------
# What makes a character a bot, in one place instead of eight.
#
# The name used to be the test: everything this project creates is called
# bot<something>, so `name LIKE 'bot%'` found them all. Rename them - which is
# exactly what players keep asking for, human nicknames instead of botarek7 -
# and every ranking, the live map, the world statistics and the season page
# quietly stop counting them.
#
# The core never asks the name. CPlayerBotManager::LoadRegisteredBots accepts a
# character only when its account login is exactly playerbot_NNN, and renaming
# a character does not touch an account login. So that is what is asked here,
# with the old name test kept beside it so a hand-made bot on an ordinary
# account stays visible exactly as before.
# ---------------------------------------------------------------------------
def bot_predicate(alias="p"):
    """SQL that is true for a Playerbot, for whichever alias the query uses."""
    ref = f"{alias}." if alias else ""
    return (
        "(EXISTS (SELECT 1 FROM account.account ba"
        f" WHERE ba.id = {ref}account_id"
        " AND LEFT(ba.login, 10) = 'playerbot_')"
        f" OR {ref}name LIKE 'bot%%')"
    )


BOT_IS = bot_predicate("p")
BOT_IS_BARE = bot_predicate("")
