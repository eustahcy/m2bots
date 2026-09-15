"""First run, signing in, signing out."""
from flask import Blueprint, flash, redirect, render_template, request, url_for
from werkzeug.security import generate_password_hash

from .. import settings
from ..security import sign_in, sign_out

bp = Blueprint("auth", __name__)


@bp.route("/login", methods=["GET", "POST"])
def login():
    current = settings.read()
    if current.get("auth_enabled") != "1":
        # Nothing to sign in to. Sending a visitor to a passphrase box on an
        # open panel would only look like a fault.
        return redirect(url_for("dashboard.index"))
    if request.method == "POST":
        if settings.password_matches(current, request.form.get("password", "")):
            sign_in()
            return redirect(request.args.get("next") or url_for("dashboard.index"))
        flash("Nieprawidłowe hasło.", "error")
    return render_template("auth/login.html")


@bp.post("/logout")
def logout():
    sign_out()
    return redirect(url_for("auth.login"))


@bp.route("/setup", methods=["GET", "POST"])
def setup():
    current = settings.read()
    if current.get("setup_complete") == "1":
        return redirect(url_for("dashboard.index"))
    if request.method == "POST":
        values, error = settings.validate_display(request.form)
        password = request.form.get("panel_password", "")
        protect = request.form.get("auth_enabled") == "1"
        if not error and protect and len(password) < settings.PASSWORD_MIN_LENGTH:
            error = f"Hasło panelu musi mieć co najmniej {settings.PASSWORD_MIN_LENGTH} znaków."
        if error:
            flash(error, "error")
        else:
            values.update({
                "setup_complete": "1",
                "auth_enabled": "1" if protect else "0",
                "auth_password_hash": generate_password_hash(password) if protect else "",
            })
            settings.write(values)
            if protect:
                sign_in()
            flash("Konfiguracja zapisana. Miłej gry.")
            return redirect(url_for("dashboard.index"))
    return render_template("auth/setup.html", current=current,
                           themes=settings.THEMES, densities=settings.DENSITIES,
                           monitor_modes=settings.MONITOR_MODES)
