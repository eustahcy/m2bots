"""Exercise Tuike's write paths: forms, the file spools, and both workers."""
import os
import sys
import tempfile
import traceback

ROOT = os.path.abspath(
    sys.argv[1] if len(sys.argv) > 1 else os.path.join(os.path.dirname(__file__), ".."))
sys.path.insert(0, ROOT)

SPOOL = tempfile.mkdtemp(prefix="tuike-spool-")
UPDATE = tempfile.mkdtemp(prefix="tuike-update-")

os.environ.update({
    "TUIKE_ROOT": ROOT,
    "DB_USER": "test", "DB_PASSWORD": "test",
    "PLAYERBOTS_ENGINE": "mt2009",
    "TUIKE_SESSION_SECRET": "writes-test-secret",
    "TUIKE_RATES_SPOOL": SPOOL,
    "TUIKE_UPDATE_SPOOL": UPDATE,
    "TUIKE_UPDATE_CHECK": "0",
})

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import test_pages  # noqa: E402,F401  - installs the fake pymysql connection

from tuike import config, spool, updater  # noqa: E402
from tuike.gamedata import bots as botdata  # noqa: E402

failures = []


def check(name, condition, detail=""):
    if condition:
        print(f"  OK  {name}")
    else:
        failures.append((name, detail))
        print(f"  ZLE {name}  {detail}")


# --- the spool protocol ------------------------------------------------------
print("Kolejka plikowa:")

spool.queue_restart({"exp": 250, "drop": 300, "yang": 150})
request = os.path.join(SPOOL, "request")
check("queue_restart tworzy plik request", os.path.exists(request))
body = open(request, encoding="utf-8").read()
check("request niesie mnozniki", "exp=250" in body and "drop=300" in body and "yang=150" in body, body)
check("request oznaczony prefiksem tuike-", f"id={config.REQUEST_PREFIX}" in body, body)
status = open(os.path.join(SPOOL, "rates.status"), encoding="utf-8").read()
check("rates.status zapisany jako running", "state=running" in status, status)
check("read_rates czyta wlasny zapis", spool.read_rates() == {"exp": 250, "drop": 300, "yang": 150},
      str(spool.read_rates()))
check("restart_in_flight wykrywa trwajacy restart", spool.restart_in_flight() is True)

# No helper is running, so a respawn change must be refused while a plain
# restart still goes through.
helper = spool.settings_helper_status()
check("helper zglasza brak gotowosci", helper["ready"] is False)
try:
    spool.queue_server_settings("apply", {"exp": 100, "drop": 100, "yang": 100}, {"21": 30})
    failures.append(("respawn bez helpera powinien byc odrzucony", ""))
except RuntimeError as error:
    check("respawn bez helpera odrzucony z wyjasnieniem", "respawn" in str(error).lower(), str(error))

os.remove(os.path.join(SPOOL, "rates.status"))
spool.queue_server_settings("restart")
check("sam restart dziala bez helpera", os.path.exists(request))

# Rates-only apply also bypasses the helper: every respawn field untouched
# arrives as "reset", which is not a respawn change.
os.remove(os.path.join(SPOOL, "rates.status"))
spool.queue_server_settings("apply", {"exp": 120, "drop": 120, "yang": 120},
                            {"21": "reset", "23": "reset"})
check("same raty przechodza bez helpera", spool.read_rates()["exp"] == 120)

# --- bot behaviour weights ---------------------------------------------------
print("\nWagi zachowania botow:")

weights = spool.read_ai_weights()
check("brak pliku daje wartosci neutralne",
      weights["METIN"] == botdata.AI_WEIGHT_NEUTRAL and weights["CHEST"] is None, str(weights))

# A key this panel does not know must survive a save.
with open(spool.AI_WEIGHTS_FILE, "w", encoding="utf-8") as handle:
    handle.write("METIN\t200\nNOWA_OPCJA_Z_NOWEGO_RDZENIA\t42\n")
weights = spool.read_ai_weights()
check("odczyt znanego klucza", weights["METIN"] == 200, str(weights["METIN"]))

weights["METIN"] = 175
weights["CHAT"] = 0
weights["SCRAP"] = 40
weights["CHEST"] = 25
spool.write_ai_weights(weights)
saved = open(spool.AI_WEIGHTS_FILE, encoding="utf-8").read()
check("zapis znanego klucza", "METIN\t175" in saved, saved)
check("przelacznik zapisany jako 0/1", "CHAT\t0" in saved, saved)
check("procent zapisany", "SCRAP\t40" in saved, saved)
check("promile zapisane", "CHEST\t25" in saved, saved)
check("nieznany klucz nowszego rdzenia zachowany",
      "NOWA_OPCJA_Z_NOWEGO_RDZENIA\t42" in saved, saved)
check("odczyt po zapisie zgodny", spool.read_ai_weights()["METIN"] == 175)

# Values outside the allowed range are clamped, never written through.
spool.write_ai_weights({**weights, "METIN": 9999, "SCRAP": -5, "CHEST": 5000})
saved = open(spool.AI_WEIGHTS_FILE, encoding="utf-8").read()
check("waga powyzej zakresu przyciecie", f"METIN\t{botdata.AI_WEIGHT_MAX}" in saved, saved)
check("procent ponizej zera przyciety", "SCRAP\t0" in saved, saved)
check("promile powyzej 1000 przyciete", "CHEST\t1000" in saved, saved)

# --- the updater -------------------------------------------------------------
print("\nAktualizator:")

state = updater.status()
check("bez watchera updater zglasza brak gotowosci", state["watcher_ready"] is False)
check("bez watchera jest komunikat", "nie jest uruchomiony" in state["message"], state["message"])
try:
    updater.queue()
    failures.append(("zlecenie bez watchera powinno byc odrzucone", ""))
except RuntimeError as error:
    check("zlecenie bez watchera odrzucone", "updater" in str(error).lower(), str(error))

open(os.path.join(UPDATE, "watcher"), "w").close()
identifier = updater.queue()
check("zlecenie z watcherem zapisane", os.path.exists(os.path.join(UPDATE, "request")))
check("zlecenie ma prefiks tuike-", identifier.startswith(config.REQUEST_PREFIX), identifier)
check("brak plikow tymczasowych po zleceniu",
      not [name for name in os.listdir(UPDATE) if name.endswith(".new")], str(os.listdir(UPDATE)))

with open(os.path.join(UPDATE, "update.status"), "w", encoding="utf-8") as handle:
    handle.write("state=running\nstep=2\nsteps=5\nmessage=W trakcie\n")
state = updater.status()
check("postep liczony ze step/steps", state["percent"] == 40, str(state["percent"]))

# --- forms, with a real CSRF token -------------------------------------------
print("\nFormularze:")

from tuike import create_app  # noqa: E402
from tuike.security import CSRF_SESSION_KEY  # noqa: E402

app = create_app()
with app.test_client() as client:
    with client.session_transaction() as session:
        session[CSRF_SESSION_KEY] = "token"

    posts = [
        ("/manage/settings", {"csrf": "token", "panel_name": "Mój Świat",
                              "stuck_minutes": "9", "theme": "forest",
                              "density": "compact", "monitor_mode": "docker"}),
        ("/manage/behavior", {"csrf": "token", "METIN": "150", "CHAT": "1", "SCRAP": "20"}),
        ("/manage/restart-config", {"csrf": "token", "submit_action": "apply",
                                    "exp": "300", "drop": "200", "yang": "100",
                                    "map_21": "", "map_stone_21": "45"}),
        ("/manage/restart", {"csrf": "token", "confirmation": "RESTART"}),
        ("/manage/update", {"csrf": "token"}),
        ("/accounts", {"csrf": "token", "login": "tester", "password": "haslo123",
                       "deletion_code": "1234567", "authority": "PLAYER"}),
    ]
    for path, data in posts:
        try:
            response = client.post(path, data=data)
            check(f"POST {path} -> 302", response.status_code == 302,
                  f"{response.status_code}: {response.get_data(as_text=True)[:300]}")
        except Exception:
            failures.append((f"POST {path}", traceback.format_exc()))
            print(f"  ZLE POST {path} rzucil wyjatek")

    # Validation must reject, not crash.
    # The console redirects on a bad form and flashes the reason; the page is
    # then re-rendered by the follow-up GET.
    bad = [
        ("/manage/restart-config", {"csrf": "token", "submit_action": "apply",
                                    "exp": "0", "drop": "100", "yang": "100"},
         "Mnożniki muszą"),
        ("/manage/restart-config", {"csrf": "token", "submit_action": "apply",
                                    "exp": "100", "drop": "100", "yang": "100",
                                    "map_21": "99999"},
         "respawn musi"),
        ("/manage/restart", {"csrf": "token", "confirmation": "tak"}, "wpisz RESTART"),
        ("/manage/settings", {"csrf": "token", "panel_name": "", "stuck_minutes": "5",
                              "theme": "forest", "density": "compact", "monitor_mode": "vps"},
         "Nazwa panelu nie może być pusta"),
        ("/manage/settings", {"csrf": "token", "panel_name": "X", "stuck_minutes": "5",
                              "theme": "nie-ma-takiego", "density": "compact",
                              "monitor_mode": "vps"},
         "dostępnych motywów"),
    ]
    for path, data, expected_message in bad:
        response = client.post(path, data=data)
        check(f"POST {path} z blednymi danymi -> 302", response.status_code == 302,
              f"{response.status_code}")
        body = client.get("/manage").get_data(as_text=True)
        check(f"  komunikat zawiera: {expected_message}", expected_message in body,
              "komunikatu nie ma na stronie")

    # The account form re-renders in place instead of redirecting, so the
    # operator keeps what they typed.
    response = client.post("/accounts", data={"csrf": "token", "login": "ab",
                                              "password": "1", "deletion_code": "12",
                                              "authority": "PLAYER"})
    body = response.get_data(as_text=True)
    check("POST /accounts z blednym loginem -> 200 i komunikat",
          response.status_code == 200 and "Login ma mieć" in body,
          f"{response.status_code}")

    response = client.post("/accounts", data={"csrf": "token", "login": "tester2",
                                              "password": "haslo123", "deletion_code": "1234567",
                                              "authority": "GOD", "gm_name": "zle nick!",
                                              "gm_job": "0", "gm_gender": "classic",
                                              "empire": "1"})
    check("POST /accounts ze zlym nickiem GM -> komunikat",
          "Nick postaci GM" in response.get_data(as_text=True), str(response.status_code))

# --- character actions -------------------------------------------------------
print("\nCzynności na postaci:")

from tuike import commands, gm  # noqa: E402

# Validation first: every one of these is something an operator can type.
for cmd, form, why in (
    ("ITEM", {"vnum": "", "quantity": "1"}, "brak VNUM"),
    ("ITEM", {"vnum": "50051", "quantity": "0"}, "ilość zero"),
    ("ITEM", {"vnum": "50051", "quantity": "9999"}, "ilość ponad limit silnika"),
    ("GOLD", {"preset": "custom", "amount": "0"}, "kwota zero"),
    ("GOLD", {"preset": "custom", "amount": "nie-liczba"}, "kwota nieliczbowa"),
    ("LEVEL", {"level": "0"}, "poziom zero"),
    ("LEVEL", {"level": "999"}, "poziom ponad maksimum"),
    ("WARP", {"target": "1 2"}, "miejsce spoza listy"),
    ("SPEED", {"percent": "500"}, "prędkość spoza listy"),
    ("NIE_MA", {}, "nieznana komenda"),
):
    try:
        commands.normalise(cmd, form)
        failures.append((f"{cmd}: {why} powinno być odrzucone", str(form)))
        print(f"  ZLE {cmd}: {why} nie zostało odrzucone")
    except commands.CommandError:
        check(f"{cmd}: {why} odrzucone", True)

check("ITEM: poprawny formularz",
      commands.normalise("ITEM", {"vnum": "50051", "quantity": "5"}) == ("ITEM", "50051", "5"))
check("GOLD: gotowa kwota",
      commands.normalise("GOLD", {"preset": "1000000"}) == ("GOLD", "1000000", "1"))
check("GOLD: kwota ujemna odbiera",
      commands.normalise("GOLD", {"preset": "custom", "amount": "-500"})[1] == "-500")
check("LEVEL: poprawny poziom",
      commands.normalise("LEVEL", {"level": "42"}) == ("LEVEL", "42", "1"))
warp_target = actions_warp = __import__("tuike.gamedata.actions", fromlist=["x"]).WARP_LOCATIONS[0][2]
check("WARP: miejsce z listy",
      commands.normalise("WARP", {"target": warp_target})[0] == "WARP")
check("SPEED: prędkość z listy i godzinna ważność",
      commands.normalise("SPEED", {"percent": "30"}) == ("SPEED", "30", "3600"))

# The happy path: the fake queue answers "done" on the first poll.
commands.WAIT_SECONDS = 2.0
message = commands.run(7, "ITEM", "50051", "1")
check("czynność wykonana w grze daje komunikat", "wykonano w grze" in message, message)

# And the offline path: the queue row stays pending, so the panel claims it and
# writes the change itself.
import test_pages  # noqa: E402

pending = [(fragment, rows) for fragment, rows in test_pages.ROUTES
           if fragment == "SELECT status FROM player.web_admin_queue"]
for entry in pending:
    test_pages.ROUTES[test_pages.ROUTES.index(entry)] = (entry[0], [{"status": "pending"}])
try:
    message = commands.run(7, "GOLD", "1000000", "1")
    check("postać offline: Yang zapisany w bazie", "w bazie" in message, message)
    message = commands.run(7, "LEVEL", "42", "1")
    check("postać offline: poziom zapisany w bazie", "w bazie" in message, message)
    try:
        commands.run(7, "WARP", "474300", "954800")
        failures.append(("teleport dla postaci offline powinien być odrzucony", ""))
        print("  ZLE teleport offline nie został odrzucony")
    except commands.CommandError as error:
        # Whichever branch it takes - the quest said "offline", nothing has
        # ever answered here, or it simply timed out - the refusal has to name
        # the character and say the action needs it in the world.
        text = str(error).lower()
        check("teleport dla postaci offline odrzucony z powodem",
              "botarek7" in text or "niedostępne" in text, str(error))
finally:
    for entry in pending:
        index = next(i for i, (f, _r) in enumerate(test_pages.ROUTES) if f == entry[0])
        test_pages.ROUTES[index] = entry

# --- warp the operator to a bot ----------------------------------------------
print("\nTeleport operatora do bota:")

commands.WARP_ME_WAIT_SECONDS = 1.5
name = commands.warp_operator_to(851670, 291602)
check("teleport operatora zwraca nazwe postaci ktora odpowiedziala",
      name == "TestGracz", str(name))

# No recent human character at all: refused with a reason, not a silent no-op.
no_humans = [(fragment, rows) for fragment, rows in test_pages.ROUTES
             if fragment == "SELECT name FROM player.player WHERE NOT"]
for entry in no_humans:
    test_pages.ROUTES[test_pages.ROUTES.index(entry)] = (entry[0], [])
try:
    commands.warp_operator_to(1, 1)
    failures.append(("teleport bez graczy powinien byc odrzucony", ""))
    print("  ZLE teleport bez graczy nie zostal odrzucony")
except commands.CommandError as error:
    check("teleport bez ostatnio aktywnych graczy odrzucony", "gracz" in str(error).lower(), str(error))
finally:
    for entry in no_humans:
        index = next(i for i, (f, _r) in enumerate(test_pages.ROUTES) if f == entry[0])
        test_pages.ROUTES[index] = entry

# --- game master ranks -------------------------------------------------------
print("\nRangi GM:")

try:
    gm.set_rank(7, "NIE_MA_TAKIEJ")
    failures.append(("nieznana ranga powinna być odrzucona", ""))
    print("  ZLE nieznana ranga nie została odrzucona")
except ValueError as error:
    check("nieznana ranga odrzucona", True, str(error))

message = gm.set_rank(7, "LOW_WIZARD")
check("nadanie rangi daje komunikat", "Pomocnik" in message, message)
message = gm.set_rank(7, "")
check("odebranie rangi daje komunikat", "nie ma już rangi" in message, message)
check("odczyt rangi z bazy", gm.rank_of("botarek7") == "LOW_WIZARD", str(gm.rank_of("botarek7")))

# --- the game language -------------------------------------------------------
print("\nJęzyk gry:")

check("domyślnie angielski", spool.game_language() == "en", spool.game_language())
try:
    spool.queue_language("klingon")
    failures.append(("nieznany język powinien być odrzucony", ""))
except ValueError:
    check("nieznany język odrzucony", True)
try:
    spool.queue_language("en")
    failures.append(("ten sam język powinien być zgłoszony jako brak zmiany", ""))
except FileExistsError as error:
    check("ten sam język zgłoszony jako brak zmiany", True, str(error))

spool.queue_language("pl")
request_file = os.path.join(SPOOL, "lang.request")
check("zlecenie zmiany języka zapisane", os.path.exists(request_file))
body = open(request_file, encoding="utf-8").read()
check("zlecenie niesie kod języka", "lang=pl" in body, body)
check("zlecenie oznaczone prefiksem tuike-", f"id={config.REQUEST_PREFIX}" in body, body)

# --- the workers -------------------------------------------------------------
print("\nWorkery:")

from tuike.workers import collector as collector_worker  # noqa: E402
from tuike import db, grants  # noqa: E402

try:
    previous, bots, maps = collector_worker.collect(db.connect(), None)
    check("kolektor wykonuje przebieg bez bledu", True)
except Exception:
    failures.append(("kolektor", traceback.format_exc()))
    print("  ZLE kolektor rzucil wyjatek")

metrics = collector_worker.host_metrics(None)
check("host_metrics bez /proc zwraca zera zamiast rzucac", metrics[1] == 0, str(metrics))

try:
    grants.tick(db.connect())
    check("tick workera nadan przechodzi", True)
except Exception:
    failures.append(("grants.tick", traceback.format_exc()))
    print("  ZLE grants.tick rzucil wyjatek")

try:
    grants.beat("")
    check("heartbeat workera nadan przechodzi", True)
except Exception:
    failures.append(("grants.beat", traceback.format_exc()))
    print("  ZLE grants.beat rzucil wyjatek")

# Criteria parsing is what the operator most easily gets wrong.
check("kryteria: pusty formularz to brak warunkow",
      grants.criteria_from({"job": ""}) == {}, str(grants.criteria_from({"job": ""})))
check("kryteria: opis po polsku",
      grants.criteria_text({"min_level": 30, "job": 2}) == "Lv ≥ 30 · Sura",
      grants.criteria_text({"min_level": 30, "job": 2}))
for form, why in (
    ({"job": "", "min_level": "50", "max_level": "10"}, "min > max"),
    ({"job": "9"}, "nieznana klasa"),
    ({"job": "", "min_level": "abc"}, "nieliczbowy poziom"),
    ({"job": "", "min_level": "9999"}, "poziom poza zakresem"),
):
    try:
        grants.criteria_from(form)
        failures.append((f"kryteria: {why} powinny byc odrzucone", str(form)))
        print(f"  ZLE kryteria: {why} nie zostaly odrzucone")
    except grants.GrantError:
        check(f"kryteria: {why} odrzucone", True)

print()
if failures:
    print(f"NIEPOWODZENIA: {len(failures)}")
    for name, detail in failures:
        print("=" * 70)
        print(name)
        print(detail)
    sys.exit(1)
print("Wszystkie testy zapisu przeszly.")
