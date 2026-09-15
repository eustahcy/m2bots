"""Constants taken from the game's own headers, quests and client files.

Nothing in here talks to the database or the filesystem beyond loading the
three JSON extracts that ship with the panel, so it can be imported by the web
app and by both workers without side effects.
"""
from . import bots, characters, items, maps  # noqa: F401
