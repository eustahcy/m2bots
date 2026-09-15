"""Render every Tuike page against a stubbed database.

No MySQL here: pymysql.connect is replaced with a fake whose cursor answers
from a small table of SQL fragments. That is enough to exercise template
syntax, every url_for endpoint, the macros and the view logic.
"""
import datetime
import os
import sys
import traceback

# test_writes.py imports this module purely to install the fake connection, so
# the panel root falls back to the directory above this one.
ROOT = os.path.abspath(
    os.environ.get("TUIKE_ROOT")
    or (sys.argv[1] if len(sys.argv) > 1 else os.path.join(os.path.dirname(__file__), ".."))
)
sys.path.insert(0, ROOT)

os.environ.setdefault("DB_USER", "test")
os.environ.setdefault("DB_PASSWORD", "test")
os.environ.setdefault("PLAYERBOTS_ENGINE", "mt2009")
os.environ.setdefault("TUIKE_SESSION_SECRET", "smoke-test-secret")
# Point the spools somewhere that does not exist, so every reader takes its
# "nothing is there" path - the state a fresh install is actually in.
os.environ.setdefault("TUIKE_RATES_SPOOL", os.path.join(ROOT, "_no_spool"))
os.environ.setdefault("TUIKE_UPDATE_SPOOL", os.path.join(ROOT, "_no_update"))
os.environ.setdefault("TUIKE_UPDATE_CHECK", "0")

import pymysql  # noqa: E402

NOW = datetime.datetime(2026, 9, 14, 12, 0, 0)

CHARACTER = {
    "id": 7, "account_id": 3, "name": "botarek7", "level": 42, "job": 2, "exp": 1200,
    "gold": 987654, "hp": 900, "mp": 400, "x": 110000, "y": 210000, "horse_level": 11,
    "alignment": 55000, "st": 20, "ht": 15, "dx": 12, "iq": 9, "stat_point": 3,
    "skill_point": 2, "skill_group": 1, "skill_level": bytes(1200), "map_index": 23,
    "playtime": 4321, "empire": 2,
}
ITEM = {
    "id": 1, "vnum": 189, "count": 1, "window": "EQUIPMENT", "pos": 4,
    "socket0": 28030, "socket1": 0, "socket2": 0,
    "attrtype0": 121, "attrvalue0": 5, "attrtype1": 0, "attrvalue1": 0,
    "attrtype2": 0, "attrvalue2": 0, "attrtype3": 0, "attrvalue3": 0,
    "attrtype4": 0, "attrvalue4": 0, "attrtype5": 0, "attrvalue5": 0,
    "attrtype6": 0, "attrvalue6": 0,
    "applytype0": 6, "applyvalue0": 3, "applytype1": 0, "applyvalue1": 0,
    "applytype2": 0, "applyvalue2": 0,
    "item_size": 2, "item_name": b"Miecz Wojownika",
}
INVENTORY_ITEM = {**ITEM, "id": 2, "window": "INVENTORY", "pos": 46, "count": 12}
STONE = {"vnum": 28030, "item_name": b"Kamie\xf1 Duszy", "applytype0": 6, "applyvalue0": 4,
         "applytype1": 0, "applyvalue1": 0, "applytype2": 0, "applyvalue2": 0}
GUILD = {"id": 1, "name": "Testowa", "level": 5, "exp": 10, "sp": 0, "win": 3, "draw": 1,
         "loss": 2, "ladder_point": 1200, "gold": 50000, "leader_id": 7,
         "leader_name": "botarek7", "leader_level": 42, "member_count": 9}
RANK_ROW = {"id": 7, "name": "botarek7", "level": 42, "gold": 987654, "score": 5,
            "detail": b"Szczeg\xf3\x88", "vnum": 189, "item_name": b"Miecz",
            "skill_damage": 12, "avg_damage": 9, "job": 2, "exp": 100,
            "skill_group": 1, "skill_level": bytes(1200), "playtime": 4321,
            "horse_level": 11, "empire": 2, "map_index": 23}

# (fragment that must appear in the SQL, rows to return). First match wins, so
# the most specific fragments come first.
ROUTES = [
    # The character profile first: its FROM clause also matches the broader
    # player fragments further down.
    ("SELECT p.id, p.account_id, p.name", [dict(CHARACTER)]),
    ("AS yang FROM player.player", [{"yang": 99887766}]),
    ("FROM player.item_proto p WHERE", [{"vnum": 50051, "locale_name": b"Zdj\xeacie Konia",
                                         "type": 3, "size": 1}]),
    ("SELECT vnum, locale_name, type, size FROM player.item_proto",
     [{"vnum": 50051, "locale_name": b"Zdj\xeacie Konia", "type": 3, "size": 1}]),
    ("COALESCE(locale_name, CONCAT('VNUM ', vnum)) AS item_name", [dict(STONE)]),
    ("SELECT p.vnum, p.name, p.locale_name", [{"vnum": 189, "name": b"sword",
                                               "locale_name": b"Miecz", "type": 1,
                                               "subtype": 0, "size": 2, "gold": 100,
                                               "shop_buy_price": 50}]),
    ("SELECT type, COUNT(*) AS count", [{"type": 1, "count": 12, "icon_vnum": 189}]),
    ("COUNT(*) AS count FROM player.item_proto", [{"count": 12}]),
    # Gospodarka: Ikarus Shop stalls (player.ikashop_offlineshop, item rows with
    # window='IKASHOP_OFFLINESHOP') and the trade log (log.ikarusshop_log). All
    # of these must stay above "FROM player.item i" and the weapon30/plus9
    # fragment right below - both are broad enough to otherwise swallow them.
    ("FROM player.ikashop_offlineshop s JOIN player.player p ON p.id = s.owner LEFT JOIN player.item i",
     [{"owner": 7, "shop_name": "Kramik Testowy", "map": 1, "is_premium": 0,
       "owner_name": "botarek7", "owner_job": 2, "item_count": 3}]),
    ("FROM player.ikashop_offlineshop s JOIN player.player p ON p.id = s.owner WHERE s.owner",
     [{"owner": 7, "shop_name": "Kramik Testowy", "map": 1, "is_premium": 0,
       "owner_name": "botarek7", "owner_job": 2}]),
    ("i.vnum, i.count, i.ikashop_data,",
     [{"id": 11, "vnum": 189, "count": 3, "item_name": b"Miecz Wojownika",
       "ikashop_data": '{"yang": 5000}'}]),
    ("i.ikashop_data, i.owner_id, p.name AS owner_name",
     [{"id": 12, "count": 2, "owner_id": 8, "owner_name": "botarek8",
       "ikashop_data": '{"yang": 3200}'}]),
    ("FROM log.ikarusshop_log l LEFT JOIN player.player buyer",
     [{"id": 1, "time": NOW, "vnum": 189, "count": 1, "yang": 5000,
       "buyer_id": 7, "buyer_name": "botarek7", "buyer_job": 2,
       "seller_id": 8, "seller_name": "botarek8", "seller_job": 2,
       "item_name": b"Miecz Wojownika"}]),
    ("FROM log.ikarusshop_log l LEFT JOIN player.item_proto p ON p.vnum = l.vnum",
     [{"vnum": 189, "sales": 5, "units": 10, "turnover": 50000, "avg_price": 5000,
       "item_name": b"Miecz Wojownika"}]),
    ("COUNT(*) FROM player.ikashop_offlineshop", [{"count": 4}]),
    ("COUNT(*) FROM player.item WHERE window = 'IKASHOP_OFFLINESHOP'", [{"count": 42}]),
    ("FROM log.ikarusshop_log WHERE what = 'BUY_ITEM' AND time",
     [{"count": 12, "yang": 250000}]),
    # weapon30 and plus9 join the other way round and select a character.
    ("FROM player.item i JOIN player.player p ON p.id = i.owner_id", [dict(RANK_ROW)]),
    ("FROM player.item i", [dict(ITEM), dict(INVENTORY_ITEM)]),
    ("FROM player.guild_member gm", [{"pid": 7, "grade": 1, "is_general": 1, "offer": 500,
                                      "name": "botarek7", "level": 42, "job": 2,
                                      "map_index": 23, "playtime": 100}]),
    ("FROM player.guild g", [dict(GUILD)]),
    ("FROM log.log l", [{"time": NOW, "how": b"STONE_KILL", "hint": b"+7", "hint_hex": "2B37",
                         "what": b"", "who": 7, "name": "botarek7",
                         "item_name_hex": "4D6965637A", "x": 110000, "y": 210000,
                         "score": 5, "id": 7, "level": 42, "gold": 1, "metins": 3,
                         "bosses": 1, "refine7": 2, "detail": b"x"}]),
    ("FROM log.log WHERE who", [{"time": NOW, "type": b"ITEM", "how": b"GET",
                                 "hint": b"co\xb6", "what": b"1"}]),
    ("FROM account.account a", [{"id": 3, "login": "playerbot_003", "email": "",
                                 "empire": 2, "create_time": NOW, "last_play": NOW}]),
    ("FROM player.player p LEFT JOIN account.account a ON a.id = p.account_id",
     [{"id": 7, "name": "botarek7", "level": 42, "horse_level": 11, "playtime": 4321,
       "job": 2, "riding": 7}]),
    ("FROM player.player p", [dict(RANK_ROW)]),
    # More specific than the fallback right below, and must stay above it:
    # ROUTES matching is first-substring-wins by list position, not by which
    # fragment is more specific, so a generic rule earlier in the list would
    # otherwise swallow this query before it is ever reached.
    ("SELECT name FROM player.player WHERE NOT", [{"name": "TestGracz"}]),
    ("FROM player.player WHERE", [dict(RANK_ROW)]),
    ("SELECT id, job FROM player.player", [{"id": 7, "job": 2}]),
    ("SELECT id, name, level, exp, job, map_index, playtime", [dict(RANK_ROW)]),
    ("SELECT id, name, level, job, map_index, gold, playtime, last_play",
     [{"id": 7, "name": "botarek7", "level": 42, "job": 2, "map_index": 23,
       "gold": 1000, "playtime": 4321, "last_play": NOW}]),
    ("SELECT p.id, p.account_id, p.name", [dict(CHARACTER)]),
    ("AS characters", [{"characters": 1500, "accounts": 1010, "item_stacks": 42000,
                        "yang": 99887766}]),
    ("COUNT(*) AS count", [{"count": 3}]),
    ("MAX(captured_at) AS captured_at", [{"captured_at": NOW}]),
    ("web_tuike_item_snapshot s", [{"vnum": 189, "amount": 120, "item_name": b"Miecz"}]),
    ("web_tuike_item_snapshot", [{"captured_at": "09-14 12:00", "amount": 120}]),
    ("web_tuike_metric_snapshot", [{"captured_at": "09-14 12:00", "value": 99887766}]),
    ("web_tuike_map_snapshot", [{"label": "09-14 12:00", "map_index": 23,
                                 "character_count": 7}]),
    ("web_tuike_system_snapshot", [{"label": "12:00", "captured_at": NOW,
                                    "cpu_percent": 31.5, "ram_percent": 44.0,
                                    "ram_used_mb": 3800, "ram_total_mb": 8000,
                                    "disk_percent": 55.0, "disk_used_mb": 40000,
                                    "disk_total_mb": 80000}]),
    ("web_tuike_grants", []),
    ("web_tuike_settings", []),
    # The grant page asks for counts; commands.py asks one row for its status.
    ("SELECT status FROM player.web_admin_queue", [{"status": "done"}]),
    ("FROM common.gmlist", [{"rank": "LOW_WIZARD", "mName": "botarek7", "a": "LOW_WIZARD"}]),
    # warp_operator_to: the rows it just queued, and the poll that finds one
    # of them answered. (Who might be online is matched further up, ahead of
    # the generic player.player fallback it would otherwise fall behind.)
    ("SELECT id, player_name FROM player.web_admin_queue WHERE cmd = 'WARP'",
     [{"id": 1, "player_name": "TestGracz"}]),
    ("SELECT id, status FROM player.web_admin_queue WHERE id IN",
     [{"id": 1, "status": "done"}]),
    ("web_admin_queue", [{"n": 0, "oldest": None}]),
    ("player.quest", [{"szName": "make_herb_lv4"}]),
    ("GET_LOCK", [{"acquired": 1}]),
    ("RELEASE_LOCK", [{"released": 1}]),
]


class FakeCursor:
    def __init__(self):
        self.rows = []
        self.rowcount = 0
        self.lastrowid = 1

    def __enter__(self):
        return self

    def __exit__(self, *exc):
        return False

    def execute(self, sql, params=()):
        flat = " ".join(str(sql).split())
        self.rows = []
        # 999999 is the smoke test's "no such record", so the not-found paths
        # get exercised rather than always finding the canned row.
        if 999999 in (params if isinstance(params, (list, tuple)) else (params,)):
            self.rowcount = 0
            return 0
        for fragment, rows in ROUTES:
            if " ".join(fragment.split()) in flat:
                self.rows = [dict(row) for row in rows]
                break
        self.rowcount = len(self.rows)
        return self.rowcount

    def executemany(self, sql, seq):
        self.rows = []
        self.rowcount = len(list(seq))
        return self.rowcount

    def fetchall(self):
        return list(self.rows)

    def fetchone(self):
        return dict(self.rows[0]) if self.rows else None

    def close(self):
        pass


class FakeConnection:
    def __enter__(self):
        return self

    def __exit__(self, *exc):
        return False

    def cursor(self):
        return FakeCursor()

    def begin(self):
        pass

    def commit(self):
        pass

    def rollback(self):
        pass

    def close(self):
        pass


pymysql.connect = lambda *args, **kwargs: FakeConnection()

PAGES = [
    ("/", 200), ("/players", 200), ("/players?q=bot", 200),
    ("/player/7", 200), ("/guilds", 200), ("/guild/1", 200),
    ("/rankings", 200), ("/rankings?type=weapon30&sort=skill", 200),
    ("/rankings?type=skills", 200), ("/rankings?type=biologist", 200),
    ("/rankings?type=shops", 200), ("/rankings?type=plus9", 200),
    ("/rankings?type=nonsense", 200),
    ("/economy", 200), ("/economy?q=miecz", 200), ("/economy/item/189", 200),
    ("/economy/shops", 200), ("/economy/shop/7", 200), ("/economy/shop/999999", 404),
    ("/items", 200), ("/items?type=1&q=miecz", 200),
    ("/maps", 200), ("/season", 200), ("/system", 200),
    ("/manage", 200), ("/manage/items", 200),
    ("/accounts", 200), ("/accounts?q=bot&display=all", 200),
    ("/gm-commands", 200), ("/changelog", 200),
    ("/setup", 302), ("/login", 302),
    ("/api/live-bots", 200), ("/api/news", 200), ("/api/system", 200),
    ("/api/manage-status", 200), ("/api/heat-events?type=metins", 200),
    ("/api/heat-events?type=nope", 400),
    ("/api/status", 200), ("/api/items?q=miecz", 200), ("/api/items?q=189", 200),
    ("/api/economy", 200), ("/api/economy/market", 200),
    ("/player/999999", 404), ("/nie-ma-takiej-strony", 404),
]

def run():
  from tuike import create_app

  app = create_app()
  failures = []
  with app.test_client() as client:
    for path, expected in PAGES:
        try:
            response = client.get(path)
        except Exception:
            failures.append((path, "WYJATEK", traceback.format_exc()))
            continue
        if response.status_code != expected:
            body = response.get_data(as_text=True)
            failures.append((path, f"{response.status_code} != {expected}", body[-1600:]))
        else:
            print(f"  OK  {response.status_code}  {path}")

    # A POST without a CSRF token must be refused, on every form.
    for path in ("/manage/settings", "/manage/behavior", "/manage/restart",
                 "/manage/restart-config", "/manage/clear-stale", "/manage/update",
                 "/manage/language", "/accounts",
                 "/player/7/action", "/player/7/gm", "/player/7/warp-me"):
        response = client.post(path, data={})
        if response.status_code != 403:
            failures.append((path, f"POST bez CSRF dal {response.status_code}, oczekiwano 403", ""))
        else:
            print(f"  OK  403 (brak CSRF)  POST {path}")

  print()
  if failures:
    print(f"NIEPOWODZENIA: {len(failures)}")
    for path, why, detail in failures:
        print("=" * 70)
        print(f"{path}  ->  {why}")
        print(detail)
    sys.exit(1)
  print(f"Wszystkie {len(PAGES)} stron i 11 testow CSRF przeszlo.")


if __name__ == "__main__":
    run()
