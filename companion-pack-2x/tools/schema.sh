#!/bin/sh
set -eu
# Passed on stdin to sh -s, avoiding Windows native argument quoting of secrets.
# All credentials remain inside the container.
app_query() {
  MYSQL_PWD="${M2_DB_PASSWORD:-}" mariadb --protocol=tcp --host=127.0.0.1 --user="${M2_DB_USER:-metin2}" --batch --skip-column-names
}
if app_query >/dev/null 2>&1 <<'SQL'
SELECT owner_pid,companion_pid,enabled FROM common.playerbot_companion LIMIT 0;
SQL
then
  echo 'Existing companion table verified with game database account.'
  exit 0
fi
# A fresh installation may require root to create the table. Never reset users,
# passwords or volumes to make this work.
if ! MYSQL_PWD="${MARIADB_ROOT_PASSWORD:-}" mariadb --user=root --batch <<'SQL'
CREATE TABLE IF NOT EXISTS common.playerbot_companion (owner_pid INT UNSIGNED NOT NULL PRIMARY KEY, companion_pid INT UNSIGNED NOT NULL UNIQUE, enabled TINYINT UNSIGNED NOT NULL DEFAULT 1, created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP);
SELECT owner_pid,companion_pid,enabled FROM common.playerbot_companion LIMIT 0;
SQL
then
  echo 'Cannot verify the companion table with the game account or initialize it with root. Check the configured database credentials; no password or account was changed.' >&2
  exit 1
fi
if ! app_query >/dev/null <<'SQL'
SELECT owner_pid,companion_pid,enabled FROM common.playerbot_companion LIMIT 0;
SQL
then
  echo 'The game database account cannot read the companion table. Resolve its existing access before rebuilding.' >&2
  exit 1
fi
