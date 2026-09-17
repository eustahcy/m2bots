"""The HTTP surface, one blueprint per part of the panel.

NAVIGATION is the single source of truth for the sidebar: adding a page means
adding a blueprint and one line here, and nothing in the templates changes.
"""
from . import (accounts, api, auth, characters, dashboard, economy, grants,
               guilds, manage, rankings, reference, system, world)

# (group, ((endpoint, icon, label), ...)). The icon is a name from
# templates/partials/icons.html.
NAVIGATION = (
    ("Świat", (
        ("dashboard.index", "dashboard", "Pulpit"),
        ("characters.roster", "users", "Postacie i boty"),
        ("guilds.roster", "flag", "Gildie"),
        ("rankings.index", "trophy", "Rankingi"),
        ("world.maps", "map", "Aktywność map"),
        ("world.season", "sparkles", "Sezon"),
    )),
    ("Dane", (
        ("economy.index", "coins", "Gospodarka"),
        ("economy.shops", "store", "Stragany"),
        ("economy.catalogue", "box", "Baza przedmiotów"),
        ("system.index", "gauge", "Wydajność"),
    )),
    ("Administracja", (
        ("manage.index", "settings", "Zarządzanie"),
        ("grants.index", "gift", "Nadawanie przedmiotów"),
        ("accounts.index", "key", "Konta i GM"),
        ("reference.gm_commands", "terminal", "Komendy GM"),
        ("reference.changelog", "list", "Changelog"),
    )),
)

BLUEPRINTS = (
    auth.bp, dashboard.bp, characters.bp, guilds.bp, rankings.bp, economy.bp,
    world.bp, system.bp, manage.bp, grants.bp, accounts.bp, reference.bp, api.bp,
)


# The icon each page wears in its banner. Pages that are not menu entries of
# their own (one character, one guild, one stall) borrow the icon of the list
# they belong to, so a detail page still looks like where it came from.
NAV_ICONS = {endpoint: name for _group, links in NAVIGATION for endpoint, name, _label in links}
NAV_ICONS.update({
    "characters.profile": "users",
    "guilds.detail": "flag",
    "economy.shops": "store",
    "economy.shop": "store",
    "economy.item": "coins",
    "auth.login": "key",
})


def page_icon():
    """The icon for the page being rendered, or a neutral one."""
    from flask import request

    return NAV_ICONS.get(request.endpoint or "", "sparkles")


def register_blueprints(app):
    for blueprint in BLUEPRINTS:
        app.register_blueprint(blueprint)
    app.jinja_env.globals["navigation"] = NAVIGATION
