"""Periodic snapshots: host load, map population, bot positions, the economy.

Everything with a history in Tuike comes from here. It runs in its own
container because the numbers are expensive to gather and nobody should pay for
them on a page load - and because the host's /proc has to be mounted somewhere,
and that somewhere should not be the process facing the network.
"""
import os
import time
from datetime import datetime

from .. import config, db
from ..live import positions
from ..schema import ensure

HOST_PROC = "/host/proc"
HOST_ROOT = "/hostfs"


def host_metrics(previous=None):
    """CPU, memory and disk for the host - or, on Docker Desktop, its VM.

    CPU needs two readings: the first call has none to compare with, so it
    falls back to the load average, which is close enough for one sample and
    always available.
    """
    try:
        with open(f"{HOST_PROC}/stat") as handle:
            fields = handle.readline().split()[1:]
        total = sum(int(value) for value in fields)
        idle = int(fields[3]) + int(fields[4])

        with open(f"{HOST_PROC}/meminfo") as handle:
            memory = {line.split(":")[0]: int(line.split()[1]) for line in handle if ":" in line}
        total_mb = memory["MemTotal"] // 1024
        available = memory.get("MemAvailable", memory.get("MemFree", 0))
        used_mb = (memory["MemTotal"] - available) // 1024

        disk = os.statvfs(HOST_ROOT)
        disk_total_mb = (disk.f_blocks * disk.f_frsize) // (1024 * 1024)
        disk_used_mb = ((disk.f_blocks - disk.f_bavail) * disk.f_frsize) // (1024 * 1024)

        if previous:
            busy = 1 - (idle - previous[1]) / max(1, total - previous[0])
            cpu = round(100 * busy, 1)
        else:
            with open(f"{HOST_PROC}/loadavg") as handle:
                load = float(handle.read().split()[0])
            with open(f"{HOST_PROC}/cpuinfo") as handle:
                cores = max(1, sum(1 for line in handle if line.startswith("processor")))
            cpu = round(min(100, 100 * load / cores), 1)
        return (total, idle), cpu, used_mb, total_mb, disk_used_mb, disk_total_mb
    except (OSError, KeyError, ValueError, IndexError):
        # No /proc mount, or a kernel that lays it out differently. Zeroes are
        # honest here: the panel draws an empty chart rather than a wrong one.
        return previous, 0, 0, 0, 0, 0


def collect(connection, previous):
    """One snapshot, stamped to the minute so re-runs collapse into one row."""
    now = datetime.now().replace(second=0, microsecond=0)
    previous, cpu, used, total, disk_used, disk_total = host_metrics(previous)
    ram_percent = round(100 * used / total, 1) if total else 0
    disk_percent = round(100 * disk_used / disk_total, 1) if disk_total else 0

    with connection.cursor() as cursor:
        ensure(cursor)
        cursor.execute(
            f"""INSERT INTO {config.SYSTEM_SNAPSHOT_TABLE}
                (captured_at, cpu_percent, ram_percent, ram_used_mb, ram_total_mb,
                 disk_percent, disk_used_mb, disk_total_mb)
                VALUES (%s,%s,%s,%s,%s,%s,%s,%s)
                ON DUPLICATE KEY UPDATE
                  cpu_percent=VALUES(cpu_percent), ram_percent=VALUES(ram_percent),
                  ram_used_mb=VALUES(ram_used_mb), ram_total_mb=VALUES(ram_total_mb),
                  disk_percent=VALUES(disk_percent), disk_used_mb=VALUES(disk_used_mb),
                  disk_total_mb=VALUES(disk_total_mb)""",
            (now, cpu, ram_percent, used, total, disk_percent, disk_used, disk_total),
        )

        where = positions()
        counts = {}
        for map_index, _x, _y in where.values():
            counts[map_index] = counts.get(map_index, 0) + 1
        if counts:
            cursor.executemany(
                f"INSERT IGNORE INTO {config.MAP_SNAPSHOT_TABLE} VALUES (%s,%s,%s)",
                [(now, map_index, count) for map_index, count in counts.items()],
            )
        if where:
            # The stuck check and the grant worker's "who is online" both read
            # these rows; they are the only record of where a bot was.
            cursor.executemany(
                f"INSERT IGNORE INTO {config.POSITION_SNAPSHOT_TABLE} VALUES (%s,%s,%s,%s,%s)",
                [(now, pid, map_index, x, y) for pid, (map_index, x, y) in where.items()],
            )

        cursor.execute(
            f"""INSERT IGNORE INTO {config.ITEM_SNAPSHOT_TABLE} (captured_at, vnum, amount)
                SELECT %s, vnum, SUM(count) FROM player.item GROUP BY vnum""",
            (now,),
        )
        cursor.execute(
            "SELECT COALESCE(SUM(gold),0) AS yang FROM player.player"
            " WHERE name NOT IN ('[SA]Admin','Test')"
        )
        yang = cursor.fetchone()["yang"]
        cursor.execute(
            f"INSERT IGNORE INTO {config.METRIC_SNAPSHOT_TABLE} VALUES (%s,'total_yang',%s)",
            (now, yang),
        )
    return previous, len(where), len(counts)


def main():
    previous = None
    print(f"[tuike-collector] co {config.COLLECTOR_INTERVAL} s", flush=True)
    while True:
        try:
            with db.connect() as connection:
                previous, bots, maps = collect(connection, previous)
                print(f"[tuike-collector] snapshot: {bots} botów na {maps} mapach", flush=True)
        except Exception as exc:
            print(f"[tuike-collector] {type(exc).__name__}: {exc}", flush=True)
        time.sleep(config.COLLECTOR_INTERVAL)


if __name__ == "__main__":
    main()
