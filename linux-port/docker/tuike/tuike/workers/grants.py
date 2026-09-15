"""The grant worker: hands recorded grants to the game, a few at a time.

It is a separate process because it has to keep running while nobody has the
panel open, and because a request thread must never sit waiting on a queue the
game drains at its own pace.
"""
import time

from .. import config, db
from ..grants import beat, tick
from ..schema import ensure


def main():
    print(f"[tuike-grants] tick co {config.GRANTS_TICK_SECONDS} s", flush=True)
    while True:
        error = ""
        try:
            with db.connect() as connection:
                with connection.cursor() as cursor:
                    ensure(cursor)
                tick(connection)
        except Exception as exc:
            error = f"{type(exc).__name__}: {exc}"
            print(f"[tuike-grants] {error}", flush=True)
        try:
            # The stamp goes out whatever the tick did, so the page can tell a
            # dead worker from a failing one.
            beat(error)
        except Exception as exc:
            print(f"[tuike-grants] heartbeat: {type(exc).__name__}: {exc}", flush=True)
        time.sleep(config.GRANTS_TICK_SECONDS)


if __name__ == "__main__":
    main()
