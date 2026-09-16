"""Quick local smoke test: does the app import and serve its pages at all?
No real docker/DB on this machine, so status calls are expected to degrade
gracefully (None/empty), not crash - that is exactly what this checks."""
import hashlib
import os
import secrets

salt = secrets.token_hex(16)
password = "smoke-test-password"
digest = hashlib.sha256((salt + password).encode()).hexdigest()
os.environ["M2_VPS_PANEL_PASSWORD_HASH"] = f"{salt}:{digest}"
os.environ["M2_VPS_PANEL_SECRET"] = secrets.token_hex(32)
os.environ["M2_VPS_PANEL_STATE_DIR"] = os.path.join(os.path.dirname(__file__), "_smoke_state")
os.environ["M2_COMPOSE_DIR"] = "/nonexistent/path"

import app as panel  # noqa: E402

client = panel.app.test_client()

r = client.get("/")
assert r.status_code == 302, f"/ should redirect when logged out, got {r.status_code}"

r = client.get("/login")
assert r.status_code == 200 and b"Has\xc5\x82o" in r.data, "login page should render"

r = client.post("/login", data={"password": "wrong"})
assert b"B\xc5\x82\xc4\x99dne has\xc5\x82o" in r.data, "wrong password should be rejected"

# Brute force: the panel is root over docker on a public port, so guessing has
# to hit a wall. Eight tries inside the window, then the address is locked out.
for _ in range(panel.LOGIN_MAX_ATTEMPTS):
    r = client.post("/login", data={"password": "wrong"})
assert r.status_code == 429, f"repeated failures should lock out, got {r.status_code}"
# login_locked_for() needs a request context, so check the state it reads instead.
assert any(entry[2] > 0 for entry in panel._login_attempts.values()), \
    "the failing address should carry a lockout deadline"
# Even the right password waits out the lockout - otherwise it is not a lockout.
r = client.post("/login", data={"password": password})
assert r.status_code == 429, "a lockout must hold regardless of the password offered"
# ...and a clean slate lets the operator back in.
panel._login_attempts.clear()

r = client.post("/login", data={"password": password}, follow_redirects=True)
assert b"Panel serwera" in r.data and r.status_code == 200, "correct password should log in"

r = client.get("/api/status")
data = r.get_json()
assert data["ok"] is True, "status endpoint should respond OK even with no docker/db"
# Not asserting a specific state here: this dev machine may itself have a
# local game stack bound to the same ports (11000/13000) from unrelated work,
# which would legitimately answer "up". Only the shape matters for a smoke test.
assert data["game_state"] in ("up", "partial", "down"), f"unexpected state {data['game_state']!r}"
assert data["live_characters"] is None, "no compose dir -> no volume -> None, not a crash"
assert data["db"] is None, "no db reachable -> None, not a crash"
assert isinstance(data["host"], dict), "host metrics should always be a dict"

# The git block degrades like everything else: no checkout under the (fake)
# compose dir means "not a repository", reported rather than raised.
assert isinstance(data["git"], dict), "git status should always be a dict"
assert data["git"]["ok"] is False, "no checkout -> ok False, not a crash"
assert "git-setup" in data["git"]["error"], "the error should say how to fix it"

r = client.get("/api/action-log")
assert r.get_json()["ok"] is True

r = client.post("/api/action/start")
assert r.status_code == 403, "action without CSRF header must be refused"

r = client.post("/api/git/check")
assert r.status_code == 403, "git check without CSRF header must be refused"

r = client.get("/settings")
assert r.status_code == 200 and b"Zmiana has\xc5\x82a" in r.data, "settings page should render"

r = client.get("/")
assert b"Panele serwera" in r.data, "dashboard should show the related-panels section"
assert b"Tuike" in r.data, "Tuike should be one of the linked panels"
assert b"Aktualizacja z GitHuba" in r.data, "dashboard should show the git update section"
assert b'data-action="git-update"' in r.data, "the apply-update button should be on the page"
assert b'id="git-apply" data-action="git-update" disabled' in r.data, \
    "apply must render disabled: it is gated on a check saying we are behind"

# The launcher shell: one big state readout and one primary button that the
# script swaps between start and stop. Start/stop are no longer separate
# buttons, so a stray [data-action="start"] would mean the old dashboard.
assert b'id="cta"' in r.data, "the launcher needs its primary action button"
assert b'id="hero-text"' in r.data, "the launcher needs the big status readout"
assert b'data-action="start"' not in r.data, "start is the CTA now, not its own button"
assert b'id="version-chip"' in r.data, "the header should carry a version chip"

r = client.get("/logout", follow_redirects=True)
assert b"Has\xc5\x82o" in r.data, "logout should return to the login page"

print("OK: all smoke checks passed")
