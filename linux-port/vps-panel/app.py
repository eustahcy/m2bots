"""M2 VPS Panel — the physical power switch for one Metin2 Playerbots stack.

Runs natively on the VPS host, outside Docker, on purpose: the whole point of
this panel is to say "the stack is down" and still offer a Start button, which
a panel running *inside* the stack it controls cannot do for itself. Tuike,
Seban and the classic panel already cover game data (characters, economy,
guilds...); this one covers exactly the four things an operator does from
outside the game: start, stop, restart, rebuild — plus enough status to know
which one is needed.

One file, on purpose: this is not a second game-data panel, and it has no
templates folder, no database migrations and no background workers of its
own. Config comes from environment variables the systemd unit sets
(see m2-vps-panel.service / install.sh), never hand-edited here.
"""
import glob
import hashlib
import hmac
import json
import os
import secrets
import socket
import subprocess
import threading
import time
from datetime import datetime, timedelta
from pathlib import Path

from flask import Flask, redirect, render_template_string, request, session, url_for

try:
    import pymysql
except ImportError:  # pragma: no cover - installer always adds it
    pymysql = None

# --- configuration -----------------------------------------------------
# Where the docker-compose project this panel controls lives. The installer
# writes this into the systemd unit's environment file after finding it.
COMPOSE_DIR = Path(os.environ.get("M2_COMPOSE_DIR", "/home/debian/metin2-playerbots/linux-port/docker"))
COMPOSE_FILE = COMPOSE_DIR / "docker-compose.yml"
ENV_FILE = COMPOSE_DIR / ".env"

# The server folder: two levels above the compose project (VERSION, CHANGELOG.md,
# linux-port/, files/). Used for the version badge whether or not git is in play.
SERVER_ROOT = Path(os.environ.get("M2_SERVER_ROOT") or COMPOSE_DIR.parent.parent)

# --- the git checkout this panel updates from ------------------------------
# Opt-in, deliberately: only git-setup.sh sets M2_GIT_DIR, and until it has,
# this panel offers no update button at all.
#
# The first version of this defaulted to "the server folder, if it happens to
# be a checkout". On the real VPS that folder turned out to be a checkout of
# the UPSTREAM project with 77 locally modified tracked files - every ZIP
# deploy this server had ever received. A helpful-looking "Zastosuj
# aktualizację" would have reset --hard onto somebody else's main and thrown
# all of it away. A panel must never discover a destination by accident.
GIT_ENABLED = bool(os.environ.get("M2_GIT_DIR"))
GIT_DIR = Path(os.environ.get("M2_GIT_DIR") or SERVER_ROOT)
GIT_REMOTE = os.environ.get("M2_GIT_REMOTE", "origin")
GIT_BRANCH = os.environ.get("M2_GIT_BRANCH", "main")

# This panel runs as root over a checkout owned by the login user, which git
# 2.35.2+ refuses to touch ("detected dubious ownership") unless the path is
# marked safe. git-setup.sh also writes this into root's global config, but
# passing it per-invocation means the panel still works on a box where that
# config was lost - a refusal here would look exactly like "no updates".
GIT_BASE = ["git", "-c", f"safe.directory={GIT_DIR}", "-C", str(GIT_DIR)]
# Never let git stop for a username/password prompt: a private remote added by
# hand would otherwise hang the fetch until its timeout instead of failing with
# something an operator can read.
GIT_ENV = {**os.environ, "GIT_TERMINAL_PROMPT": "0", "GIT_ASKPASS": "", "LC_ALL": "C"}

PANEL_PORT = int(os.environ.get("M2_VPS_PANEL_PORT", "9797"))
# A salted SHA-256 of the operator's password, "salt:hash" - never the
# password itself. The installer prints the password once and forgets it.
PANEL_PASSWORD_HASH = os.environ.get("M2_VPS_PANEL_PASSWORD_HASH", "")
SESSION_SECRET = os.environ.get("M2_VPS_PANEL_SECRET") or secrets.token_hex(32)
# The systemd EnvironmentFile - rewritten in place when the password changes,
# the same file the installer wrote it into originally.
PANEL_ENV_FILE = Path(os.environ.get("M2_VPS_PANEL_ENV_FILE", "/etc/m2-vps-panel.env"))

# The other panels this stack already runs, read from the same .env this one
# controls rather than duplicated here - a port changed in one place shows up
# correctly in both. (host, default port) pairs; the host part is unused,
# kept only so the tuple shape matches PORT_ENV_KEYS below at a glance.
PANEL_LINKS = (
    ("Tuike", "M2_TUIKE_PANEL_PORT", 7799),
    ("Seban Panel", "M2_SEBAN_PANEL_PORT", 7790),
    ("Panel klasyczny", "M2_PANEL_PUBLIC_PORT", 7788),
    ("ItemShop", "M2_ITEMSHOP_PUBLIC_PORT", 7791),
)

# The two ports that answer, in order, while the world is coming up - the
# same pair the classic panel already uses to decide "is the game up".
STATUS_PORTS = [11000, 13000]
STATUS_HOST = "127.0.0.1"
PORT_TIMEOUT = 1.5

# A bot's live status line goes stale if nothing rewrote its file for this
# long - the game process died, or never started. Same floor Tuike uses.
STATUS_MAX_AGE_SECONDS = 30

# Shared by every action, not only a build - see _run_action.
ACTION_LOG = Path(os.environ.get("M2_VPS_PANEL_STATE_DIR", "/var/lib/m2-vps-panel")) / "action.log"
ACTION_LOG.parent.mkdir(parents=True, exist_ok=True)

app = Flask(__name__)
app.secret_key = SESSION_SECRET
app.config.update(SESSION_COOKIE_HTTPONLY=True, SESSION_COOKIE_SAMESITE="Lax")

# One action at a time - a second "compile" clicked mid-build would otherwise
# spawn a second `docker compose build` fighting the first for the same
# layer cache, and a "stop" mid-restart would leave the compose CLI's own
# state file the two invocations race on in an undefined place.
_action_lock = threading.Lock()
_action_state = {"running": False, "label": "", "started": 0.0}


# --- small helpers -------------------------------------------------------
def read_env_file(path):
    """A shell-less .env reader: KEY=value lines, '#' comments, done."""
    values = {}
    try:
        for line in path.read_text(encoding="utf-8", errors="replace").splitlines():
            line = line.strip()
            if not line or line.startswith("#") or "=" not in line:
                continue
            key, _, value = line.partition("=")
            values[key.strip()] = value.strip().strip('"').strip("'")
    except OSError:
        pass
    return values


def update_env_file(path, key, value):
    """Rewrite one KEY=value line in place, appending it if it is new.

    Written beside the file and renamed over it, the way Tuike's own spool
    writer does it, so a process re-reading this file on its own schedule
    never sees a half-written line.
    """
    lines = []
    try:
        lines = path.read_text(encoding="utf-8").splitlines()
    except OSError:
        pass
    found = False
    for i, line in enumerate(lines):
        if line.startswith(f"{key}="):
            lines[i] = f"{key}={value}"
            found = True
            break
    if not found:
        lines.append(f"{key}={value}")
    temporary = path.with_suffix(path.suffix + ".new")
    temporary.write_text("\n".join(lines) + "\n", encoding="utf-8")
    temporary.chmod(0o600)
    os.replace(temporary, path)


def related_panels():
    """The other panels on this stack, as (label, url) - only the ones this
    request could plausibly reach, using the host the browser is already
    using to reach this panel (works the same over an IP or a domain)."""
    host = request.host.split(":", 1)[0]
    env = read_env_file(ENV_FILE)
    links = []
    for label, env_key, default_port in PANEL_LINKS:
        port = env.get(env_key, "").strip() or default_port
        links.append((label, f"http://{host}:{port}"))
    return links


def port_open(host, port, timeout=PORT_TIMEOUT):
    try:
        with socket.create_connection((host, port), timeout=timeout):
            return True
    except OSError:
        return False


def game_status():
    """Up, coming up, or down - by which of the two ports answer."""
    auth = port_open(STATUS_HOST, STATUS_PORTS[0])
    world = port_open(STATUS_HOST, STATUS_PORTS[1])
    if auth and world:
        return "up", "Serwer działa"
    if auth:
        return "partial", "Logowanie działa, świat wstaje"
    return "down", "Serwer nie odpowiada"


def server_version():
    """The VERSION file next to linux-port/ — what this server is running."""
    try:
        return SERVER_ROOT.joinpath("VERSION").read_text(encoding="utf-8").strip() or None
    except OSError:
        return None


def server_brand():
    """The name shown in the header. M2_BRAND is the stack's own setting, so a
    server that already named itself does not have to name itself twice."""
    return (os.environ.get("M2_VPS_PANEL_BRAND")
            or read_env_file(ENV_FILE).get("M2_BRAND", "").strip()
            or "Metin2 Playerbots")


def game_uptime():
    """How long the game container has been up, as docker itself words it."""
    prefix = read_env_file(ENV_FILE).get("M2_CONTAINER_PREFIX", "metin2")
    try:
        out = subprocess.run(
            ["docker", "inspect", "-f", "{{.State.StartedAt}}", f"{prefix}-game"],
            capture_output=True, text=True, timeout=10, check=True)
    except (subprocess.SubprocessError, OSError):
        return None
    stamp = out.stdout.strip()
    if not stamp:
        return None
    try:
        # Docker prints nanoseconds; fromisoformat wants at most microseconds.
        cleaned = stamp.replace("Z", "+00:00")
        if "." in cleaned:
            head, _, tail = cleaned.partition(".")
            fraction, _, offset = tail.partition("+")
            cleaned = f"{head}.{fraction[:6]}+{offset}" if offset else f"{head}.{fraction[:6]}"
        started = datetime.fromisoformat(cleaned)
    except ValueError:
        return None
    delta = datetime.now(started.tzinfo) - started
    days, seconds = delta.days, delta.seconds
    if days:
        return f"{days} dni {seconds // 3600} godz."
    if seconds >= 3600:
        return f"{seconds // 3600} godz. {(seconds % 3600) // 60} min"
    return f"{max(seconds, 0) // 60} min"


def game_var_mountpoint():
    """Where the game-var Docker volume actually lives on this host."""
    env = read_env_file(ENV_FILE)
    prefix = env.get("M2_CONTAINER_PREFIX", "")
    if not prefix:
        return None
    try:
        out = subprocess.run(
            ["docker", "volume", "inspect", f"{prefix}_game-var", "--format", "{{.Mountpoint}}"],
            capture_output=True, text=True, timeout=10, check=True,
        )
        path = Path(out.stdout.strip())
        return path if path.is_dir() else None
    except (subprocess.SubprocessError, OSError):
        return None


def live_character_count():
    """Bots currently ticking, by counting fresh playerbot_status.tsv rows.

    The file's own mtime is the freshness signal, not the updated_ms column
    inside it - that column is the game process's own uptime clock, not wall
    time, and this reads it from outside the game entirely.
    """
    mount = game_var_mountpoint()
    if not mount:
        return None
    total = 0
    found_any = False
    cutoff = time.time() - STATUS_MAX_AGE_SECONDS
    for tsv in glob.glob(str(mount / "channel1" / "*" / "playerbot_status.tsv")):
        found_any = True
        try:
            if os.path.getmtime(tsv) < cutoff:
                continue
            with open(tsv, "r", encoding="utf-8", errors="replace") as handle:
                total += max(0, sum(1 for _ in handle) - 1)  # minus the header row
        except OSError:
            continue
    return total if found_any else None


def db_totals():
    """Registered accounts and characters - context, not a "who's online" claim."""
    if pymysql is None:
        return None
    env = read_env_file(ENV_FILE)
    password = env.get("M2_DB_PASSWORD", "")
    user = env.get("M2_DB_USER", "metin2")
    if not password:
        return None
    try:
        conn = pymysql.connect(host="127.0.0.1", port=3306, user=user, password=password,
                                connect_timeout=3, charset="utf8mb4")
        with conn:
            with conn.cursor() as cursor:
                cursor.execute("SELECT COUNT(*) FROM player.player")
                characters = cursor.fetchone()[0]
                cursor.execute("SELECT COUNT(*) FROM account.account")
                accounts = cursor.fetchone()[0]
        return {"characters": characters, "accounts": accounts}
    except Exception:
        return None


def host_metrics():
    """CPU load, memory, disk - straight from /proc, no container in the way."""
    metrics = {}
    # Broad except throughout: this dashboard tile must never turn a reading
    # it could not take (an unexpected /proc layout, a platform without one at
    # all) into a 500 for the whole status endpoint.
    try:
        load1, _, _ = Path("/proc/loadavg").read_text().split()[:3]
        metrics["load1"] = float(load1)
    except Exception:
        metrics["load1"] = None
    try:
        fields = {}
        for line in Path("/proc/meminfo").read_text().splitlines():
            if ":" in line:
                key, value = line.split(":", 1)
                fields[key] = int(value.strip().split()[0])
        total = fields.get("MemTotal", 0)
        available = fields.get("MemAvailable", fields.get("MemFree", 0))
        metrics["ram_used_mb"] = (total - available) // 1024
        metrics["ram_total_mb"] = total // 1024
    except Exception:
        metrics["ram_used_mb"] = metrics["ram_total_mb"] = None
    try:
        disk = os.statvfs("/")
        metrics["disk_used_gb"] = round((disk.f_blocks - disk.f_bavail) * disk.f_frsize / 1e9, 1)
        metrics["disk_total_gb"] = round(disk.f_blocks * disk.f_frsize / 1e9, 1)
    except Exception:
        metrics["disk_used_gb"] = metrics["disk_total_gb"] = None
    return metrics


def containers_status():
    """One row per compose service: name and whether it is running."""
    try:
        out = subprocess.run(
            ["docker", "compose", "--project-directory", str(COMPOSE_DIR), "ps", "--format", "json"],
            capture_output=True, text=True, timeout=15, check=True,
        )
    except (subprocess.SubprocessError, OSError):
        return None
    rows = []
    for line in out.stdout.splitlines():
        line = line.strip()
        if not line:
            continue
        try:
            data = json.loads(line)
        except ValueError:
            continue
        rows.append({"name": data.get("Service", data.get("Name", "?")),
                     "state": data.get("State", "?"),
                     "health": data.get("Health", "")})
    rows.sort(key=lambda row: row["name"])
    return rows


# --- actions: the five buttons --------------------------------------------
# --- git: reading the checkout --------------------------------------------
def _git(*args, timeout=30):
    """Run one read-only git command in the checkout. Returns (rc, stdout)."""
    try:
        proc = subprocess.run(GIT_BASE + list(args), capture_output=True, text=True,
                              timeout=timeout, env=GIT_ENV)
    except (OSError, subprocess.SubprocessError):
        return 1, ""
    return proc.returncode, (proc.stdout or "").strip()


def git_available():
    return GIT_ENABLED and (GIT_DIR / ".git").exists()


def git_status():
    """What this checkout is, and how far it is from the remote.

    Deliberately does not fetch. /api/status polls this every four seconds;
    a network round trip on that path would make the whole dashboard stall
    whenever GitHub is slow. The behind/ahead numbers are therefore as fresh
    as the last "Sprawdź aktualizacje" - which is the button's whole job.
    """
    if not GIT_ENABLED:
        return {"ok": False, "configured": False,
                "error": "Aktualizacje z gita nie są włączone na tym serwerze. "
                         "Włącza je linux-port/vps-panel/git-setup.sh."}
    if not git_available():
        return {"ok": False, "configured": True,
                "error": f"{GIT_DIR} nie jest repozytorium git "
                         f"(uruchom linux-port/vps-panel/git-setup.sh)"}
    rc, branch = _git("rev-parse", "--abbrev-ref", "HEAD")
    if rc != 0:
        return {"ok": False, "error": "git nie odpowiada w tym katalogu"}
    _, commit = _git("rev-parse", "--short", "HEAD")
    _, subject = _git("log", "-1", "--pretty=%s")
    _, when = _git("log", "-1", "--pretty=%cd", "--date=format:%Y-%m-%d %H:%M")
    _, remote_url = _git("remote", "get-url", GIT_REMOTE)
    _, dirty = _git("status", "--porcelain", "--untracked-files=no")

    tracking = f"{GIT_REMOTE}/{GIT_BRANCH}"
    behind = ahead = None
    rc, counts = _git("rev-list", "--left-right", "--count", f"HEAD...{tracking}")
    if rc == 0 and "\t" in counts:
        left, _, right = counts.partition("\t")
        ahead, behind = _as_int(left), _as_int(right)

    return {
        "ok": True,
        "branch": branch,
        "commit": commit,
        "subject": subject,
        "date": when,
        "remote": remote_url,
        "tracking": tracking,
        "behind": behind,
        "ahead": ahead,
        # Tracked files edited on the server itself. Untracked ones are
        # excluded on purpose: .env and the panel's own byproducts are
        # ignored, and an operator's stray file is not a reason to warn.
        "dirty": len([ln for ln in dirty.splitlines() if ln.strip()]),
    }


def _as_int(text):
    try:
        return int(text.strip())
    except (TypeError, ValueError):
        return None


def git_incoming():
    """The commits and files an update would bring, newest first."""
    tracking = f"{GIT_REMOTE}/{GIT_BRANCH}"
    _, log = _git("log", "--no-decorate", "--pretty=%h %ad %s", "--date=short",
                  f"HEAD..{tracking}", "-n", "60")
    _, files = _git("diff", "--name-status", f"HEAD..{tracking}")
    file_rows = []
    for line in files.splitlines()[:300]:
        status, _, path = line.partition("\t")
        if path:
            file_rows.append({"status": status.strip()[:1], "path": path.strip()})
    return {
        "commits": [ln for ln in log.splitlines() if ln.strip()],
        "files": file_rows,
        "files_total": len([ln for ln in files.splitlines() if ln.strip()]),
    }


def git_touches_panel(paths):
    """True if an update would replace this panel's own source.

    The panel runs from /opt/m2-vps-panel, which install.sh copied out of the
    checkout - so a git update changes the repository's copy and leaves the
    running one alone. Detecting it is the difference between an operator who
    knows to re-run install.sh and one who wonders why their fix did nothing.
    """
    return any(p["path"].startswith("linux-port/vps-panel/") for p in paths)


def _compose(*args, log_to=None):
    cmd = ["docker", "compose", "--project-directory", str(COMPOSE_DIR)] + list(args)
    if log_to:
        with open(log_to, "ab") as handle:
            handle.write(f"\n$ {' '.join(cmd)}\n".encode())
            handle.flush()
            proc = subprocess.run(cmd, stdout=handle, stderr=subprocess.STDOUT, timeout=1800)
            return proc.returncode
    proc = subprocess.run(cmd, capture_output=True, text=True, timeout=300)
    return proc.returncode, proc.stdout, proc.stderr


def _run_action(label, fn):
    """fn runs in a background thread and must return an exit code; the UI
    polls /api/status meanwhile and /api/action-log for its output.

    Every action - not only the compile button - logs here now. A start,
    stop or restart that silently threw its output away was the one thing
    an operator could not diagnose when it went wrong ("brakuje logow
    uruchamiania", operator report, 14 September); a fresh log per run means
    a stalled action never leaves an old one looking current.
    """
    if not _action_lock.acquire(blocking=False):
        return False
    _action_state.update(running=True, label=label, started=time.time())
    ACTION_LOG.write_text(f"=== {label} - rozpoczęto {datetime.now().isoformat(timespec='seconds')} ===\n")

    def worker():
        try:
            code = fn()
            with open(ACTION_LOG, "ab") as handle:
                handle.write(f"\n=== {label}: zakończono, kod wyjścia {code} ===\n".encode())
        except Exception as exc:
            with open(ACTION_LOG, "ab") as handle:
                handle.write(f"\n=== {label}: BŁĄD - {exc} ===\n".encode())
        finally:
            _action_state["running"] = False
            _action_lock.release()

    threading.Thread(target=worker, daemon=True).start()
    return True


def action_start():
    return _run_action("Uruchamianie", lambda: _compose("up", "-d", log_to=ACTION_LOG))


def action_stop():
    return _run_action("Zatrzymywanie", lambda: _compose("stop", log_to=ACTION_LOG))


def action_restart_game():
    # up -d --force-recreate, not restart: compose's own restart bounces a
    # container in place without necessarily waiting for a dependency's
    # healthcheck first, and a game container recreated before mariadb had
    # answered is exactly the kind of half-started service an operator saw
    # after restarting the whole stack at once. force-recreate goes through
    # the same dependency/health gate `up` always does, and still bounces a
    # container that had nothing else change.
    return _run_action("Restart gry",
                        lambda: _compose("up", "-d", "--force-recreate", "game", log_to=ACTION_LOG))


def action_restart_stack():
    return _run_action("Restart całego stosu",
                        lambda: _compose("up", "-d", "--force-recreate", log_to=ACTION_LOG))


def action_compile():
    return _run_action("Kompilacja silnika", lambda: _compose("build", "game", log_to=ACTION_LOG))


def _git_logged(args, log_to, timeout=900):
    """A git command whose output belongs in the action log, like _compose's."""
    cmd = GIT_BASE + list(args)
    with open(log_to, "ab") as handle:
        handle.write(f"\n$ {' '.join(cmd)}\n".encode())
        handle.flush()
        try:
            proc = subprocess.run(cmd, stdout=handle, stderr=subprocess.STDOUT,
                                  timeout=timeout, env=GIT_ENV)
        except subprocess.TimeoutExpired:
            handle.write(b"\n(git przekroczyl limit czasu)\n")
            return 124
        return proc.returncode


def _note(text):
    with open(ACTION_LOG, "ab") as handle:
        handle.write(f"\n{text}\n".encode())


def git_fetch():
    """Bring the remote's refs up to date. Returns (ok, message)."""
    if not git_available():
        return False, git_status().get("error", "git nie jest skonfigurowany")
    rc, _ = _git("fetch", GIT_REMOTE, GIT_BRANCH, "--prune", timeout=300)
    if rc != 0:
        return False, ("Nie udało się pobrać zmian z GitHuba. Sprawdź sieć na VPS "
                       f"i adres zdalny ({GIT_REMOTE}).")
    return True, ""


def action_git_update():
    """fetch -> reset --hard -> build game -> up -d, in the action log.

    reset --hard, not pull: the checkout on a server is not a place anybody
    should be committing from, and a merge conflict raised at 3am by `pull`
    on a box with no editor is the worst possible failure mode. Whatever the
    remote branch says is what the server runs.

    Untracked files survive it deliberately (no `git clean`): .env, the
    backups folder and the launcher's local state are all untracked, and a
    clean would take an operator's own files with them. Files *deleted* in
    the repository are still removed, because those are tracked.
    """
    if not git_available():
        return False

    def run():
        code = _git_logged(["fetch", GIT_REMOTE, GIT_BRANCH, "--prune"], ACTION_LOG, timeout=600)
        if code != 0:
            _note("=== przerwano: pobranie z GitHuba nie powiodło się, nic nie zmieniono ===")
            return code

        # Recomputed after the fetch, not taken from the check button: that
        # click may be minutes old, and what matters is what is landing now.
        incoming = git_incoming()
        panel_changed = git_touches_panel(incoming["files"])
        if not incoming["commits"]:
            _note("=== nie ma czego aktualizować: ten checkout jest już na "
                  f"{GIT_REMOTE}/{GIT_BRANCH} ===")
            return 0
        _note(f"--- {len(incoming['commits'])} nowych commitów, "
              f"{incoming['files_total']} zmienionych plików ---")

        code = _git_logged(["reset", "--hard", f"{GIT_REMOTE}/{GIT_BRANCH}"], ACTION_LOG)
        if code != 0:
            _note("=== przerwano: reset nie powiódł się, stos nadal chodzi na starej wersji ===")
            return code

        code = _compose("build", "game", log_to=ACTION_LOG)
        if code != 0:
            _note("=== kompilacja nie powiodła się. Pliki są już nowe, ale kontener "
                  "gry chodzi na starym obrazie - popraw błąd i kliknij Kompiluj ===")
            return code

        code = _compose("up", "-d", log_to=ACTION_LOG)
        if panel_changed:
            _note("=== UWAGA: ta aktualizacja zmieniła kod panelu VPS. Panel chodzi "
                  "z /opt/m2-vps-panel i nie podmienia się sam. Uruchom na VPS:\n"
                  f"    sudo sh {GIT_DIR}/linux-port/vps-panel/install.sh ===")
        return code

    return _run_action("Aktualizacja z Git", run)


# --- auth ------------------------------------------------------------------
def hash_password(password, salt=None):
    salt = salt or secrets.token_hex(16)
    digest = hashlib.sha256((salt + password).encode()).hexdigest()
    return f"{salt}:{digest}"


def check_password(password):
    if not PANEL_PASSWORD_HASH or ":" not in PANEL_PASSWORD_HASH:
        return False
    salt, digest = PANEL_PASSWORD_HASH.split(":", 1)
    return hmac.compare_digest(hash_password(password, salt).split(":", 1)[1], digest)


def change_password(new_password):
    """Persist a new password and restart so every worker picks it up.

    gunicorn runs this app as several separate processes sharing one port;
    a Python-level global set here is only ever seen by the worker that
    served this request. The env file is the one thing every worker reads
    at boot, so the honest way to make the change stick everywhere is to
    write it there and then actually reboot the service - not pretend the
    change is live in workers it never reached.
    """
    global PANEL_PASSWORD_HASH
    PANEL_PASSWORD_HASH = hash_password(new_password)
    update_env_file(PANEL_ENV_FILE, "M2_VPS_PANEL_PASSWORD_HASH", PANEL_PASSWORD_HASH)

    def restart_soon():
        time.sleep(1.5)  # long enough for this request's own response to go out
        subprocess.run(["systemctl", "restart", "m2-vps-panel"], timeout=30)

    threading.Thread(target=restart_soon, daemon=True).start()


def logged_in():
    return session.get("auth") is True


def require_login():
    if not logged_in():
        return redirect(url_for("login"))
    return None


def csrf_token():
    if "csrf" not in session:
        session["csrf"] = secrets.token_hex(16)
    return session["csrf"]


def require_csrf():
    return request.form.get("csrf") == session.get("csrf")


# --- pages -----------------------------------------------------------------
LOGIN_PAGE = """
<!doctype html><html lang="pl"><head><meta charset="utf-8">
<title>{{ brand }} — Panel serwera</title>
<meta name="viewport" content="width=device-width, initial-scale=1">
<style>
*{box-sizing:border-box}
body{background:#070b13;color:#eaf1fd;margin:0;min-height:100vh;
  font:15px/1.55 system-ui,-apple-system,"Segoe UI",sans-serif;
  display:grid;place-items:center;padding:20px;
  background-image:radial-gradient(900px 420px at 50% -120px,#1b2f55 0%,transparent 70%)}
.card{background:linear-gradient(160deg,#132441 0%,#0f1828 65%);border:1px solid #1e3150;
  border-radius:18px;padding:34px 30px;width:100%;max-width:370px;
  box-shadow:0 22px 56px rgba(0,0,0,.45)}
.mark{width:52px;height:52px;border-radius:14px;display:grid;place-items:center;margin-bottom:18px;
  background:linear-gradient(145deg,#2563eb,#0f3d8a);font-size:1.5rem;
  box-shadow:0 8px 22px rgba(37,99,235,.38)}
h1{font-size:1.22rem;margin:0 0 3px;letter-spacing:.01em}
.sub{color:#7189a8;font-size:.8rem;margin:0 0 22px}
label{display:block;font-size:.72rem;color:#7189a8;text-transform:uppercase;
  letter-spacing:.08em;margin-bottom:6px}
input{width:100%;padding:12px 13px;margin-bottom:16px;background:#050a12;
  border:1px solid #1e3150;border-radius:9px;color:#eaf1fd;font-size:1rem}
input:focus{outline:0;border-color:#2563eb}
button{width:100%;padding:13px;border:0;border-radius:11px;color:#fff;font-size:.97rem;
  font-weight:700;cursor:pointer;letter-spacing:.02em;
  background:linear-gradient(145deg,#2f6ff0,#1746a8);box-shadow:0 10px 24px rgba(37,99,235,.28)}
button:hover{filter:brightness(1.1)}
.error{background:#3f1a22;border:1px solid #6b1f2c;color:#fb7185;border-radius:9px;
  padding:9px 12px;font-size:.84rem;margin:0 0 16px}
</style></head><body>
<form class="card" method="post">
  <div class="mark">⬢</div>
  <h1>{{ brand }}</h1>
  <p class="sub">Panel serwera</p>
  {% if error %}<p class="error">{{ error }}</p>{% endif %}
  <label for="password">Hasło</label>
  <input id="password" type="password" name="password" autofocus required>
  <button type="submit">Zaloguj</button>
</form>
</body></html>
"""

DASHBOARD_PAGE = """
<!doctype html><html lang="pl"><head><meta charset="utf-8">
<title>{{ brand }} — Panel serwera</title>
<meta name="viewport" content="width=device-width, initial-scale=1">
<style>
:root{--bg:#070b13;--panel:#0f1828;--raised:#16233a;--border:#1e3150;--text:#eaf1fd;
  --dim:#a9bed8;--muted:#7189a8;--ok:#3fd3a0;--warn:#f5c451;--bad:#fb7185;--accent:#2563eb;
  --gold:#ffd500}
*{box-sizing:border-box}
body{background:var(--bg);color:var(--text);margin:0;padding:0;
  font:15px/1.55 system-ui,-apple-system,"Segoe UI",sans-serif;
  background-image:radial-gradient(1200px 500px at 50% -180px,#1b2f55 0%,transparent 70%)}
.wrap{max-width:1000px;margin:0 auto;padding:18px 20px 40px}

/* --- pasek gorny --- */
.topbar{display:flex;align-items:center;gap:14px;padding:10px 0 22px}
.mark{width:42px;height:42px;border-radius:12px;flex:none;display:grid;place-items:center;
  background:linear-gradient(145deg,#2563eb,#0f3d8a);font-size:1.25rem;
  box-shadow:0 6px 18px rgba(37,99,235,.35)}
.brand{flex:1;min-width:0}
.brand b{display:block;font-size:1.15rem;letter-spacing:.01em;line-height:1.2}
.brand span{display:block;font-size:.78rem;color:var(--muted);overflow:hidden;
  text-overflow:ellipsis;white-space:nowrap}
.chip{padding:5px 12px;border-radius:999px;background:var(--raised);border:1px solid var(--border);
  font-size:.78rem;font-weight:600;color:var(--dim);white-space:nowrap}
.topbar a{color:var(--muted);text-decoration:none;font-size:.82rem;white-space:nowrap}
.topbar a:hover{color:var(--text)}

/* --- hero --- */
.hero{background:linear-gradient(160deg,#132441 0%,#0f1828 60%);
  border:1px solid var(--border);border-radius:18px;padding:26px;margin-bottom:16px;
  box-shadow:0 18px 44px rgba(0,0,0,.35)}
.hero-top{display:flex;align-items:center;gap:22px;flex-wrap:wrap}
.hero-state{flex:1;min-width:220px}
.hero-state .label{font-size:.75rem;color:var(--muted);text-transform:uppercase;
  letter-spacing:.09em;margin-bottom:6px}
.hero-state .value{font-size:1.75rem;font-weight:700;line-height:1.15;display:flex;
  align-items:center;gap:12px}
.beacon{width:13px;height:13px;border-radius:50%;flex:none;background:currentColor}
.state-up{color:var(--ok)}.state-partial{color:var(--warn)}.state-down{color:var(--bad)}
.beacon.live{animation:pulse 2s ease-in-out infinite}
@keyframes pulse{0%,100%{box-shadow:0 0 0 0 currentColor;opacity:1}
  50%{box-shadow:0 0 0 7px transparent;opacity:.65}}
.hero-note{font-size:.82rem;color:var(--muted);margin-top:7px}

.cta{padding:16px 34px;border-radius:13px;border:0;font-size:1.02rem;font-weight:700;
  cursor:pointer;color:#fff;letter-spacing:.02em;min-width:230px;
  transition:transform .12s ease,filter .12s ease}
.cta:hover:not(:disabled){transform:translateY(-1px);filter:brightness(1.1)}
.cta.go{background:linear-gradient(145deg,#19b87c,#0d7a52);box-shadow:0 10px 26px rgba(25,184,124,.28)}
.cta.halt{background:linear-gradient(145deg,#d9455f,#8f1f33);box-shadow:0 10px 26px rgba(217,69,95,.26)}
.cta:disabled{opacity:.45;cursor:not-allowed;transform:none;filter:none}

.tiles{display:grid;grid-template-columns:repeat(auto-fit,minmax(132px,1fr));gap:11px;margin-top:22px}
.tile{background:rgba(10,19,34,.62);border:1px solid var(--border);border-radius:12px;padding:12px 14px}
.tile span{display:block;font-size:.68rem;color:var(--muted);text-transform:uppercase;
  letter-spacing:.06em;margin-bottom:3px}
.tile b{font-size:1.22rem;font-weight:700}

/* --- karty --- */
.panel{background:var(--panel);border:1px solid var(--border);border-radius:15px;
  padding:20px 22px;margin-bottom:14px}
.panel h2{font-size:.76rem;margin:0 0 15px;color:var(--muted);text-transform:uppercase;
  letter-spacing:.09em;display:flex;align-items:center;gap:9px}
.panel h2 .tag{margin-left:auto;text-transform:none;letter-spacing:0;font-size:.75rem}
.actions{display:flex;flex-wrap:wrap;gap:9px}
button{padding:11px 17px;border-radius:10px;border:1px solid var(--border);background:var(--raised);
  color:var(--text);font-size:.88rem;font-weight:600;cursor:pointer;
  transition:border-color .12s ease,background .12s ease}
button:hover:not(:disabled){border-color:var(--accent);background:#1b2c49}
button.build{background:#16294a;border-color:#2c4a7a;color:#8fc0ff}
button.grab{background:#1d3a2c;border-color:#2b6048;color:var(--ok)}
button:disabled{opacity:.45;cursor:not-allowed}
.busy{color:var(--warn);font-size:.85rem;margin:0 0 12px;display:flex;align-items:center;gap:8px}
.spin{width:12px;height:12px;border:2px solid currentColor;border-right-color:transparent;
  border-radius:50%;display:inline-block;animation:spin .8s linear infinite}
@keyframes spin{to{transform:rotate(360deg)}}

.linkrow{display:flex;flex-wrap:wrap;gap:9px}
.linkrow a{padding:10px 16px;border-radius:10px;border:1px solid var(--border);background:var(--raised);
  color:var(--text);text-decoration:none;font-size:.87rem;font-weight:600}
.linkrow a:hover{border-color:var(--accent);background:#1b2c49}

table{width:100%;border-collapse:collapse;font-size:.85rem}
td,th{padding:8px 6px;text-align:left;border-bottom:1px solid var(--border)}
th{font-size:.7rem;color:var(--muted);text-transform:uppercase;letter-spacing:.06em}
tr:last-child td{border-bottom:0}
.dotstate{display:inline-flex;align-items:center;gap:7px}
.dotstate i{width:7px;height:7px;border-radius:50%;background:currentColor}
.state-running{color:var(--ok)}.state-exited{color:var(--bad)}.state-restarting{color:var(--warn)}

pre{background:#050a12;border:1px solid var(--border);border-radius:10px;padding:13px;
  max-height:300px;overflow:auto;font-size:.77rem;white-space:pre-wrap;word-break:break-word;
  color:#cddcf0;margin:0}

/* --- aktualizacja --- */
.gitline{font-size:.85rem;color:var(--dim);margin:0 0 14px;display:flex;flex-wrap:wrap;
  align-items:center;gap:8px}
.gitline code{background:#050a12;border:1px solid var(--border);border-radius:5px;
  padding:1px 7px;font-size:.8rem;color:#8fc0ff}
.pill{display:inline-block;padding:3px 11px;border-radius:999px;font-size:.77rem;font-weight:700}
.pill.behind{background:#4a3a12;color:var(--warn)}
.pill.current{background:#123d2e;color:var(--ok)}
.pill.err{background:#3f1a22;color:var(--bad)}
#git-result{margin-top:15px}
#git-result h3{font-size:.7rem;color:var(--muted);text-transform:uppercase;
  letter-spacing:.06em;margin:14px 0 7px}
.git-list{background:#050a12;border:1px solid var(--border);border-radius:9px;padding:10px 12px;
  max-height:210px;overflow:auto;font-size:.79rem;font-family:ui-monospace,Consolas,monospace}
.git-list div{padding:2px 0;white-space:pre-wrap;word-break:break-all}
.git-list .A{color:var(--ok)}.git-list .M{color:#8fc0ff}.git-list .D{color:var(--bad)}
.git-list .R{color:var(--warn)}
.warnbox{background:#463714;border:1px solid #6b551d;color:#f7dda1;border-radius:9px;
  padding:11px 13px;font-size:.82rem;margin-top:13px}
.warnbox code{background:rgba(0,0,0,.3);padding:1px 6px;border-radius:4px}

@media (max-width:620px){
  .wrap{padding:14px 14px 30px}
  .hero{padding:20px}
  .cta{width:100%;min-width:0}
  .hero-state .value{font-size:1.45rem}
}
</style></head><body>
<div class="wrap">

  <div class="topbar">
    <div class="mark">⬢</div>
    <div class="brand">
      <b>{{ brand }}</b>
      <span>{{ compose_dir }}</span>
    </div>
    <span class="chip" id="version-chip">—</span>
    <a href="{{ url_for('settings') }}">ustawienia</a>
    <a href="{{ url_for('logout') }}">wyloguj</a>
  </div>

  <div class="hero">
    <div class="hero-top">
      <div class="hero-state">
        <div class="label">Stan serwera</div>
        <div class="value state-down" id="hero-value">
          <i class="beacon" id="hero-beacon"></i><span id="hero-text">Sprawdzam…</span>
        </div>
        <div class="hero-note" id="hero-note"></div>
      </div>
      <button class="cta go" id="cta" disabled>—</button>
    </div>
    <div class="tiles" id="tiles"></div>
  </div>

  <div class="panel">
    <h2>Sterowanie</h2>
    <p id="busy" class="busy" hidden><i class="spin"></i><span id="busy-text"></span></p>
    <div class="actions">
      <button data-action="restart-game">⟳ Restart gry</button>
      <button data-action="restart-stack">⟳ Restart całego stosu</button>
      <button class="build" data-action="compile">🔨 Kompiluj silnik</button>
    </div>
  </div>

  <div class="panel">
    <h2>Aktualizacja z GitHuba</h2>
    <p id="git-current" class="gitline">Ładowanie…</p>
    <div class="actions">
      <button id="git-check" data-git="check">🔄 Sprawdź aktualizacje</button>
      <button class="grab" id="git-apply" data-action="git-update" disabled>⬇ Zastosuj aktualizację</button>
    </div>
    <div id="git-result" hidden></div>
  </div>

  <div class="panel">
    <h2>Panele serwera</h2>
    <div class="linkrow">
      {% for label, url in links %}
        <a href="{{ url }}" target="_blank" rel="noopener">{{ label }} →</a>
      {% endfor %}
    </div>
  </div>

  <div class="panel">
    <h2>Kontenery <span class="tag" id="containers-tag"></span></h2>
    <table id="containers"><tbody><tr><td>Ładowanie…</td></tr></tbody></table>
  </div>

  <div class="panel">
    <h2>Log ostatniej akcji</h2>
    <pre id="action-log">(brak)</pre>
  </div>
</div>

<script>
const csrf = {{ csrf|tojson }};
let gitBehind = 0;
let gameState = 'down';

function escapeHtml(s) {
  return String(s).replace(/[&<>"']/g, c => ({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));
}

async function post(action) {
  const res = await fetch('/api/action/' + action, {method: 'POST', headers: {'X-CSRF': csrf}});
  if (!res.ok) {
    let message = 'Nie udało się (' + res.status + ').';
    try { const body = await res.json(); if (body.error) message = body.error; } catch (e) {}
    alert(message);
  }
  refresh();
}

document.querySelectorAll('[data-action]').forEach(btn => {
  btn.addEventListener('click', () => {
    if (btn.dataset.action === 'git-update' && !confirm(
        'Nadpisać pliki serwera wersją z GitHuba, przebudować obraz gry i wstać na nowo?\n\n' +
        'Baza, .env i kopie zapasowe zostają nietknięte.')) return;
    post(btn.dataset.action);
  });
});

// Jeden duży przycisk zamiast pary Uruchom/Zatrzymaj: w danej chwili sensowna
// jest dokładnie jedna z tych akcji, a launcher ma pokazywać tę jedną.
document.getElementById('cta').addEventListener('click', () => {
  if (gameState === 'down') { post('start'); return; }
  if (confirm('Zatrzymać serwer? Gracze i boty zostaną rozłączeni.\n\nPostęp zostaje — baza nie jest ruszana.')) {
    post('stop');
  }
});

function renderHero(data) {
  const value = document.getElementById('hero-value');
  const beacon = document.getElementById('hero-beacon');
  gameState = data.game_state;
  value.className = 'value state-' + data.game_state;
  beacon.className = 'beacon' + (data.game_state === 'up' ? ' live' : '');
  document.getElementById('hero-text').textContent = data.game_label;
  document.getElementById('hero-note').textContent =
    data.uptime ? 'Gra działa od ' + data.uptime : '';
  document.getElementById('version-chip').textContent =
    data.version ? 'wersja ' + data.version : 'wersja nieznana';

  const cta = document.getElementById('cta');
  cta.disabled = data.action.running;
  if (data.game_state === 'down') {
    cta.className = 'cta go';
    cta.textContent = '▶  URUCHOM SERWER';
  } else {
    cta.className = 'cta halt';
    cta.textContent = '■  ZATRZYMAJ SERWER';
  }

  const items = [
    ['Postacie online', data.live_characters ?? '—'],
    ['Postacie w bazie', data.db ? data.db.characters : '—'],
    ['Konta', data.db ? data.db.accounts : '—'],
    ['Obciążenie', data.host.load1 ?? '—'],
    ['RAM', data.host.ram_used_mb != null ? data.host.ram_used_mb + ' / ' + data.host.ram_total_mb + ' MB' : '—'],
    ['Dysk', data.host.disk_used_gb != null ? data.host.disk_used_gb + ' / ' + data.host.disk_total_gb + ' GB' : '—'],
  ];
  document.getElementById('tiles').innerHTML = items.map(([label, value]) =>
    `<div class="tile"><span>${escapeHtml(label)}</span><b>${escapeHtml(value)}</b></div>`).join('');
}

function renderGitLine(git) {
  const el = document.getElementById('git-current');
  if (!git || !git.ok) {
    el.innerHTML = '<span class="pill err">nieskonfigurowane</span> ' +
      escapeHtml((git && git.error) || 'git nie jest skonfigurowany');
    gitBehind = 0;
    return;
  }
  // behind === null znaczy "jeszcze nie wiadomo" (nikt nie pobierał, albo
  // gałąź zdalna nie istnieje) - a to nie to samo co "aktualne". Zielone
  // "aktualne" byłoby tu jedynym miejscem, w którym ten panel kłamie.
  const pill = (git.behind === null || git.behind === undefined)
    ? '<span class="pill err">nieznane — kliknij „Sprawdź”</span>'
    : (git.behind > 0
        ? `<span class="pill behind">${git.behind} commitów w tyle</span>`
        : '<span class="pill current">aktualne</span>');
  const dirty = git.dirty > 0
    ? `<span style="color:var(--warn)">⚠ ${git.dirty} plików zmienionych na serwerze (zostaną nadpisane)</span>`
    : '';
  el.innerHTML = pill +
    `<code>${escapeHtml(git.branch)}</code><code>${escapeHtml(git.commit)}</code>` +
    `<span>${escapeHtml(git.subject || '')}</span>` +
    (git.date ? `<span style="color:var(--muted)">${escapeHtml(git.date)}</span>` : '') + dirty;
  gitBehind = git.behind || 0;
}

function applyGitGate(running) {
  const apply = document.getElementById('git-apply');
  if (apply) apply.disabled = running || gitBehind <= 0;
  const check = document.getElementById('git-check');
  if (check) check.disabled = running;
}

document.getElementById('git-check').addEventListener('click', async () => {
  const button = document.getElementById('git-check');
  const box = document.getElementById('git-result');
  button.disabled = true;
  button.textContent = '⏳ Sprawdzam…';
  try {
    const res = await fetch('/api/git/check', {method: 'POST', headers: {'X-CSRF': csrf}});
    const data = await res.json();
    if (!data.ok) {
      box.hidden = false;
      box.innerHTML = `<div class="warnbox">${escapeHtml(data.error || 'Nie udało się sprawdzić.')}</div>`;
      return;
    }
    renderGitLine(data.git);
    box.hidden = false;
    if (!data.incoming.commits.length) {
      box.innerHTML = '<div class="git-list"><div>Nic nowego — serwer ma najnowszą wersję.</div></div>';
      return;
    }
    const commits = data.incoming.commits.map(c => `<div>${escapeHtml(c)}</div>`).join('');
    const files = data.incoming.files.map(f =>
      `<div class="${escapeHtml(f.status)}">${escapeHtml(f.status)}  ${escapeHtml(f.path)}</div>`).join('');
    const more = data.incoming.files_total > data.incoming.files.length
      ? `<div>… i ${data.incoming.files_total - data.incoming.files.length} więcej</div>` : '';
    box.innerHTML =
      `<h3>Nowe commity (${data.incoming.commits.length})</h3><div class="git-list">${commits}</div>` +
      `<h3>Zmienione pliki (${data.incoming.files_total})</h3><div class="git-list">${files}${more}</div>` +
      (data.panel_changed
        ? '<div class="warnbox">Ta aktualizacja zmienia kod panelu VPS. Panel nie podmieni ' +
          'się sam — po zastosowaniu uruchom na serwerze <code>sudo sh ' +
          'linux-port/vps-panel/install.sh</code>.</div>'
        : '');
  } catch (e) {
    box.hidden = false;
    box.innerHTML = '<div class="warnbox">Panel nie odpowiedział. Spróbuj ponownie.</div>';
  } finally {
    button.textContent = '🔄 Sprawdź aktualizacje';
    applyGitGate(false);
  }
});

async function refresh() {
  try {
    const res = await fetch('/api/status', {cache: 'no-store'});
    const data = await res.json();
    renderHero(data);
    renderGitLine(data.git);

    const busy = document.getElementById('busy');
    document.querySelectorAll('[data-action]').forEach(b => b.disabled = data.action.running);
    applyGitGate(data.action.running);
    if (data.action.running) {
      busy.hidden = false;
      document.getElementById('busy-text').textContent = data.action.label + '…';
    } else {
      busy.hidden = true;
    }

    const body = document.querySelector('#containers tbody');
    const tag = document.getElementById('containers-tag');
    if (!data.containers || !data.containers.length) {
      tag.textContent = '';
      body.innerHTML = '<tr><td>Brak danych (Docker nie odpowiada?)</td></tr>';
    } else {
      const up = data.containers.filter(c => c.state === 'running').length;
      tag.textContent = up + ' / ' + data.containers.length + ' działa';
      body.innerHTML = '<tr><th>Usługa</th><th>Stan</th></tr>' + data.containers.map(c =>
        `<tr><td>${escapeHtml(c.name)}</td>` +
        `<td><span class="dotstate state-${escapeHtml(c.state)}"><i></i>${escapeHtml(c.state)}` +
        `${c.health ? ' (' + escapeHtml(c.health) + ')' : ''}</span></td></tr>`
      ).join('');
    }
  } catch (e) { /* spróbujemy przy następnym tyknięciu */ }
}

async function refreshLog() {
  try {
    const res = await fetch('/api/action-log', {cache: 'no-store'});
    const data = await res.json();
    const pre = document.getElementById('action-log');
    pre.textContent = data.log || '(brak)';
    pre.scrollTop = pre.scrollHeight;
  } catch (e) { /* ignorujemy */ }
}

refresh(); refreshLog();
setInterval(refresh, 4000);
setInterval(refreshLog, 2000);
</script>
</body></html>
"""

SETTINGS_PAGE = """
<!doctype html><html lang="pl"><head><meta charset="utf-8">
<title>Ustawienia — M2 VPS Panel</title>
<meta name="viewport" content="width=device-width, initial-scale=1">
<style>
*{box-sizing:border-box}
body{background:#070b13;color:#eaf1fd;margin:0;padding:20px;
  font:15px/1.55 system-ui,-apple-system,"Segoe UI",sans-serif;
  background-image:radial-gradient(900px 420px at 50% -120px,#1b2f55 0%,transparent 70%)}
.wrap{max-width:430px;margin:40px auto}
.panel{background:linear-gradient(160deg,#132441 0%,#0f1828 65%);border:1px solid #1e3150;
  border-radius:18px;padding:30px;box-shadow:0 20px 50px rgba(0,0,0,.4)}
h1{font-size:1.12rem;margin:0 0 20px}
label{display:block;font-size:.72rem;color:#7189a8;text-transform:uppercase;
  letter-spacing:.08em;margin-bottom:6px}
input{width:100%;padding:12px 13px;margin-bottom:16px;background:#050a12;
  border:1px solid #1e3150;border-radius:9px;color:#eaf1fd;font-size:1rem}
input:focus{outline:0;border-color:#2563eb}
button{padding:12px 22px;border:0;border-radius:11px;color:#fff;font-size:.95rem;font-weight:700;
  cursor:pointer;background:linear-gradient(145deg,#2f6ff0,#1746a8);
  box-shadow:0 10px 24px rgba(37,99,235,.28)}
button:hover{filter:brightness(1.1)}
.error{background:#3f1a22;border:1px solid #6b1f2c;color:#fb7185;border-radius:9px;
  padding:9px 12px;font-size:.84rem;margin:0 0 16px}
.back{color:#7189a8;font-size:.84rem;text-decoration:none;display:inline-block;margin-bottom:14px}
.back:hover{color:#eaf1fd}
</style></head><body>
<div class="wrap">
  <a class="back" href="{{ url_for('index') }}">← wróć</a>
  <div class="panel">
    <h1>Zmiana hasła panelu</h1>
    {% if error %}<p class="error">{{ error }}</p>{% endif %}
    <form method="post">
      <input type="hidden" name="csrf" value="{{ csrf }}">
      <label>Obecne hasło</label>
      <input type="password" name="current_password" required autofocus>
      <label>Nowe hasło (min. 8 znaków)</label>
      <input type="password" name="new_password" required minlength="8">
      <label>Powtórz nowe hasło</label>
      <input type="password" name="confirm_password" required minlength="8">
      <button type="submit">Zmień hasło</button>
    </form>
  </div>
</div>
</body></html>
"""

SETTINGS_DONE_PAGE = """
<!doctype html><html lang="pl"><head><meta charset="utf-8">
<title>Ustawienia — M2 VPS Panel</title>
<meta name="viewport" content="width=device-width, initial-scale=1">
<style>
body{background:#070b13;color:#eaf1fd;margin:0;min-height:100vh;
  font:15px/1.55 system-ui,-apple-system,"Segoe UI",sans-serif;
  display:grid;place-items:center;padding:20px;
  background-image:radial-gradient(900px 420px at 50% -120px,#1b2f55 0%,transparent 70%)}
.card{background:linear-gradient(160deg,#132441 0%,#0f1828 65%);border:1px solid #1e3150;
  border-radius:18px;padding:34px;max-width:390px;text-align:center;
  box-shadow:0 20px 50px rgba(0,0,0,.4)}
a{color:#8fc0ff}
</style></head><body>
<div class="card">
  <p>✅ {{ message }}</p>
  <p><a href="{{ url_for('login') }}">Przejdź do logowania</a></p>
</div>
</body></html>
"""


@app.route("/login", methods=["GET", "POST"])
def login():
    if request.method == "POST":
        if check_password(request.form.get("password", "")):
            session["auth"] = True
            return redirect(url_for("index"))
        return render_template_string(LOGIN_PAGE, error="Błędne hasło.", brand=server_brand())
    if logged_in():
        return redirect(url_for("index"))
    return render_template_string(LOGIN_PAGE, error=None, brand=server_brand())


@app.route("/logout")
def logout():
    session.clear()
    return redirect(url_for("login"))


@app.route("/")
def index():
    guard = require_login()
    if guard:
        return guard
    return render_template_string(DASHBOARD_PAGE, compose_dir=str(COMPOSE_DIR), csrf=csrf_token(),
                                   links=related_panels(), brand=server_brand())


@app.route("/settings", methods=["GET", "POST"])
def settings():
    guard = require_login()
    if guard:
        return guard
    error = None
    message = None
    if request.method == "POST":
        if request.form.get("csrf") != session.get("csrf"):
            error = "Sesja wygasła, odśwież stronę."
        elif not check_password(request.form.get("current_password", "")):
            error = "Obecne hasło jest nieprawidłowe."
        elif len(request.form.get("new_password", "")) < 8:
            error = "Nowe hasło musi mieć co najmniej 8 znaków."
        elif request.form.get("new_password") != request.form.get("confirm_password"):
            error = "Nowe hasła nie są takie same."
        else:
            change_password(request.form["new_password"])
            session.clear()
            return render_template_string(
                SETTINGS_DONE_PAGE,
                message="Hasło zmienione. Panel restartuje się (kilka sekund) - zaloguj się nowym hasłem.")
    return render_template_string(SETTINGS_PAGE, error=error, message=message, csrf=csrf_token())


@app.route("/api/status")
def api_status():
    guard = require_login()
    if guard:
        return guard
    state, label = game_status()
    return {
        "ok": True,
        "game_state": state,
        "game_label": label,
        "live_characters": live_character_count(),
        "db": db_totals(),
        "host": host_metrics(),
        "containers": containers_status(),
        "git": git_status(),
        "version": server_version(),
        "uptime": game_uptime(),
        "action": {"running": _action_state["running"], "label": _action_state["label"]},
    }


@app.route("/api/git/check", methods=["POST"])
def api_git_check():
    """Fetch, then report what an update would bring. Synchronous on purpose:
    it is seconds of network and nothing else, and an operator who clicked
    "check" wants the answer on the same click, not a log to tail."""
    guard = require_login()
    if guard:
        return guard
    if request.headers.get("X-CSRF") != session.get("csrf"):
        return {"ok": False, "error": "csrf"}, 403
    # A fetch running against the same index as an in-flight reset is how you
    # get an index.lock collision; the lock the other actions use covers it.
    if _action_state["running"]:
        return {"ok": False, "error": f"Trwa: {_action_state['label']}. Poczekaj."}, 409
    fetched, message = git_fetch()
    if not fetched:
        return {"ok": False, "error": message}, 502
    status = git_status()
    incoming = git_incoming()
    return {"ok": True, "git": status, "incoming": incoming,
            "panel_changed": git_touches_panel(incoming["files"])}


@app.route("/api/action-log")
def api_action_log():
    guard = require_login()
    if guard:
        return guard
    try:
        text = ACTION_LOG.read_text(encoding="utf-8", errors="replace")
    except OSError:
        text = ""
    return {"ok": True, "log": text[-16000:]}


@app.route("/api/action/<name>", methods=["POST"])
def api_action(name):
    guard = require_login()
    if guard:
        return guard
    if request.headers.get("X-CSRF") != session.get("csrf"):
        return {"ok": False, "error": "csrf"}, 403
    handlers = {
        "start": action_start,
        "stop": action_stop,
        "restart-game": action_restart_game,
        "restart-stack": action_restart_stack,
        "compile": action_compile,
        "git-update": action_git_update,
    }
    handler = handlers.get(name)
    if not handler:
        return {"ok": False, "error": "unknown action"}, 404
    # "busy" is the only reason the other actions refuse, but git-update also
    # refuses when it has no configured checkout - reporting that as "busy"
    # would send an operator looking for a running build that isn't there.
    if name == "git-update" and not git_available():
        return {"ok": False, "error": git_status().get("error", "git nie jest skonfigurowany")}, 409
    started = handler()
    if not started:
        return {"ok": False, "error": "Trwa inna akcja. Poczekaj, aż się skończy."}, 409
    return {"ok": True}


if __name__ == "__main__":
    app.run(host="0.0.0.0", port=PANEL_PORT)
