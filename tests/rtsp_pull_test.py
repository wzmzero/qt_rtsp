#!/usr/bin/env python3
"""
独立 RTSP 拉流测试程序（与 Qt 客户端解耦）
- 第一步：ffprobe 验证 RTSP 地址可访问
- 第二步：ffmpeg 走 TCP + 软解码，连续解码若干秒
返回码：0 成功，非 0 失败
"""

import argparse
import subprocess
import sys


def run(cmd, timeout=20):
    print("[CMD]", " ".join(cmd))
    p = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, timeout=timeout)
    print(p.stdout.strip())
    return p.returncode, p.stdout


def check_probe(url: str) -> bool:
    cmd = [
        "ffprobe",
        "-v", "error",
        "-rtsp_transport", "tcp",
        "-select_streams", "v:0",
        "-show_entries", "stream=codec_name,width,height",
        "-of", "default=nw=1:nk=1",
        url,
    ]
    rc, out = run(cmd, timeout=15)
    ok = rc == 0 and "h264" in out.lower()
    print(f"[PROBE] {'PASS' if ok else 'FAIL'}")
    return ok


def check_decode(url: str, sec: int) -> bool:
    # 强制软解码，避免 VAAPI/CUDA 环境差异导致误判
    cmd = [
        "ffmpeg",
        "-v", "warning",
        "-rtsp_transport", "tcp",
        "-fflags", "+genpts+discardcorrupt",
        "-flags", "low_delay",
        "-hwaccel", "none",
        "-t", str(sec),
        "-i", url,
        "-an", "-sn", "-dn",
        "-f", "null", "-",
    ]
    rc, out = run(cmd, timeout=sec + 20)
    bad = ["could not", "error", "invalid data", "connection refused"]
    ok = rc == 0 and not any(k in out.lower() for k in bad)
    print(f"[DECODE] {'PASS' if ok else 'FAIL'}")
    return ok


def main():
    ap = argparse.ArgumentParser(description="RTSP pull test (module style)")
    ap.add_argument("--url", default="rtsp://127.0.0.1:8554/live", help="RTSP URL")
    ap.add_argument("--seconds", type=int, default=6, help="Decode duration seconds")
    args = ap.parse_args()

    probe_ok = check_probe(args.url)
    decode_ok = check_decode(args.url, args.seconds)

    if probe_ok and decode_ok:
        print("RESULT: PASS")
        return 0
    print("RESULT: FAIL")
    return 1


if __name__ == "__main__":
    sys.exit(main())
