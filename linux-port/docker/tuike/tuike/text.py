"""Turning what the game stored into something a Polish browser can show.

The engine writes CP1250 into columns MySQL believes are something else, so
the driver's own decode gives mojibake for every Polish letter ("Skorzane" came
back as "SkAtrzane"). Reading the column as HEX() and decoding the raw bytes
here sidesteps whatever charset the column claims.
"""

GAME_ENCODINGS = ("cp1250", "utf-8", "latin1")


def game_text(value):
    """Decode a value the driver handed back, whatever it decided it was."""
    if isinstance(value, memoryview):
        value = value.tobytes()
    if isinstance(value, bytes):
        for encoding in GAME_ENCODINGS:
            try:
                return value.decode(encoding)
            except UnicodeDecodeError:
                continue
        return value.decode("cp1250", "replace")
    return value or ""


def hex_text(value):
    """Decode a HEX(column) result as the CP1250 bytes the engine wrote."""
    try:
        return bytes.fromhex(str(value or "")).decode("cp1250")
    except (TypeError, ValueError, UnicodeDecodeError):
        return ""


def thousands(value):
    """1234567 -> '1 234 567'. The separator Polish uses, not a comma."""
    try:
        return f"{int(value or 0):,}".replace(",", " ")
    except (TypeError, ValueError):
        return "0"


def hours(minutes):
    """player.playtime is minutes; the panel always shows whole hours."""
    try:
        return int(minutes or 0) // 60
    except (TypeError, ValueError):
        return 0


def clamp(value, low, high, default=None):
    """An integer from untrusted input, forced into range."""
    try:
        return max(low, min(high, int(value)))
    except (TypeError, ValueError):
        return default if default is not None else low
