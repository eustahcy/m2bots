"""One connection helper, used by the web app and by both workers."""
import re

import pymysql

from . import config

# MyISAM does not survive an unclean stop, and this panel reads log.log - the
# busiest table in the world - on its front page. When that table is marked
# crashed every query against it raises, Flask shows its own "Internal Server
# Error", and the screenshot that reaches the operator says nothing about which
# table or what to do. These are the errno values MySQL uses to say so:
# 1194 "is marked as crashed and should be repaired", 1195 and 144 "last repair
# failed", 145 the same on older servers.
CRASHED_TABLE_ERRNOS = (144, 145, 1194, 1195)


def connect():
    return pymysql.connect(
        host=config.DB_HOST,
        port=config.DB_PORT,
        user=config.DB_USER,
        password=config.DB_PASSWORD,
        charset="utf8mb4",
        cursorclass=pymysql.cursors.DictCursor,
        autocommit=True,
    )


def rows(sql, params=()):
    """Every row of a read-only query, as dictionaries."""
    with connect() as connection:
        with connection.cursor() as cursor:
            cursor.execute(sql, params)
            return cursor.fetchall()


def one(sql, params=()):
    """The first row, or an empty dict - never None, so callers can .get()."""
    result = rows(sql, params)
    return result[0] if result else {}


def scalar(sql, params=(), default=None):
    row = one(sql, params)
    return next(iter(row.values()), default) if row else default


def execute(sql, params=()):
    """A single write outside a transaction; returns the affected row count."""
    with connect() as connection:
        with connection.cursor() as cursor:
            cursor.execute(sql, params)
            return cursor.rowcount


def crashed_table_name(error):
    """The table a crashed-table OperationalError is complaining about, or ''."""
    message = str(error.args[1]) if len(error.args) > 1 else str(error)
    match = re.search(r"Table '([^']+)'", message)
    return match.group(1).replace("./", "").replace("/", ".") if match else ""


def is_crashed_table(error):
    return bool(error.args) and error.args[0] in CRASHED_TABLE_ERRNOS
