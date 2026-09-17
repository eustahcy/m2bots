"""Tuike's own tables, created on demand by whichever process gets there first.

Tuike never alters a table the game owns. Its history and settings live under
the web_tuike_ prefix so it can run beside the Seban panel without either one
overwriting the other's rows; the single shared table is the game's own queue,
player.web_admin_queue, which every panel in the stack appends to and the
in-game web_admin quest drains.
"""
from . import config

DEFAULT_SETTINGS = {
    "panel_name": "Metin2 Singleplayer",
    "stuck_minutes": "5",
    "theme": "midnight",
    "monitor_mode": "vps",
    "density": "comfortable",
    # Rankings and the dashboard carousel count Playerbots only unless the
    # operator opens them to real players too.
    "rankings_include_players": "0",
    # The pages a player may open without a password (/serwer). Off until the
    # operator publishes them.
    "public_page": "0",
    # A single-player suite needs no wizard and no passphrase - one player at
    # their own machine. An operator who publishes the panel turns auth on from
    # the settings page.
    "setup_complete": "1",
    "auth_enabled": "0",
    "auth_password_hash": "",
}

STATEMENTS = (
    f"""CREATE TABLE IF NOT EXISTS {config.SETTINGS_TABLE} (
      name VARCHAR(64) NOT NULL PRIMARY KEY,
      value VARCHAR(255) NOT NULL,
      updated_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP
    ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4""",

    f"""CREATE TABLE IF NOT EXISTS {config.ITEM_SNAPSHOT_TABLE} (
      captured_at DATETIME NOT NULL,
      vnum INT UNSIGNED NOT NULL,
      amount BIGINT UNSIGNED NOT NULL,
      PRIMARY KEY(captured_at, vnum), KEY(vnum, captured_at)
    ) ENGINE=InnoDB""",

    f"""CREATE TABLE IF NOT EXISTS {config.MAP_SNAPSHOT_TABLE} (
      captured_at DATETIME NOT NULL,
      map_index INT UNSIGNED NOT NULL,
      character_count INT UNSIGNED NOT NULL,
      PRIMARY KEY(captured_at, map_index), KEY(map_index, captured_at)
    ) ENGINE=InnoDB""",

    f"""CREATE TABLE IF NOT EXISTS {config.SYSTEM_SNAPSHOT_TABLE} (
      captured_at DATETIME NOT NULL PRIMARY KEY,
      cpu_percent DECIMAL(5,1) NOT NULL,
      ram_percent DECIMAL(5,1) NOT NULL,
      ram_used_mb INT UNSIGNED NOT NULL,
      ram_total_mb INT UNSIGNED NOT NULL,
      disk_percent DECIMAL(5,1) NOT NULL DEFAULT 0,
      disk_used_mb INT UNSIGNED NOT NULL DEFAULT 0,
      disk_total_mb INT UNSIGNED NOT NULL DEFAULT 0
    ) ENGINE=InnoDB""",

    f"""CREATE TABLE IF NOT EXISTS {config.METRIC_SNAPSHOT_TABLE} (
      captured_at DATETIME NOT NULL,
      metric VARCHAR(64) NOT NULL,
      value BIGINT NOT NULL,
      PRIMARY KEY(captured_at, metric), KEY(metric, captured_at)
    ) ENGINE=InnoDB""",

    f"""CREATE TABLE IF NOT EXISTS {config.POSITION_SNAPSHOT_TABLE} (
      captured_at DATETIME NOT NULL,
      pid INT UNSIGNED NOT NULL,
      map_index INT UNSIGNED NOT NULL,
      x INT NOT NULL, y INT NOT NULL,
      PRIMARY KEY(captured_at, pid), KEY(pid, captured_at)
    ) ENGINE=InnoDB""",

    f"""CREATE TABLE IF NOT EXISTS {config.GRANTS_TABLE} (
      id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
      batch CHAR(32) NOT NULL,
      player_id INT UNSIGNED NOT NULL,
      player_name VARCHAR(24) NOT NULL,
      vnum INT UNSIGNED NOT NULL,
      quantity INT UNSIGNED NOT NULL DEFAULT 1,
      criteria VARCHAR(1000) NOT NULL DEFAULT '{{}}',
      only_missing TINYINT(1) NOT NULL DEFAULT 1,
      status VARCHAR(24) NOT NULL DEFAULT 'waiting',
      queue_id INT NULL,
      created DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
      updated DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
      next_try DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
      UNIQUE KEY batch_player(batch, player_id),
      KEY pending(status, next_try),
      KEY recipient(player_id, vnum, status)
    ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4""",

    f"""CREATE TABLE IF NOT EXISTS {config.AUDIT_TABLE} (
      id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
      at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
      action VARCHAR(32) NOT NULL,
      target VARCHAR(64) NOT NULL DEFAULT '',
      detail VARCHAR(255) NOT NULL DEFAULT '',
      KEY recent(at), KEY subject(target, at)
    ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4""",

    # The game's own queue. Tuike creates it only if nothing else has: the
    # in-game quest reads it, and a stack where no panel has ever run would
    # otherwise have nowhere to put a grant.
    f"""CREATE TABLE IF NOT EXISTS {config.GAME_QUEUE_TABLE} (
      id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
      player_name VARCHAR(24) NOT NULL,
      cmd VARCHAR(32) NOT NULL,
      arg1 VARCHAR(255) NOT NULL DEFAULT '',
      arg2 VARCHAR(255) NOT NULL DEFAULT '',
      status VARCHAR(24) NOT NULL DEFAULT 'pending',
      created DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
      updated DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
      KEY pending(status, created), KEY player_status(player_name, status)
    ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4""",
)


def ensure(cursor):
    """Create everything Tuike owns, then seed the settings it needs a row for."""
    for statement in STATEMENTS:
        cursor.execute(statement)
    cursor.executemany(
        f"INSERT IGNORE INTO {config.SETTINGS_TABLE} (name, value) VALUES (%s, %s)",
        tuple(DEFAULT_SETTINGS.items()),
    )


def ensure_once(connection):
    with connection.cursor() as cursor:
        ensure(cursor)
