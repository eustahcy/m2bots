"""Panel configuration the operator can change, kept in the database.

None of this is a secret from the environment: the panel name, the look, how
long a bot may stand still before it is flagged, and - when the panel is
published - the hash of its passphrase. Keeping it in the database rather than
a file means a recreated container keeps its settings.
"""
import pymysql
from werkzeug.security import check_password_hash, generate_password_hash

from . import config, db
from .schema import DEFAULT_SETTINGS, ensure
from .text import clamp

THEMES = (
    ("midnight", "Midnight — granat i błękit"),
    ("ember", "Ember — grafit i bursztyn"),
    ("forest", "Forest — zieleń i mech"),
    ("dawn", "Dawn — jasny, dzienny"),
)
DENSITIES = (("comfortable", "Swobodna"), ("compact", "Zwarta"))
MONITOR_MODES = (("vps", "VPS / host"), ("docker", "Docker / lokalnie"))

THEME_IDS = frozenset(key for key, _label in THEMES)
DENSITY_IDS = frozenset(key for key, _label in DENSITIES)
MONITOR_IDS = frozenset(key for key, _label in MONITOR_MODES)

PANEL_NAME_MAX = 48
STUCK_MINUTES_RANGE = (1, 120)
PASSWORD_MIN_LENGTH = 8


def read():
    """Every setting, with a default for anything the table does not hold."""
    values = dict(DEFAULT_SETTINGS)
    try:
        for row in db.rows(f"SELECT name, value FROM {config.SETTINGS_TABLE}"):
            if row["name"] in values:
                values[row["name"]] = str(row["value"])
    except pymysql.MySQLError:
        # The very first request can arrive before the collector has created
        # the table. Defaults are a working panel, not an error page.
        pass
    return values


def write(values):
    """Persist a partial set of settings, creating the table if it is new."""
    if not values:
        return
    with db.connect() as connection:
        with connection.cursor() as cursor:
            ensure(cursor)
            cursor.executemany(
                f"INSERT INTO {config.SETTINGS_TABLE} (name, value) VALUES (%s, %s) "
                "ON DUPLICATE KEY UPDATE value=VALUES(value)",
                tuple((name, str(value)) for name, value in values.items()),
            )


def stamp(key, value):
    """One settings row used as a heartbeat or a last-error note."""
    db.execute(
        f"INSERT INTO {config.SETTINGS_TABLE} (name, value) VALUES (%s, %s) "
        "ON DUPLICATE KEY UPDATE value=VALUES(value)",
        (key, str(value)[:255]),
    )


def stuck_minutes(current=None):
    current = current or read()
    return clamp(current.get("stuck_minutes", "5"), *STUCK_MINUTES_RANGE, default=5)


def validate_display(form):
    """Check the appearance half of the settings form. Returns (values, error)."""
    name = (form.get("panel_name") or "").strip()[:PANEL_NAME_MAX]
    if not name:
        return None, "Nazwa panelu nie może być pusta."
    theme = form.get("theme", "midnight")
    density = form.get("density", "comfortable")
    monitor_mode = form.get("monitor_mode", "vps")
    if theme not in THEME_IDS:
        return None, "Wybierz jeden z dostępnych motywów."
    if density not in DENSITY_IDS:
        return None, "Wybierz jedno z dostępnych zagęszczeń widoku."
    if monitor_mode not in MONITOR_IDS:
        return None, "Wybierz prawidłowe źródło monitoringu."
    return {
        "panel_name": name,
        "stuck_minutes": str(clamp(form.get("stuck_minutes", "5"), *STUCK_MINUTES_RANGE, default=5)),
        "theme": theme,
        "density": density,
        "monitor_mode": monitor_mode,
    }, None


def resolve_password(form, current):
    """Work out the new password hash from the form. Returns (hash, error).

    An empty password field with protection already on means "keep the hash you
    have" - otherwise saving any other setting would silently clear it.
    """
    enabled = form.get("auth_enabled") == "1"
    password = form.get("panel_password", "")
    if not enabled:
        return "", None
    if password and len(password) < PASSWORD_MIN_LENGTH:
        return None, f"Nowe hasło musi mieć co najmniej {PASSWORD_MIN_LENGTH} znaków."
    existing = current.get("auth_password_hash", "")
    password_hash = generate_password_hash(password) if password else existing
    if not password_hash:
        return None, "Aby włączyć ochronę, ustaw hasło panelu."
    return password_hash, None


def password_matches(current, password):
    password_hash = current.get("auth_password_hash", "")
    return bool(password_hash) and check_password_hash(password_hash, password)
