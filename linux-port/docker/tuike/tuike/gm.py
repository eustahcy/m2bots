"""Granting and taking away the in-game admin commands.

Unlike everything else on the character page this does not go through the
helper quest and does not need the player online: the game reads its list of
game masters from common.gmlist, so the row IS the grant. What no row can do is
make a running server notice, which is what the reload below is for.

Two details of the server's own matching decide the shape of the row:

  * The list is keyed on the CHARACTER name, and on this build the account must
    match as well. Both go in the row; one without the other grants nothing,
    silently.

  * mServerIP stays 'ALL'. The db core asks for rows matching 'ALL' or its own
    address. At boot its own address is right; on a reload it is garbage,
    because the game core sends the reload packet with no body and the db core
    reads the address out of it anyway - servers have been seen asking for a
    fragment of a character name left in the buffer. A row pinned to an address
    would therefore be found at boot and lost at the next reload, or the other
    way round. 'ALL' matches either way.
"""
import os
import time

import pymysql

from . import config, db, engine
from .gamedata.actions import GM_RANK_SET, gm_rank_label

# How long to wait for an online owner to run the reload for us.
RELOAD_WAIT_SECONDS = 6.0
RELOAD_POLL_SECONDS = 0.6
RELOAD_ASK_LIMIT = 8


def rank_of(name):
    """The rank this character holds, or None for an ordinary player.

    Also None when the table cannot be read: the card then just offers the
    choice, which is a better failure than claiming somebody is nobody.
    """
    try:
        row = db.one("SELECT mAuthority AS rank FROM common.gmlist WHERE mName = %s LIMIT 1",
                     (name,))
    except pymysql.MySQLError:
        return None
    if not row:
        return None
    # PLAYER is in the ENUM and means "no rank": the server skips those rows
    # when it reads the list. Anything else unrecognised is treated the same.
    rank = (row["rank"] or "").strip()
    return rank if rank in GM_RANK_SET else None


def _ask_game_container():
    """Leave a note for m2-gm in the game container. True when it was written.

    Written is not the same as done, and the caller must not claim otherwise.
    """
    spool = config.RATES_SPOOL
    try:
        spool.mkdir(parents=True, exist_ok=True)
        temporary = spool / "gm.request.new"
        stamp = int(time.time())
        temporary.write_text(f"id={config.REQUEST_PREFIX}{stamp}\ntime={stamp}\n", encoding="utf-8")
        os.replace(temporary, spool / "gm.request")
        return True
    except OSError:
        return False


def _ask_an_owner():
    """Have an online owner run /reload a. True when one did.

    The mt2009 line has no admin socket, so nothing outside the game can make
    it re-read the list. What does re-read it is the db core on
    HEADER_GD_RELOAD_ADMIN, which /reload a sends - and the helper quest runs a
    command as the character whose timer it is, so only one that already holds
    the rank can carry it. One row per owner; the first "done" is enough and
    the rest are withdrawn.
    """
    try:
        owners = [row["mName"] for row in db.rows(
            "SELECT mName FROM common.gmlist WHERE mAuthority = 'IMPLEMENTOR' LIMIT %s",
            (RELOAD_ASK_LIMIT,)) if row.get("mName")]
        if not owners:
            return False
        queue_ids = []
        with db.connect() as connection, connection.cursor() as cursor:
            for owner in owners:
                cursor.execute(
                    f"INSERT INTO {config.GAME_QUEUE_TABLE} (player_name, cmd, arg1, arg2)"
                    " VALUES (%s, 'GM_RELOAD', '', '')", (owner,))
                queue_ids.append(cursor.lastrowid)
    except pymysql.MySQLError:
        return False

    marks = ",".join(["%s"] * len(queue_ids))
    done = False
    deadline = time.time() + RELOAD_WAIT_SECONDS
    try:
        while time.time() < deadline and not done:
            time.sleep(RELOAD_POLL_SECONDS)
            rows = db.rows(
                f"SELECT status FROM {config.GAME_QUEUE_TABLE} WHERE id IN ({marks})", queue_ids)
            done = any(row.get("status") == "done" for row in rows)
    finally:
        # Whatever happened, no owner should find a stale errand later.
        try:
            db.execute(
                f"UPDATE {config.GAME_QUEUE_TABLE} SET status = 'cancelled'"
                f" WHERE id IN ({marks}) AND status = 'pending'", queue_ids)
        except pymysql.MySQLError:
            pass
    return done


def reload_list():
    """Make the running server re-read common.gmlist. True when it will."""
    if engine.IS_MT2009:
        return _ask_an_owner()
    return _ask_game_container()


def set_rank(pid, rank):
    """Grant or revoke a rank. An empty rank removes it. Returns a message."""
    if rank and rank not in GM_RANK_SET:
        # Not reachable from the form; reachable from a hand-made POST, and an
        # unknown value would be stored happily and then dropped by the server.
        raise ValueError("Nieznana ranga.")

    character = db.one(
        "SELECT p.name AS name, a.login AS login FROM player.player p"
        " LEFT JOIN account.account a ON a.id = p.account_id WHERE p.id = %s", (pid,))
    if not character:
        raise ValueError("Tej postaci już nie ma.")
    name, login = character["name"], character.get("login") or ""

    with db.connect() as connection, connection.cursor() as cursor:
        # Replace rather than update: mName has no unique key, and a table that
        # has collected two rows for one character would otherwise keep the
        # older rank alive underneath the new one.
        cursor.execute("DELETE FROM common.gmlist WHERE mName = %s", (name,))
        if rank:
            cursor.execute(
                "INSERT INTO common.gmlist (mAccount, mName, mContactIP, mServerIP, mAuthority)"
                " VALUES (%s, %s, '', 'ALL', %s)", (login, name, rank))

    reloaded = reload_list()
    if rank and reloaded:
        return f"{name} ma teraz rangę {gm_rank_label(rank)}. Serwer już o tym wie."
    if rank:
        return (f"{name} ma teraz rangę {gm_rank_label(rank)}. Wpis jest w bazie, "
                "ale zacznie działać dopiero po restarcie serwera — żadna postać "
                "z rangą właściciela nie jest teraz w grze, więc nie ma kto odświeżyć listy.")
    if reloaded:
        # Removal is the asymmetric case: the reload hands the server the new
        # list, but the server re-applies ranks only for characters IN that
        # list. One just taken out is not visited, so a player who is online
        # keeps the commands until they log out. Saying so is the difference
        # between a known limit and a bug report.
        return (f"{name} nie ma już rangi. Jeśli postać jest teraz w grze, "
                "komendy znikną jej dopiero po wylogowaniu.")
    return (f"{name} nie ma już rangi w bazie. Serwer zauważy to po restarcie "
            "albo po wylogowaniu tej postaci.")
