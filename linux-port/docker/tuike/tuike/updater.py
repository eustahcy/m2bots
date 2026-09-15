"""The isolated updater, reached through a second spool and nothing else.

Updating means running commands on the host, and this panel is the part of the
stack facing the network. So Tuike never mounts Docker's socket: it drops a
request of fixed, minimal content - an id, the version it believes is
installed, a timestamp - into a directory the updater container watches, and
reads back the status and log the updater writes there. No command, URL or path
crosses that boundary, and nothing the browser sends reaches the host.
"""
import json
import os
import re
import time
import uuid
from pathlib import Path
from urllib.error import HTTPError, URLError
from urllib.request import Request, urlopen

from . import config
from .spool import read_values, file_age

SPOOL = config.UPDATE_SPOOL
# The updater rewrites its watcher file on a timer; older than this and the
# container is not running.
WATCHER_MAX_AGE = 90
LOG_TAIL_LINES = 18
DEFAULT_STEPS = 5

_release_cache = {"checked_at": 0.0, "latest": None, "error": None}


def status():
    """State, progress and the tail of the log, as the updater reports them."""
    result = read_values(SPOOL / "update.status")
    age = file_age(SPOOL / "watcher")
    result["watcher_age"] = age
    result["watcher_ready"] = age is not None and age < WATCHER_MAX_AGE
    result["state"] = result.get("state", "idle")
    try:
        step = int(result.get("step", 0))
        steps = int(result.get("steps", DEFAULT_STEPS)) or DEFAULT_STEPS
        result["percent"] = max(0, min(100, round(step * 100 / steps)))
    except ValueError:
        result["percent"] = 0
    if result["state"] == "running":
        # A running update that has not reported a step yet still deserves a
        # bar that has moved; a 0% bar reads as "nothing is happening".
        result["percent"] = max(5, result["percent"])
    result.setdefault(
        "message",
        "Aktualizator czeka na zlecenie." if result["watcher_ready"]
        else "Aktualizator nie jest uruchomiony.",
    )
    try:
        result["log"] = (SPOOL / "update.log").read_text(
            encoding="utf-8", errors="replace").splitlines()[-LOG_TAIL_LINES:]
    except OSError:
        result["log"] = []
    return result


def queue():
    """Ask for the updater's one fixed sequence. Raises RuntimeError if it cannot."""
    current = status()
    if not current["watcher_ready"]:
        raise RuntimeError(
            "Aktualizator nie jest uruchomiony. Administrator musi wystartować usługę "
            "updater: docker compose --profile update up -d updater")
    if current.get("state") == "running":
        raise RuntimeError("Aktualizacja już trwa. Poczekaj na jej zakończenie.")
    version = installed_version().strip()
    if not re.fullmatch(r"[0-9]+(?:\.[0-9]+)*", version):
        version = "0"
    identifier = config.REQUEST_PREFIX + uuid.uuid4().hex
    SPOOL.mkdir(parents=True, exist_ok=True)
    temporary = SPOOL / f"{identifier}.new"
    try:
        temporary.write_text(
            f"id={identifier}\nversion={version}\ntime={int(time.time())}\n", encoding="utf-8")
        try:
            temporary.chmod(0o660)
        except OSError:
            pass
        # replace is atomic, and the worker records the id before doing any
        # work, so a finished request never runs twice after a recreation.
        os.replace(temporary, SPOOL / "request")
    finally:
        Path(temporary).unlink(missing_ok=True)
    return identifier


def installed_version():
    """Prefer the version the updater confirmed, then the configured baseline."""
    current = status()
    if current.get("state") == "ok":
        match = re.search(r"version ([0-9]+(?:\.[0-9]+)+)", current.get("message", ""))
        if match:
            return match.group(1)
    return config.PLAYERBOTS_VERSION


def _version_key(value):
    match = re.fullmatch(r"v?([0-9]+(?:\.[0-9]+)+)", str(value or "").strip(), re.I)
    return tuple(int(part) for part in match.group(1).split(".")) if match else None


def latest_release():
    """GitHub's newest published release, cached so page loads never hammer it."""
    if not config.UPDATE_CHECK:
        return {"checked_at": 0.0, "latest": None, "error": "Sprawdzanie wydań jest wyłączone."}
    now = time.time()
    if now - _release_cache["checked_at"] < config.RELEASE_CACHE_SECONDS:
        return dict(_release_cache)
    result = {"checked_at": now, "latest": None, "error": None}
    try:
        request = Request(config.RELEASE_URL, headers={
            "Accept": "application/vnd.github+json",
            "User-Agent": f"{config.PANEL_CODENAME}-Panel",
        })
        with urlopen(request, timeout=3) as response:
            payload = json.load(response)
        tag = str(payload.get("tag_name") or "").strip()
        if not _version_key(tag):
            raise ValueError("GitHub nie zwrócił poprawnego numeru wydania.")
        result["latest"] = tag.lstrip("vV")
    except (OSError, ValueError, HTTPError, URLError, json.JSONDecodeError) as exc:
        result["error"] = str(exc)[:120] or "Nie udało się połączyć z GitHub."
    _release_cache.update(result)
    return dict(result)


def release_status():
    """Installed against newest, in the shape the dashboard badge wants."""
    installed = installed_version().strip()
    info = latest_release()
    latest = info.get("latest")
    installed_key, latest_key = _version_key(installed), _version_key(latest)
    if installed_key and latest_key:
        behind = installed_key < latest_key
        return {
            "installed": installed, "latest": latest, "behind": behind,
            "tone": "outdated" if behind else "current",
            "label": f"Dostępna {latest}" if behind else "Aktualna",
        }
    return {
        "installed": installed, "latest": latest, "behind": False, "tone": "unknown",
        "label": "Nie sprawdzono GitHub" if info.get("error") else "Brak wersji lokalnej",
    }
