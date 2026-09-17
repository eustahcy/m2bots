"""Creating accounts and game masters."""
from flask import Blueprint, abort, flash, redirect, render_template, request, url_for

from .. import engine
from ..gamedata.actions import GM_RANKS
from ..gamedata.characters import EMPIRES, GM_GENDER_OPTIONS, GM_JOB_OPTIONS, GM_NAME_PATTERN
from ..queries import accounts as queries
from ..security import login_required, require_csrf

bp = Blueprint("accounts", __name__)

# What the "rodzaj konta" chips offer, in the order they read best: the plain
# case first, then the same ranks the character page's GM tool grants - one
# vocabulary for "what can this account do" everywhere in the panel.
ACCOUNT_KINDS = (("PLAYER", "Zwykły gracz", "bez uprawnień"), *GM_RANKS)


@bp.route("/accounts", methods=["GET", "POST"])
@login_required
def index():
    if request.method == "POST":
        require_csrf()
        try:
            flash(queries.create(queries.validate(request.form)))
            return redirect(url_for("accounts.index"))
        except queries.AccountError as error:
            flash(str(error), "error")
    query = request.args.get("q", "").strip()[:60]
    display = request.args.get("display", "100")
    accounts = queries.roster(query, display)
    return render_template(
        "accounts.html",
        accounts=accounts,
        bot_accounts=sum(1 for row in accounts if str(row.get("login") or "").startswith("playerbot_")),
        query=query,
        display=display if display in queries.DISPLAY_SIZES else "100",
        display_sizes=queries.DISPLAY_SIZES,
        account_kinds=ACCOUNT_KINDS,
        empires=EMPIRES,
        jobs=GM_JOB_OPTIONS,
        genders=GM_GENDER_OPTIONS,
        name_pattern=GM_NAME_PATTERN,
        login_max=engine.LOGIN_MAX_LENGTH,
        audit=queries.audit(),
    )


@bp.route("/account/<int:account_id>")
@login_required
def detail(account_id):
    """One account: its characters, where it logs in from, and what can be done."""
    account = queries.detail(account_id)
    if not account:
        abort(404)
    return render_template(
        "accounts/detail.html",
        account=account,
        characters=queries.characters(account_id),
        logins=queries.logins(account_id),
        related=queries.related(account_id),
        password_min=queries.PASSWORD_MIN_LENGTH,
    )


@bp.post("/account/<int:account_id>/block")
@login_required
def block(account_id):
    """Close an account to new logins, or open it again."""
    require_csrf()
    try:
        flash(queries.set_blocked(account_id, request.form.get("blocked") == "1"))
    except queries.AccountError as error:
        flash(str(error), "error")
    except Exception:
        flash("Nie udało się zmienić stanu konta. Sprawdź, czy baza odpowiada.", "error")
    return redirect(url_for("accounts.detail", account_id=account_id))


@bp.post("/account/<int:account_id>/password")
@login_required
def password(account_id):
    """Set a new password for an account whose owner has lost theirs."""
    require_csrf()
    try:
        flash(queries.set_password(account_id, request.form.get("password", "")))
    except queries.AccountError as error:
        flash(str(error), "error")
    except Exception:
        flash("Nie udało się zmienić hasła. Sprawdź, czy baza odpowiada.", "error")
    return redirect(url_for("accounts.detail", account_id=account_id))
