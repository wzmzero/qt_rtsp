#!/usr/bin/env python3
import sqlite3
from pathlib import Path

DB = Path('/home/wzm/qt_rtsp/config/client_cfg.db')

req = {
    'telemetry': {'sent_ts_ms','source_ts_ms'},
    'detection_object': {'telemetry_id','label_id','label','confidence','cx','cy','w','h'},
    'gps': {'telemetry_id','time_usec','lat_e7','lon_e7','alt_mm','eph_cm','epv_cm','vel_cms','cog_cdeg','fix_type','satellites_visible'},
    'app_config': {'id','rtsp_url','tcp_host'},
    'app_logs': {'id','ts_ms','level','type','message'},
}

conn = sqlite3.connect(DB)
cur = conn.cursor()

def cols(table):
    cur.execute(f"PRAGMA table_info({table})")
    return {r[1] for r in cur.fetchall()}

ok = True
for t, cset in req.items():
    got = cols(t)
    miss = cset - got
    if miss:
        ok = False
        print(f"[FAIL] {t} missing columns: {sorted(miss)}")
    else:
        print(f"[OK] {t}")

# Foreign key sanity
cur.execute("PRAGMA foreign_key_list(detection_object)")
fk1 = cur.fetchall()
cur.execute("PRAGMA foreign_key_list(gps)")
fk2 = cur.fetchall()
if not fk1:
    ok = False
    print('[FAIL] detection_object has no foreign key')
if not fk2:
    ok = False
    print('[FAIL] gps has no foreign key')

print('RESULT:', 'PASS' if ok else 'FAIL')
raise SystemExit(0 if ok else 1)
