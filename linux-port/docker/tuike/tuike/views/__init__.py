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


def register_blueprints(app):
    for blueprint in BLUEPRINTS:
        app.register_blueprint(blueprint)
    app.jinja_env.globals["navigation"] = NAVIGATION
