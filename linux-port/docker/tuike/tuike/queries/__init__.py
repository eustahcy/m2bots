"""Every SQL statement Tuike runs, grouped by what it is asking about.

Views call into here and get plain dictionaries back. No module in this package
imports Flask, so the workers can reuse the same queries.
"""
from . import accounts, characters, economy, guilds, rankings, world  # noqa: F401
