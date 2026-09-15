"""The HTTP surface, one blueprint per part of the panel.

NAVIGATION is the single source of truth for the sidebar: adding a page means
adding a blueprint and one line here, and nothing in the templates changes.
"""
from . import (accounts, api, auth, characters, dashboard, economy, grants,
               guilds, manage, rankings, reference, system, world)

# (endpoint, icon, label). A bare string is a separator between groups.
NAVIGATION = (
    ("Świat", (
        ("dashboard.index", "◎", "Pulpit"),
        ("characters.roster", "☗", "Postacie i boty"),
        ("guilds.roster", "⚑", "Gildie"),
        ("rankings.index", "♛", "Rankingi"),
        ("world.maps", "⌖", "Aktywność map"),
        ("world.season", "✦", "Sezon"),
    )),
    ("Dane", (
        ("economy.index", "◈", "Gospodarka"),
        ("economy.shops", "🛒", "Stragany"),
        ("economy.catalogue", "▦", "Baza przedmiotów"),
        ("system.index", "▤", "Wydajność"),
    )),
    ("Administracja", (
        ("manage.index", "⚙", "Zarządzanie"),
        ("grants.index", "🎁", "Nadawanie przedmiotów"),
        ("accounts.index", "☘", "Konta i GM"),
        ("reference.gm_commands", "⌘", "Komendy GM"),
        ("reference.changelog", "☷", "Changelog"),
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
