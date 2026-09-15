"""Everything Tuike reads from its environment, resolved once at import.

Tuike runs beside the older Seban panel and the classic Tieru panel, so every
name here is prefixed TUIKE_ and every default is safe for a stack that was
configured before Tuike existed. Where docker-compose already publishes a
generic name the whole stack shares (DB_HOST, PLAYERBOTS_*), that name is read
directly rather than duplicated under a second spelling.
"""
import os
from pathlib import Path

PACKAGE_DIR = Path(__file__).resolve().parent
PROJECT_DIR = PACKAGE_DIR.parent
STATIC_DIR = PACKAGE_DIR / "static"
DATA_DIR = STATIC_DIR / "data"


def _env(name, default=""):
    return os.environ.get(name, default).strip()


def _int(name, default):
    try:
        return int(_env(name) or default)
    except ValueError:
        return default


# --- identity ---------------------------------------------------------------
PANEL_CODENAME = "Tuike"
PANEL_TAGLINE = "Tuike Control Center"
# The port is a fact about this panel, not a preference: the Dockerfile
# exposes it, gunicorn binds it and the compose file publishes it.
PANEL_PORT = _int("TUIKE_PANEL_PORT", 7799)

try:
    PANEL_VERSION = _env("TUIKE_PANEL_VERSION") or (PROJECT_DIR / "VERSION").read_text(encoding="utf-8").strip()
except OSError:
    PANEL_VERSION = "dev"

# --- session ----------------------------------------------------------------
# Cookies are scoped to the host and not the port, so a panel sharing an
# address with Seban (:7790) or Tieru (:7788) must carry its own cookie name or
# the three log each other out.
SESSION_SECRET = _env("TUIKE_SESSION_SECRET") or "change-this-before-public-use"
SESSION_COOKIE_NAME = _env("TUIKE_SESSION_COOKIE_NAME") or "tuike_session"
SESSION_ADMIN_KEY = "tuike_admin"

# --- database ---------------------------------------------------------------
DB_HOST = _env("DB_HOST") or "mariadb"
DB_PORT = _int("DB_PORT", 3306)
DB_USER = _env("DB_USER") or "metin2"
DB_PASSWORD = os.environ.get("DB_PASSWORD", "")

# Tuike keeps its own history and settings so that it can run beside Seban
# without either panel overwriting the other's rows. The queue in the middle
# (player.web_admin_queue) is the game's, and is shared on purpose.
TABLE_PREFIX = "web_tuike_"
SETTINGS_TABLE = f"player.{TABLE_PREFIX}settings"
ITEM_SNAPSHOT_TABLE = f"player.{TABLE_PREFIX}item_snapshot"
MAP_SNAPSHOT_TABLE = f"player.{TABLE_PREFIX}map_snapshot"
SYSTEM_SNAPSHOT_TABLE = f"player.{TABLE_PREFIX}system_snapshot"
METRIC_SNAPSHOT_TABLE = f"player.{TABLE_PREFIX}metric_snapshot"
POSITION_SNAPSHOT_TABLE = f"player.{TABLE_PREFIX}bot_position_snapshot"
GRANTS_TABLE = f"player.{TABLE_PREFIX}grants"
# Written by the game's own web_admin quest; every panel in the stack appends
# to it and the quest drains it. Never prefixed.
GAME_QUEUE_TABLE = "player.web_admin_queue"
RATES_TABLE = "player.web_admin_rates"
# MySQL advisory lock, so Tuike's grant worker never blocks Seban's.
GRANTS_LOCK = "tuike_item_grants"

# --- the world this panel looks at ------------------------------------------
# mt2009 keeps an item's bonus lines as POINT_* numbers, has no empire column
# on account.account and no bank_value on player.player. See engine.py.
ENGINE = (_env("PLAYERBOTS_ENGINE") or "r40250").lower()
GAME_HOST = _env("PLAYERBOTS_GAME_HOST") or "metin2-game"
GAME_LOGIN_PORT = _int("PLAYERBOTS_LOGIN_PORT", 11000)
GAME_WORLD_PORT = _int("PLAYERBOTS_WORLD_PORT", 13000)
STATUS_GLOB = _env("PLAYERBOTS_STATUS_GLOB") or "/opt/metin2/var/channel1/*/playerbot_status.tsv"
# A status file older than this is a core that stopped writing, not a world
# standing still.
STATUS_MAX_AGE_SECONDS = 25
PLAYERBOTS_VERSION = _env("PLAYERBOTS_VERSION") or "nieustawiona"

# --- shared spools ----------------------------------------------------------
# The same two directories the classic panel and the game container share. The
# protocol is a few key=value files; Tuike writes requests stamped "tuike-" so
# a helper's log says which panel asked.
RATES_SPOOL = Path(_env("TUIKE_RATES_SPOOL") or "/opt/m2spool")
UPDATE_SPOOL = Path(_env("TUIKE_UPDATE_SPOOL") or "/opt/m2update")
REQUEST_PREFIX = "tuike-"

# --- links out --------------------------------------------------------------
# Opened by the visitor's browser, so a reachable address and not a container
# name. Skill icons are served from here too.
TIERU_PANEL_URL = _env("TIERU_PANEL_URL") or "http://127.0.0.1:7788"
SEBAN_PANEL_URL = _env("TUIKE_SEBAN_PANEL_URL")
ITEMSHOP_URL = _env("ITEMSHOP_URL") or "http://127.0.0.1:7791"
# The VPS control panel (start/stop/restart/compile) is a separate tool this
# stack may or may not run - empty means "not deployed here", and the card
# that would link to it just does not show.
VPS_PANEL_URL = _env("VPS_PANEL_URL")

# --- background work --------------------------------------------------------
COLLECTOR_INTERVAL = max(30, _int("TUIKE_COLLECTOR_INTERVAL", 300))
GRANTS_TICK_SECONDS = max(1, _int("TUIKE_GRANTS_TICK", 3))

# --- the one outbound call --------------------------------------------------
# Once every 15 minutes the dashboard asks GitHub for the newest published
# Playerbots release. Set TUIKE_UPDATE_CHECK=0 and Tuike contacts nothing at
# all; nothing else about the panel changes.
UPDATE_CHECK = _env("TUIKE_UPDATE_CHECK", "1") != "0"
RELEASE_URL = "https://api.github.com/repos/TieruYT/metin2-playerbots/releases/latest"
RELEASE_CACHE_SECONDS = 900
