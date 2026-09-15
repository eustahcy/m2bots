"""Who may see the panel, and proof that a POST came from one of its pages."""
import hmac
import secrets
from functools import wraps

from flask import abort, redirect, request, session, url_for

from . import config, settings

CSRF_SESSION_KEY = "tuike_csrf"


def is_admin():
    return bool(session.get(config.SESSION_ADMIN_KEY))


def sign_in():
    session.clear()
    session[config.SESSION_ADMIN_KEY] = True
    session.permanent = True


def sign_out():
    session.clear()


def login_required(view):
    """Send an unconfigured panel to its wizard and a locked one to its login."""
    @wraps(view)
    def wrapped(*args, **kwargs):
        current = settings.read()
        if current.get("setup_complete") != "1":
            return redirect(url_for("auth.setup"))
        if current.get("auth_enabled") != "1":
            return view(*args, **kwargs)
        if not is_admin():
            return redirect(url_for("auth.login", next=request.full_path))
        return view(*args, **kwargs)
    return wrapped


def csrf_token():
    """One token per session, handed to every form that changes something."""
    token = session.get(CSRF_SESSION_KEY)
    if not token:
        token = secrets.token_hex(32)
        session[CSRF_SESSION_KEY] = token
    return token


def require_csrf(field="csrf"):
    """Abort unless the submitted token matches this session's."""
    expected = session.get(CSRF_SESSION_KEY, "")
    supplied = request.form.get(field, "")
    if not expected or not hmac.compare_digest(supplied, expected):
        abort(403, "Sesja formularza wygasła. Odśwież stronę i spróbuj ponownie.")


def protected_admin():
    """True only when the panel is passphrase-protected AND this session is in.

    The updater button asks for this rather than for login_required: on an
    unprotected panel anybody who can reach the page could rebuild the server.
    """
    current = settings.read()
    return current.get("auth_enabled") == "1" and is_admin()
