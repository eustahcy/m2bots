"""Read-only reference pages: the GM command list and the panel's changelog."""
from flask import Blueprint, render_template

from .. import config
from ..security import login_required

bp = Blueprint("reference", __name__)

try:
    GM_COMMANDS = (config.PROJECT_DIR / "gm_commands.txt").read_text(encoding="utf-8", errors="replace")
except OSError:
    GM_COMMANDS = "Brak pliku z komendami."


def changelog_entries():
    """Read the repository's own changelog into dated entries.

    The format is the one CHANGELOG.md already uses: '## <when> · <version>'
    headings with '- ' bullets under them.
    """
    try:
        lines = (config.PROJECT_DIR / "CHANGELOG.md").read_text(encoding="utf-8").splitlines()
    except OSError:
        return []
    entries, current = [], None
    for line in lines:
        if line.startswith("## "):
            if current:
                entries.append(current)
            heading = line[3:].strip()
            timestamp, separator, version = heading.partition(" · ")
            current = {
                "timestamp": timestamp if separator else "Wcześniejsza wersja",
                "version": version if separator else heading,
                "changes": [],
            }
        elif current and line.startswith("- "):
            current["changes"].append(line[2:].strip())
    if current:
        entries.append(current)
    return entries


@bp.route("/gm-commands")
@login_required
def gm_commands():
    return render_template("reference/gm_commands.html", commands=GM_COMMANDS)


@bp.route("/changelog")
@login_required
def changelog():
    return render_template("reference/changelog.html", entries=changelog_entries())
