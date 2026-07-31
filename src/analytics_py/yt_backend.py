#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
yt_backend.py  –  YoutubeDownloader v2.0 Python 백엔드
C# WPF UI에서 subprocess.Start()로 실행되며,
JSON Lines 형식으로 stdout에 진행 상황을 출력합니다.

Usage:
    python yt_backend.py <url> [options]

Output (JSON Lines to stdout):
    {"type":"info",  "msg":"..."}
    {"type":"progress", "percent":45.2, "speed":"3.2 MiB/s", "eta":"00:01:23", "filename":"영상.mp4"}
    {"type":"done",  "status":"completed", "files":2, "total_bytes":1234567}
    {"type":"error", "msg":"..."}
"""

import os
import sys
import re
import json
import shutil
import argparse
import zipfile
import tempfile
import urllib.request
from pathlib import Path

# ── 콘솔 UTF-8 강제 설정 ───────────────────────────────────
if sys.platform == "win32":
    try:
        if sys.stdout and hasattr(sys.stdout, "reconfigure"):
            sys.stdout.reconfigure(encoding="utf-8", errors="replace")
        if sys.stderr and hasattr(sys.stderr, "reconfigure"):
            sys.stderr.reconfigure(encoding="utf-8", errors="replace")
    except Exception:
        pass

# ── 경로 설정 ──────────────────────────────────────────────
SCRIPT_DIR   = Path(__file__).resolve().parent
PROJECT_ROOT = SCRIPT_DIR.parent.parent   # test1/
DATA_DIR     = PROJECT_ROOT / "data"
DOWNLOAD_DIR = DATA_DIR / "downloads"
BIN_DIR      = DATA_DIR / "bin"


def emit(obj: dict):
    """JSON Lines 1줄을 stdout으로 출력 (C# ReadLine()으로 수신)"""
    print(json.dumps(obj, ensure_ascii=False), flush=True)


def emit_info(msg: str):
    emit({"type": "info", "msg": msg})


def emit_error(msg: str):
    emit({"type": "error", "msg": msg})


# ── ffmpeg 탐색 ────────────────────────────────────────────
def find_ffmpeg() -> str | None:
    """ffmpeg.exe 경로 반환 (없으면 None)"""
    ff = shutil.which("ffmpeg")
    if ff:
        return str(Path(ff).parent)

    for p in [BIN_DIR, PROJECT_ROOT / "bin", PROJECT_ROOT / "ffmpeg"]:
        if (p / "ffmpeg.exe").exists():
            return str(p)

    user = os.environ.get("USERPROFILE", "")
    if user:
        candidates = list(Path(user).glob(
            "AppData/Local/Microsoft/WinGet/Packages/**/*ffmpeg.exe"))
        if candidates:
            return str(candidates[0].parent)
    return None


def ensure_ffmpeg() -> str | None:
    """ffmpeg 없으면 자동 다운로드 (data/bin 폴더)"""
    loc = find_ffmpeg()
    if loc:
        return loc

    BIN_DIR.mkdir(parents=True, exist_ok=True)
    target = BIN_DIR / "ffmpeg.exe"
    if target.exists():
        return str(BIN_DIR)

    emit_info("📦 [최초 설정] FFmpeg 자동 다운로드 중...")
    url = ("https://github.com/yt-dlp/FFmpeg-Builds/releases/download/"
           "latest/ffmpeg-master-latest-win64-gpl.zip")
    tmp_zip = BIN_DIR / "_ffmpeg_tmp.zip"
    try:
        req = urllib.request.Request(url, headers={"User-Agent": "Mozilla/5.0"})
        with urllib.request.urlopen(req) as resp, open(tmp_zip, "wb") as f:
            shutil.copyfileobj(resp, f)

        with zipfile.ZipFile(tmp_zip, "r") as z:
            for m in z.namelist():
                if m.endswith("ffmpeg.exe"):
                    with z.open(m) as src, open(target, "wb") as dst:
                        shutil.copyfileobj(src, dst)
                elif m.endswith("ffprobe.exe"):
                    with z.open(m) as src, open(BIN_DIR / "ffprobe.exe", "wb") as dst:
                        shutil.copyfileobj(src, dst)

        tmp_zip.unlink(missing_ok=True)
        if target.exists():
            emit_info("✅ FFmpeg 설치 완료!")
            return str(BIN_DIR)
    except Exception as e:
        emit_error(f"FFmpeg 자동 다운로드 실패: {e}")
    return None


# ── 화질 포맷 스펙 ─────────────────────────────────────────
def get_format_spec(quality: str) -> str:
    if not quality or "best" in quality.lower():
        return "bv*+ba/b/best"
    if "worst" in quality.lower():
        return "b[height<=360]/worst"
    m = re.search(r"\d+", quality)
    if not m:
        return "bv*+ba/b/best"
    h = m.group(0)
    return f"bv*[height<={h}]+ba/b[height<={h}]+ba/best"


# ── 파일 스캔 ──────────────────────────────────────────────
def scan_files(directory: Path) -> set:
    if not directory.exists():
        return set()
    return {p for p in directory.rglob("*") if p.is_file()}


# ── 메인 다운로드 함수 ─────────────────────────────────────
def run_download(args):
    try:
        import yt_dlp
    except ImportError:
        emit_error("yt-dlp 가 설치되어 있지 않습니다. pip install yt-dlp 실행 후 다시 시도하세요.")
        sys.exit(1)

    out_dir      = Path(args.output).resolve() if args.output else DOWNLOAD_DIR
    archive_file = out_dir / "archive.txt"
    out_dir.mkdir(parents=True, exist_ok=True)

    before_files = scan_files(out_dir)
    format_spec  = get_format_spec(args.quality)

    # ── 진행률 파싱용 정규식 ──
    PERCENT_RE = re.compile(r"(\d+\.?\d*)\s*%")
    SPEED_RE   = re.compile(r"(\d+\.?\d*\s*[KMG]iB/s)")
    ETA_RE     = re.compile(r"ETA\s+(\d{2}:\d{2}(?::\d{2})?)")
    FNAME_RE   = re.compile(r"\[download\] Destination:\s*(.+)")

    current_filename = ""

    class JsonLogger:
        def debug(self, msg):
            if not msg or msg.startswith("[debug] "):
                return
            # 파일명 추출
            nonlocal current_filename
            m = FNAME_RE.search(msg)
            if m:
                current_filename = Path(m.group(1).strip()).name

            # 진행률 파싱
            pm = PERCENT_RE.search(msg)
            if pm:
                pct = float(pm.group(1))
                speed = (SPEED_RE.search(msg) or type("", (), {"group": lambda s, i: ""})()).group(1) if SPEED_RE.search(msg) else ""
                eta   = ETA_RE.search(msg).group(1) if ETA_RE.search(msg) else ""
                emit({"type": "progress", "percent": pct,
                      "speed": speed, "eta": eta,
                      "filename": current_filename})
            else:
                emit_info(msg.strip())

        def info(self, msg):
            if msg:
                emit_info(msg.strip())

        def warning(self, msg):
            if msg:
                emit_info(f"⚠️ {msg.strip()}")

        def error(self, msg):
            if msg:
                emit_error(msg.strip())

    def progress_hook(d):
        if d.get("status") == "downloading":
            pct   = d.get("_percent_str", "0%").strip().replace("%", "")
            speed = d.get("_speed_str", "").strip()
            eta   = d.get("_eta_str", "").strip()
            fname = Path(d.get("filename", "")).name
            try:
                emit({"type": "progress", "percent": float(pct),
                      "speed": speed, "eta": eta, "filename": fname})
            except ValueError:
                pass

    ydl_opts = {
        "format":              format_spec,
        "outtmpl":             str(out_dir / "%(title)s [%(id)s].%(ext)s"),
        "merge_output_format": "mp4",
        "noplaylist":          False,
        "continue_dl":         True,
        "nocheckcertificate":  True,
        "logger":              JsonLogger(),
        "progress_hooks":      [progress_hook],
    }

    ff_loc = ensure_ffmpeg()
    if ff_loc:
        ydl_opts["ffmpeg_location"] = ff_loc
    else:
        emit_info("⚠️ ffmpeg 없음 – 단일 스트림(b/best)으로 다운로드합니다.")
        ydl_opts["format"] = "b/best"

    if not args.ignore_archive:
        ydl_opts["download_archive"] = str(archive_file)
    if args.playlist_items:
        ydl_opts["playlist_items"] = args.playlist_items
    if args.cookies and args.cookies != "none":
        ydl_opts["cookiesfrombrowser"] = (args.cookies,)
    if args.save_sub:
        ydl_opts["writesubtitles"]  = True
        ydl_opts["subtitleslangs"]  = ["ko", "en"]
        ydl_opts["postprocessors"]  = [{"key": "FFmpegSubtitlesConvertor", "format": "srt"}]

    emit_info(f"저장 폴더: {out_dir}")
    return_code = 0

    try:
        with yt_dlp.YoutubeDL(ydl_opts) as ydl:
            return_code = ydl.download([args.url])
    except Exception as e:
        emit_error(f"다운로드 오류: {e}")
        return_code = 1
    finally:
        after_files  = scan_files(out_dir)
        new_files    = sorted(after_files - before_files)
        total_bytes  = sum(f.stat().st_size for f in new_files)
        status       = "completed" if return_code == 0 else ("failed" if not new_files else "partial")

        emit({
            "type":        "done",
            "status":      status,
            "files":       len(new_files),
            "total_bytes": total_bytes,
            "filenames":   [f.name for f in new_files],
        })

    return return_code


# ── CLI 진입점 ─────────────────────────────────────────────
def main():
    parser = argparse.ArgumentParser(description="yt_backend – JSON Lines stdout 다운로더")
    parser.add_argument("url",                            help="유튜브 URL")
    parser.add_argument("--output",           default="", help="저장 폴더")
    parser.add_argument("--quality",          default="best")
    parser.add_argument("--cookies",          default="none")
    parser.add_argument("--ignore-archive",   action="store_true")
    parser.add_argument("--save-sub",         action="store_true")
    parser.add_argument("--playlist-items",   default="")
    args = parser.parse_args()

    sys.exit(run_download(args))


if __name__ == "__main__":
    main()
