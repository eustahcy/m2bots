"""Test aktualizatora z GitHuba — na prawdziwym repozytorium, bez Dockera.

Buduje sobie własną atrapę: puste repozytorium "zdalne", drzewo "PC", które do
niego wypycha, i klon udający serwer. Potem sprawdza dokładnie te rzeczy, na
których ten aktualizator może się wyłożyć po cichu:

  * czy status czyta stan lokalnie (poll co 4 s nie ma prawa chodzić po sieci),
  * czy „Sprawdź" widzi nowe commity i poprawnie je opisuje,
  * czy wykrywa, że aktualizacja podmienia kod samego panelu,
  * czy `reset --hard` NIE rusza .env — to jedyny plik, którego utrata
    oznacza serwer bez dostępu do własnej bazy,
  * czy kroki docker compose lecą w dobrej kolejności i tylko po udanym resecie.

Kroki `docker compose` są podmienione na atrapę; Docker nie jest potrzebny.

    python3 git_test.py
"""
import hashlib
import os
import secrets
import shutil
import subprocess
import sys
import tempfile
import time
from pathlib import Path

HERE = Path(__file__).resolve().parent

if not shutil.which("git"):
    print("POMINIĘTO: git nie jest zainstalowany.")
    sys.exit(0)

root = Path(tempfile.mkdtemp(prefix="m2-git-test-"))
remote, work, vps = root / "remote.git", root / "pc", root / "vps"
state_dir = root / "state"

failures = []


def check(label, condition, detail=""):
    print(f"  {'OK  ' if condition else 'FAIL'}  {label}{(' — ' + str(detail)) if detail else ''}")
    if not condition:
        failures.append(label)


def git(*args, cwd):
    return subprocess.run(["git", *args], cwd=str(cwd), check=True,
                          capture_output=True, text=True).stdout.strip()


def write(path, text):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text, encoding="utf-8")


def commit_and_push(message):
    git("add", "-A", cwd=work)
    git("commit", "-q", "-m", message, cwd=work)
    git("push", "-q", "origin", "main", cwd=work)


try:
    # --- atrapa: zdalne repozytorium, drzewo PC, pierwszy import -------------
    subprocess.run(["git", "init", "-q", "--bare", str(remote)], check=True)
    work.mkdir()
    git("init", "-q", "-b", "main", cwd=work)
    git("config", "user.email", "test@example.com", cwd=work)
    git("config", "user.name", "Test", cwd=work)
    git("remote", "add", "origin", str(remote), cwd=work)

    write(work / "VERSION", "2.0.42\n")
    write(work / "linux-port/docker/ENGINE", "mt2009\n")
    write(work / "linux-port/docker/docker-compose.yml", "services: {}\n")
    write(work / "linux-port/docker/.env.example", "M2_DB_PASSWORD=zmien-mnie\n")
    # Najlepiej ten sam .gitignore, którym broni się prawdziwe repozytorium —
    # wtedy test sprawdza regułę, która naprawdę obowiązuje. Ale ten plik bywa
    # uruchamiany z katalogu roboczego na serwerze (~/vps-panel), gdzie drzewa
    # repo nad nim nie ma; wtedy wystarczy reguła, o którą tu chodzi.
    project_ignore = HERE.parent.parent / ".gitignore"
    if project_ignore.is_file():
        shutil.copy(project_ignore, work / ".gitignore")
        print(f"(.gitignore z repozytorium: {project_ignore})")
    else:
        write(work / ".gitignore", ".env\nbackups/\n")
        print("(.gitignore zastępczy — drzewa repozytorium nie ma nad tym plikiem)")
    commit_and_push("Pierwszy import")

    # --- atrapa: serwer, czyli klon plus własny .env -------------------------
    subprocess.run(["git", "clone", "-q", "-b", "main", str(remote), str(vps)], check=True)
    env_file = vps / "linux-port/docker/.env"
    write(env_file, "M2_DB_PASSWORD=supersekret-z-vpsa\n")

    # --- dwa nowe commity z PC ----------------------------------------------
    write(work / "linux-port/docker/game/nowy.cpp", "int main() { return 0; }\n")
    commit_and_push("Zmiana w silniku")
    write(work / "linux-port/vps-panel/app.py", "# panel\n")
    commit_and_push("Zmiana w panelu")

    # --- panel ---------------------------------------------------------------
    salt, password = secrets.token_hex(16), "test-password"
    os.environ["M2_VPS_PANEL_PASSWORD_HASH"] = \
        f"{salt}:{hashlib.sha256((salt + password).encode()).hexdigest()}"
    os.environ["M2_VPS_PANEL_SECRET"] = secrets.token_hex(32)
    os.environ["M2_VPS_PANEL_STATE_DIR"] = str(state_dir)
    os.environ["M2_COMPOSE_DIR"] = str(vps / "linux-port" / "docker")
    os.environ["M2_GIT_DIR"] = str(vps)
    os.environ["M2_GIT_BRANCH"] = "main"

    sys.path.insert(0, str(HERE))
    import app as panel

    print("1. Status czyta lokalnie, bez sieci")
    status = panel.git_status()
    check("widzi repozytorium", status["ok"], status.get("error"))
    check("gałąź to main", status.get("branch") == "main", status.get("branch"))
    # Klon powstał przed tamtymi commitami, więc jego ref zdalny ich nie zna.
    # Gdyby git_status sam pobierał, zobaczyłby 2 i ten warunek by padł.
    check("behind = 0 dopóki nikt nie pobrał", status.get("behind") == 0, status.get("behind"))

    print("2. „Sprawdź aktualizacje” pobiera i opisuje, co przyjdzie")
    client = panel.app.test_client()
    client.post("/login", data={"password": password})
    client.get("/")  # to strona główna wystawia token CSRF
    with client.session_transaction() as sess:
        csrf = sess["csrf"]

    data = client.post("/api/git/check", headers={"X-CSRF": csrf}).get_json()
    check("endpoint odpowiada OK", data.get("ok") is True, data.get("error"))
    check("wykrył 2 nowe commity", data["git"]["behind"] == 2, data["git"]["behind"])
    check("wypisał commity", len(data["incoming"]["commits"]) == 2, data["incoming"]["commits"])
    paths = [f["path"] for f in data["incoming"]["files"]]
    check("widzi zmieniony plik silnika", "linux-port/docker/game/nowy.cpp" in paths, paths)
    check("ostrzega, że zmienia się kod panelu", data["panel_changed"] is True, paths)

    print("3. Zastosowanie aktualizacji")
    before = env_file.read_text(encoding="utf-8")
    calls = []
    panel._compose = lambda *a, **kw: (calls.append(a) or 0)
    check("akcja wystartowała", panel.action_git_update() is True)
    for _ in range(400):
        if not panel._action_state["running"]:
            break
        time.sleep(0.05)
    check("akcja się zakończyła", panel._action_state["running"] is False)
    check("kolejność: build game, potem up -d",
          calls == [("build", "game"), ("up", "-d")], calls)
    check("po aktualizacji behind = 0", panel.git_status().get("behind") == 0)
    check("nowy plik jest na dysku", (vps / "linux-port/docker/game/nowy.cpp").exists())
    check(".env przetrwał reset --hard", env_file.read_text(encoding="utf-8") == before)
    log = (state_dir / "action.log").read_text(encoding="utf-8", errors="replace")
    check("log mówi, jak przebudować panel", "install.sh" in log)

    print("4. Nieudany reset nie przechodzi do kompilacji")
    # Gałąź, której nie ma: fetch musi paść, a docker compose nie ruszyć wcale.
    panel.GIT_BRANCH, calls[:] = "nie-ma-takiej-galezi", []
    panel.action_git_update()
    for _ in range(400):
        if not panel._action_state["running"]:
            break
        time.sleep(0.05)
    check("przy błędzie fetcha nie buduje niczego", calls == [], calls)
    panel.GIT_BRANCH = "main"

    print("5. Bez nagłówka CSRF endpoint odmawia")
    check("brak CSRF -> 403", client.post("/api/git/check").status_code == 403)

finally:
    shutil.rmtree(root, ignore_errors=True)

print()
if failures:
    print(f"NIEPOWODZENIA ({len(failures)}): " + ", ".join(failures))
    sys.exit(1)
print("OK: aktualizator z GitHuba przeszedł wszystkie sprawdzenia.")
