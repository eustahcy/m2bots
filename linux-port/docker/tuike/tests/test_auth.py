"""The setup wizard and the passphrase gate, against a mutable settings store."""
import os
import sys
import tempfile

ROOT = os.path.abspath(
    sys.argv[1] if len(sys.argv) > 1 else os.path.join(os.path.dirname(__file__), ".."))
sys.path.insert(0, ROOT)

os.environ.update({
    "DB_USER": "test", "DB_PASSWORD": "test", "PLAYERBOTS_ENGINE": "mt2009",
    "TUIKE_SESSION_SECRET": "auth-test-secret",
    "TUIKE_RATES_SPOOL": tempfile.mkdtemp(prefix="tuike-auth-spool-"),
    "TUIKE_UPDATE_SPOOL": tempfile.mkdtemp(prefix="tuike-auth-update-"),
    "TUIKE_UPDATE_CHECK": "0",
})

import pymysql  # noqa: E402

# The only state this test needs: the settings table.
SETTINGS = {}


class Cursor:
    def __init__(self):
        self.rows = []
        self.rowcount = 0
        self.lastrowid = 1

    def __enter__(self):
        return self

    def __exit__(self, *exc):
        return False

    def execute(self, sql, params=()):
        flat = " ".join(str(sql).split())
        self.rows = []
        if "SELECT name, value FROM player.web_tuike_settings" in flat:
            self.rows = [{"name": k, "value": v} for k, v in SETTINGS.items()]
        elif "INSERT INTO player.web_tuike_settings" in flat and params:
            SETTINGS[params[0]] = str(params[1])
        self.rowcount = len(self.rows)

    def executemany(self, sql, seq):
        flat = " ".join(str(sql).split())
        seq = list(seq)
        if "INSERT INTO player.web_tuike_settings" in flat:
            for name, value in seq:
                SETTINGS[name] = str(value)
        elif "INSERT IGNORE INTO player.web_tuike_settings" in flat:
            for name, value in seq:
                SETTINGS.setdefault(name, str(value))
        self.rowcount = len(seq)

    def fetchall(self):
        return list(self.rows)

    def fetchone(self):
        return dict(self.rows[0]) if self.rows else None

    def close(self):
        pass


class Connection:
    def __enter__(self):
        return self

    def __exit__(self, *exc):
        return False

    def cursor(self):
        return Cursor()

    def begin(self):
        pass

    def commit(self):
        pass

    def rollback(self):
        pass

    def close(self):
        pass


pymysql.connect = lambda *a, **k: Connection()

from tuike import create_app  # noqa: E402

failures = []


def check(name, condition, detail=""):
    if condition:
        print(f"  OK  {name}")
    else:
        failures.append((name, detail))
        print(f"  ZLE {name}  {detail}")


app = create_app()

# --- the wizard --------------------------------------------------------------
print("Kreator pierwszego uruchomienia:")
SETTINGS.clear()
SETTINGS.update({"setup_complete": "0", "panel_name": "Metin2 Singleplayer",
                 "stuck_minutes": "5", "theme": "midnight", "density": "comfortable",
                 "monitor_mode": "vps", "auth_enabled": "0", "auth_password_hash": ""})

with app.test_client() as client:
    check("niedokonczony setup przekierowuje z pulpitu",
          client.get("/").status_code == 302)
    check("strona kreatora sie renderuje", client.get("/setup").status_code == 200)

    # Too short a password with protection asked for must be refused.
    response = client.post("/setup", data={
        "panel_name": "Mój Świat", "stuck_minutes": "7", "theme": "ember",
        "density": "compact", "monitor_mode": "vps",
        "auth_enabled": "1", "panel_password": "krotkie"})
    check("krotkie haslo odrzucone", response.status_code == 200
          and "co najmniej" in response.get_data(as_text=True))
    check("setup nadal niedokonczony", SETTINGS["setup_complete"] == "0")

    response = client.post("/setup", data={
        "panel_name": "Mój Świat", "stuck_minutes": "7", "theme": "ember",
        "density": "compact", "monitor_mode": "vps",
        "auth_enabled": "1", "panel_password": "bardzo-dlugie-haslo"})
    check("poprawny kreator przekierowuje na pulpit", response.status_code == 302)
    check("ustawienia zapisane", SETTINGS["panel_name"] == "Mój Świat"
          and SETTINGS["theme"] == "ember" and SETTINGS["stuck_minutes"] == "7",
          str(SETTINGS))
    check("ochrona wlaczona", SETTINGS["auth_enabled"] == "1")
    check("zapisano skrot, nie haslo",
          SETTINGS["auth_password_hash"] and "bardzo-dlugie-haslo" not in SETTINGS["auth_password_hash"])
    check("kreator loguje od razu", client.get("/").status_code == 200)
    check("ukonczony kreator przekierowuje z /setup", client.get("/setup").status_code == 302)

# --- the gate ----------------------------------------------------------------
print("\nBramka hasla:")
with app.test_client() as fresh:
    for path in ("/", "/players", "/manage", "/accounts", "/manage/items",
                 "/api/live-bots", "/api/manage-status", "/economy", "/rankings"):
        response = fresh.get(path)
        check(f"bez zalogowania {path} -> logowanie",
              response.status_code == 302 and "/login" in response.headers.get("Location", ""),
              f"{response.status_code} -> {response.headers.get('Location')}")

    response = fresh.post("/login", data={"password": "zle-haslo"})
    check("bledne haslo nie loguje", response.status_code == 200
          and "Nieprawidłowe hasło" in response.get_data(as_text=True))
    check("po bledzie pulpit nadal zamkniety", fresh.get("/").status_code == 302)

    response = fresh.post("/login", data={"password": "bardzo-dlugie-haslo"})
    check("poprawne haslo loguje", response.status_code == 302)
    check("po zalogowaniu pulpit otwarty", fresh.get("/").status_code == 200)

    check("wylogowanie zamyka panel",
          fresh.post("/logout").status_code == 302 and fresh.get("/").status_code == 302)

# --- protection off ----------------------------------------------------------
print("\nPanel bez ochrony:")
SETTINGS["auth_enabled"] = "0"
SETTINGS["auth_password_hash"] = ""
with app.test_client() as open_client:
    check("bez ochrony pulpit otwarty", open_client.get("/").status_code == 200)
    check("bez ochrony /login przekierowuje na pulpit",
          open_client.get("/login").status_code == 302)
    # The update button must stay unavailable: on an open panel anybody who can
    # reach the page could rebuild the server.
    body = open_client.get("/manage").get_data(as_text=True)
    check("bez ochrony przycisk aktualizacji jest niedostepny",
          "wymagają włączonej ochrony hasłem" in body)
    from tuike.security import CSRF_SESSION_KEY
    with open_client.session_transaction() as session:
        session[CSRF_SESSION_KEY] = "token"
    open_client.post("/manage/update", data={"csrf": "token"})
    body = open_client.get("/manage").get_data(as_text=True)
    check("proba aktualizacji bez ochrony odrzucona",
          "wymagają włączonej ochrony hasłem" in body)

print()
if failures:
    print(f"NIEPOWODZENIA: {len(failures)}")
    for name, detail in failures:
        print("=" * 70)
        print(f"{name}\n{detail}")
    sys.exit(1)
print("Wszystkie testy uwierzytelniania przeszly.")
