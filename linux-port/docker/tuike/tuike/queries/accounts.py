"""Creating accounts, and creating a game master along with one.

An ordinary account is a single row. A game master is four: the account, the
character, the character-slot index the client reads at login, and the gmlist
row that grants the authority. The original tables are MyISAM, so MariaDB does
not roll a half-finished set back for us - hence the explicit cleanup below,
which removes only what this request created.
"""
import re

import pymysql

from .. import config, db, engine, live
from ..text import game_text
from ..gamedata.characters import (
    AUTHORITIES, GM_EMPIRE_STARTS, GM_JOB_OPTIONS, GM_JOB_STARTS, GM_NAME_PATTERN,
    GM_RACE_BY_CLASS_GENDER, GM_GENDER_OPTIONS,
)

DISPLAY_SIZES = ("100", "1000", "all")
PASSWORD_MIN_LENGTH = 6
LOGIN_MIN_LENGTH = 3
DELETION_CODE_LENGTH = 7


class AccountError(ValueError):
    """Something the operator can fix by editing the form."""


def roster(query="", display="100"):
    """Newest accounts first, optionally filtered by login or character name."""
    if display not in DISPLAY_SIZES:
        display = "100"
    where, params = [], []
    if query:
        where.append("(a.login LIKE %s OR EXISTS ("
                     "SELECT 1 FROM player.player p WHERE p.account_id = a.id AND p.name LIKE %s))")
        params.extend([f"%{query}%", f"%{query}%"])
    empire_column = "a.empire" if engine.ACCOUNT_HAS_EMPIRE else "0 AS empire"
    sql = (f"SELECT a.id, a.login, a.email, a.status, {empire_column},"
           " a.create_time, a.last_play"
           " FROM account.account a")
    if where:
        sql += " WHERE " + " AND ".join(where)
    sql += " ORDER BY a.id DESC"
    if display != "all":
        sql += " LIMIT %s"
        params.append(int(display))
    return db.rows(sql, params)


def validate(form):
    """Check a creation form. Returns a clean dict or raises AccountError."""
    login = (form.get("login") or "").strip()
    password = form.get("password") or ""
    email = (form.get("email") or "").strip()[:120]
    deletion_code = (form.get("deletion_code") or "").strip()
    authority = form.get("authority", "PLAYER")
    gm_name = (form.get("gm_name") or "").strip()
    gm_gender = form.get("gm_gender", "classic")
    try:
        empire = max(1, min(3, int(form.get("empire") or 1)))
    except ValueError:
        empire = 1
    try:
        gm_job = int(form.get("gm_job") or 0)
    except ValueError:
        gm_job = -1

    limit = engine.LOGIN_MAX_LENGTH
    if not (LOGIN_MIN_LENGTH <= len(login) <= limit and login.replace("_", "").isalnum()):
        raise AccountError(f"Login ma mieć {LOGIN_MIN_LENGTH}–{limit} znaków: litery, cyfry i _.")
    if len(password) < PASSWORD_MIN_LENGTH:
        raise AccountError(f"Hasło musi mieć minimum {PASSWORD_MIN_LENGTH} znaków.")
    if authority not in AUTHORITIES:
        raise AccountError("Wybierz prawidłowy rodzaj konta.")
    if not (deletion_code.isdigit() and len(deletion_code) == DELETION_CODE_LENGTH):
        raise AccountError(f"Kod usunięcia postaci ma zawierać dokładnie {DELETION_CODE_LENGTH} cyfr.")
    if authority != "PLAYER":
        if not re.fullmatch(GM_NAME_PATTERN, gm_name):
            raise AccountError("Nick postaci GM ma mieć 2–24 znaki. "
                               "Dozwolony jest jeden prefiks, np. [GM]Seban.")
        if gm_job not in dict(GM_JOB_OPTIONS):
            raise AccountError("Wybierz poprawną klasę postaci GM.")
        if gm_gender not in dict(GM_GENDER_OPTIONS):
            raise AccountError("Wybierz prawidłową płeć postaci GM.")
    return {
        "login": login, "password": password, "email": email,
        "deletion_code": deletion_code, "authority": authority, "empire": empire,
        "gm_name": gm_name, "gm_job": gm_job, "gm_gender": gm_gender,
    }


def _insert_account(cursor, values):
    if engine.ACCOUNT_HAS_EMPIRE:
        cursor.execute(
            "INSERT INTO account.account (login, password, social_id, email, status, empire)"
            " VALUES (%s, PASSWORD(%s), %s, %s, 'OK', %s)",
            (values["login"], values["password"], values["deletion_code"], values["email"],
             values["empire"] if values["authority"] != "PLAYER" else 0),
        )
    else:
        # mt2009's account table has no empire column; the kingdom lives in
        # player_index, written below for a GM and by the game itself for a
        # player's first character.
        cursor.execute(
            "INSERT INTO account.account (login, password, social_id, email, status)"
            " VALUES (%s, PASSWORD(%s), %s, %s, 'OK')",
            (values["login"], values["password"], values["deletion_code"], values["email"]),
        )
    return cursor.lastrowid


def _insert_gm_character(cursor, account_id, values):
    x, y, map_index = GM_EMPIRE_STARTS[values["empire"]]
    st, ht, dx, iq, hp, mp = GM_JOB_STARTS[values["gm_job"]]
    race = GM_RACE_BY_CLASS_GENDER[(values["gm_job"], values["gm_gender"])]
    bank_column = "" if not engine.PLAYER_HAS_BANK_VALUE else ",bank_value"
    bank_value = "" if not engine.PLAYER_HAS_BANK_VALUE else ",0"
    cursor.execute(
        "INSERT INTO player.player"
        " (account_id,name,job,dir,x,y,map_index,exit_x,exit_y,exit_map_index,hp,mp,"
        "  stamina,random_hp,random_sp,level,st,ht,dx,iq,stat_point,skill_point,"
        "  sub_skill_point,part_main,part_base,part_hair,skill_group,horse_hp,"
        f"  horse_stamina,horse_level,horse_hp_droptime,horse_riding,horse_skill_point{bank_column})"
        " VALUES (%s,%s,%s,0,%s,%s,%s,%s,%s,%s,%s,%s,1000,0,0,1,%s,%s,%s,%s,"
        f"         0,0,0,0,0,0,0,0,0,0,0,0,0{bank_value})",
        (account_id, values["gm_name"], race, x, y, map_index, x, y, map_index, hp, mp,
         st, ht, dx, iq),
    )
    player_id = cursor.lastrowid
    # Metin reads character slots from player_index. A player row without this
    # entry exists in SQL but is invisible at login.
    cursor.execute(
        "INSERT INTO player.player_index (id,pid1,pid2,pid3,pid4,empire)"
        " VALUES (%s,%s,0,0,0,%s)"
        " ON DUPLICATE KEY UPDATE pid1=VALUES(pid1),pid2=0,pid3=0,pid4=0,empire=VALUES(empire)",
        (account_id, player_id, values["empire"]),
    )
    cursor.execute(
        "INSERT INTO common.gmlist (mAccount,mName,mContactIP,mServerIP,mAuthority)"
        " VALUES (%s,%s,'','ALL',%s)",
        (values["login"], values["gm_name"], values["authority"]),
    )
    return player_id


def _clean_up(values, account_id, player_id):
    """Undo a half-written game master. MyISAM will not do it for us."""
    try:
        with db.connect() as connection, connection.cursor() as cursor:
            cursor.execute("DELETE FROM common.gmlist WHERE mAccount=%s AND mName=%s",
                           (values["login"], values["gm_name"]))
            if player_id:
                cursor.execute("DELETE FROM player.player WHERE id=%s AND account_id=%s",
                               (player_id, account_id))
            cursor.execute("DELETE FROM player.player_index WHERE id=%s", (account_id,))
            cursor.execute("DELETE FROM account.account WHERE id=%s AND login=%s",
                           (account_id, values["login"]))
    except pymysql.MySQLError:
        pass


def create(values):
    """Create the account, and the game master if one was asked for.

    Returns the message to show. Raises AccountError for anything the operator
    can fix and leaves nothing behind when it does.
    """
    account_id = player_id = None
    is_gm = values["authority"] != "PLAYER"
    try:
        with db.connect() as connection:
            with connection.cursor() as cursor:
                if is_gm:
                    cursor.execute("SELECT id FROM player.player WHERE name=%s LIMIT 1",
                                   (values["gm_name"],))
                    if cursor.fetchone():
                        raise AccountError("Taki nick postaci już istnieje.")
                connection.begin()
                account_id = _insert_account(cursor, values)
                if is_gm:
                    player_id = _insert_gm_character(cursor, account_id, values)
                connection.commit()
    except AccountError:
        raise
    except pymysql.MySQLError as exc:
        if account_id:
            _clean_up(values, account_id, player_id)
        detail = exc.args[1] if len(exc.args) > 1 else exc
        raise AccountError(f"Nie utworzono konta: {detail}")
    note("account_create", values["login"],
         "GM " + values["gm_name"] if is_gm else "konto gracza")
    if is_gm:
        return (f"Utworzono konto i postać GM „{values['gm_name']}”. Postać jest dostępna "
                "od razu; uprawnienia GM staną się aktywne po restarcie usług gry.")
    return "Konto utworzone."

# --- one account, and what can be done to it ---------------------------------
# A blocked account cannot log in; the engine reads account.status when a login
# arrives. It does NOT throw out a session that is already running - the quest
# this panel talks to has no command for that - so the panel says so rather
# than pretending the player vanishes on the spot.
BLOCKED_STATUS = "BLOCK"
OPEN_STATUS = "OK"
LOGIN_HISTORY_LIMIT = 40
RELATED_LIMIT = 20


def detail(account_id):
    """One account's row, with its kingdom and Dragon Coins."""
    empire_column = "a.empire" if engine.ACCOUNT_HAS_EMPIRE else "0 AS empire"
    account = db.one(
        f"SELECT a.id, a.login, a.email, a.status, a.create_time, a.last_play, a.cash,"
        f" {empire_column} FROM account.account a WHERE a.id = %s",
        (account_id,),
    )
    if account:
        account["blocked"] = str(account.get("status") or "").upper() != OPEN_STATUS
    return account


def characters(account_id):
    """The characters on this account, with their live position folded in."""
    rows = db.rows(
        "SELECT p.id, p.name, p.level, p.job, p.map_index, p.gold, p.playtime, p.last_play"
        " FROM player.player p WHERE p.account_id = %s ORDER BY p.level DESC",
        (account_id,),
    )
    current = live.statuses()
    for row in rows:
        row["name"] = game_text(row["name"])
        row["live"] = row["id"] in current
    return rows


def logins(account_id, limit=LOGIN_HISTORY_LIMIT):
    """This account's own login history: when, from where, and as whom."""
    try:
        rows = db.rows(
            "SELECT l.type, l.time, l.ip, l.hwid, l.pid, l.playtime, p.name, p.level"
            " FROM log.loginlog l LEFT JOIN player.player p ON p.id = l.pid"
            " WHERE l.account_id = %s ORDER BY l.time DESC LIMIT %s",
            (account_id, int(limit)),
        )
    except pymysql.MySQLError:
        return []
    for row in rows:
        row["type"] = game_text(row.get("type"))
        row["name"] = game_text(row.get("name"))
    return rows


def related(account_id, limit=RELATED_LIMIT):
    """Other accounts seen from the same address or the same machine.

    Shared addresses are evidence, not proof: a household, a school or a phone
    network puts unrelated people behind one address. The HWID is the stronger
    hint, and the engine only records one when the client sends it.
    """
    try:
        rows = db.rows(
            """SELECT other.account_id, a.login, a.status,
                  MAX(other.time) AS last_seen,
                  GROUP_CONCAT(DISTINCT other.ip ORDER BY other.ip SEPARATOR ', ') AS ips,
                  SUM(mine.hwid IS NOT NULL AND other.hwid = mine.hwid) AS same_machine
                FROM log.loginlog other
                JOIN (SELECT DISTINCT ip, NULLIF(hwid, '') AS hwid FROM log.loginlog
                      WHERE account_id = %s) mine
                  ON other.ip = mine.ip OR other.hwid = mine.hwid
                LEFT JOIN account.account a ON a.id = other.account_id
                WHERE other.account_id <> %s AND other.ip NOT LIKE '127.%%'
                GROUP BY other.account_id, a.login, a.status
                ORDER BY last_seen DESC LIMIT %s""",
            (account_id, account_id, int(limit)),
        )
    except pymysql.MySQLError:
        return []
    for row in rows:
        row["same_machine"] = int(row.get("same_machine") or 0) > 0
    return rows


def set_blocked(account_id, blocked):
    """Block or unblock an account. Returns a sentence for the operator."""
    account = detail(account_id)
    if not account:
        raise AccountError("Nie ma konta o tym numerze.")
    status = BLOCKED_STATUS if blocked else OPEN_STATUS
    db.execute("UPDATE account.account SET status = %s WHERE id = %s", (status, account_id))
    note("account_block" if blocked else "account_unblock", account["login"], f"status={status}")
    if blocked:
        return (f"Konto {account['login']} zablokowane. Gracz nie zaloguje się ponownie; "
                "sesja, która już trwa, kończy się dopiero przy wylogowaniu.")
    return f"Konto {account['login']} odblokowane."


def set_password(account_id, password):
    """Set a new password, hashed by the database the way the engine expects."""
    account = detail(account_id)
    if not account:
        raise AccountError("Nie ma konta o tym numerze.")
    password = (password or "").strip()
    if len(password) < PASSWORD_MIN_LENGTH:
        raise AccountError(f"Hasło musi mieć co najmniej {PASSWORD_MIN_LENGTH} znaków.")
    db.execute("UPDATE account.account SET password = PASSWORD(%s) WHERE id = %s",
               (password, account_id))
    note("account_password", account["login"], "hasło zmienione z panelu")
    return f"Hasło konta {account['login']} zmienione."


# --- the operator's own trail -------------------------------------------------
AUDIT_LIMIT = 50


def note(action, target, detail_text=""):
    """Record one operator action. Never raises: a missing trail must not stop
    the action the operator actually asked for."""
    try:
        db.execute(
            f"INSERT INTO {config.AUDIT_TABLE} (action, target, detail) VALUES (%s, %s, %s)",
            (action[:32], str(target)[:64], str(detail_text)[:255]),
        )
    except pymysql.MySQLError:
        pass


AUDIT_LABELS = {
    "account_block": "Zablokowane konto",
    "account_unblock": "Odblokowane konto",
    "account_password": "Zmienione hasło",
    "account_create": "Założone konto",
}


def audit(limit=AUDIT_LIMIT):
    """What was done from this panel, newest first."""
    try:
        rows = db.rows(
            f"SELECT at, action, target, detail FROM {config.AUDIT_TABLE}"
            f" ORDER BY id DESC LIMIT {int(limit)}")
    except pymysql.MySQLError:
        return []
    for row in rows:
        row["label"] = AUDIT_LABELS.get(row["action"], row["action"])
    return rows
