"""A few seconds of memory for the queries that scan log.log.

log.log has indexes on who, what and how - never on time - so anything that
asks "per hour, over the last day" or "per character, over all time" reads
far more rows than the page shows. The dashboard polls, and several operators
can have it open; remembering the answer for a minute or two costs a slightly
stale number and saves the database from doing the same scan per request.

Per process, on purpose: gunicorn workers each keep their own copy, which is a
few duplicate scans at worst and needs no shared state to go wrong.
"""
import copy
import functools
import threading
import time

_lock = threading.Lock()


def ttl(seconds):
    """Remember a function's result per argument tuple for `seconds`."""
    def decorate(function):
        store = {}

        @functools.wraps(function)
        def wrapper(*args):
            now = time.monotonic()
            with _lock:
                hit = store.get(args)
                if hit and now - hit[0] < seconds:
                    # A copy, so a caller decorating its rows in place does
                    # not decorate the remembered ones for everyone after it.
                    return copy.deepcopy(hit[1])
            value = function(*args)
            with _lock:
                store[args] = (now, copy.deepcopy(value))
            return value

        wrapper.cache_clear = store.clear
        return wrapper
    return decorate
