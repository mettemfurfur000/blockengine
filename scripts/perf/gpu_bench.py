#!/usr/bin/env python3
"""GPU load test harness for the blockengine client.

Launches build/blockengine_base.exe in a fixed windowed configuration, waits for it to
finish starting up and render its first frames, then samples the GPU load the game
produces over a fixed measurement window.

Two independent GPU signals are captured:

  1. WDDM per-process 3D engine busy time:
       \\GPU Engine(pid_<pid>_*engtype_3d*)\\Running Time
     The delta of this cumulative counter between two samples, divided by the wall time
     between them, is the fraction of time the game's 3D context kept the GPU busy. This
     is per-process, so it isolates exactly what the game does (vsync included).
     Units are 100ns, so busy_seconds = delta / 1e7.

  2. nvidia-smi global utilization + power draw (when an NVIDIA GPU is present). This is
     a global snapshot and is only used as a cross-check.

The metric that matters for pipeline waste is est_gpu_ms_per_frame = util% * (1000 / fps).
At a 60fps vsync cap, util% ~= 100% means the frame is GPU-bound every frame, so an idle
scene burning ~100% is the "redraw everything from scratch" pathology; a healthy idle
scene should sit well below that.

Usage:
  python scripts/perf/gpu_bench.py                  # measure, print report
  python scripts/perf/gpu_bench.py --save-baseline  # record baseline.json
  python scripts/perf/gpu_bench.py --compare        # PASS/FAIL vs baseline.json
  make perf-gpu                                     # same as first invocation
"""

import argparse
import base64
import json
import math
import os
import subprocess
import sys
import threading
import time

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
DEFAULT_EXE = os.path.join(REPO_ROOT, "build", "blockengine_base.exe")
DEFAULT_CWD = os.path.join(REPO_ROOT, "build")
BASELINE_PATH = os.path.join(REPO_ROOT, "scripts", "perf", "baseline.json")
OUT_DIR = os.path.join(REPO_ROOT, "build", "perf")

NVSMI_CANDIDATES = [
    "nvidia-smi.exe",
    r"C:\Windows\System32\nvidia-smi.exe",
    "/usr/bin/nvidia-smi",
]

POWERSHELL = "powershell.exe"


def find_nvsmi():
    for cand in NVSMI_CANDIDATES:
        for base in (cand, os.path.join(os.environ.get("SystemRoot", "C:\\Windows"), "System32", cand)):
            if os.path.isfile(base):
                return base
        if os.name == "nt":
            for p in os.environ.get("PATH", "").split(os.pathsep):
                if p:
                    full = os.path.join(p, cand)
                    if os.path.isfile(full):
                        return full
    return None


def run_powershell(command):
    # powershell.exe -Command mangles embedded double quotes; -EncodedCommand is reliable.
    encoded = base64.b64encode(command.encode("utf-16-le")).decode("ascii")
    try:
        out = subprocess.run(
            [POWERSHELL, "-NoProfile", "-NonInteractive", "-EncodedCommand", encoded],
            capture_output=True,
            text=True,
            timeout=30,
        )
        return out.stdout.strip()
    except Exception:
        return None


def sample_gpu_engine_busy(pid):
    """Cumulative busy-seconds of the process's 3D GPU engine, or None."""
    ps = (
        "[System.Threading.Thread]::CurrentThread.CurrentCulture = "
        "[System.Globalization.CultureInfo]::InvariantCulture; "
        r"$total = $null; "
        r'$c = Get-Counter -Counter "\GPU Engine(pid_{p}_*engtype_3d*)\Running Time" -ErrorAction SilentlyContinue; '
        "if ($c) {{ $total = 0.0; foreach ($s in $c.CounterSamples) {{ $total = $total + $s.CookedValue }}}}; "
        "if ($total -ne $null) {{ [Math]::Round($total / 10000000.0, 9) }}"
    ).format(p=pid)
    line = run_powershell(ps)
    if not line:
        return None
    try:
        return float(line)
    except ValueError:
        return None


def sample_nvidia():
    """Returns (util_pct, power_w) snapshots or None."""
    nvsmi = find_nvsmi()
    if not nvsmi:
        return None
    try:
        out = subprocess.run(
            [nvsmi, "--query-gpu=utilization.gpu,power.draw", "--format=csv,noheader,nounits"],
            capture_output=True,
            text=True,
            timeout=15,
        )
    except Exception:
        return None
    if out.returncode != 0:
        return None
    parts = out.stdout.strip().split(",")
    if len(parts) < 2:
        return None
    util = float(parts[0].strip())
    power = float(parts[1].strip()) if "N/A" not in parts[1] else None
    return util, power


def build_client(exe, force):
    if os.path.isfile(exe) and not force:
        return True
    print("[build] invoking make blockengine_base ...")
    r = subprocess.run(["make", "blockengine_base"], cwd=REPO_ROOT)
    return r.returncode == 0 and os.path.isfile(exe)


def wait_for_gpu_engine(pid, timeout_s):
    deadline = time.monotonic() + timeout_s
    first = None
    while time.monotonic() < deadline:
        busy = sample_gpu_engine_busy(pid)
        if busy is not None and (first is None or busy > first):
            first = busy
        if busy is not None and busy > 0.0:
            return True
        time.sleep(0.5)
    return False


def collect_samples(proc, pid, duration_s, interval_s):
    samples = []
    nv_samples = []
    prev_busy = None
    prev_t = None
    deadline = time.monotonic() + duration_s
    while time.monotonic() < deadline:
        if proc.poll() is not None:
            return samples, nv_samples, True
        t0 = time.monotonic()
        busy = sample_gpu_engine_busy(pid)
        if busy is not None:
            if prev_busy is not None and prev_t is not None:
                dt = t0 - prev_t
                if dt > 0:
                    samples.append((busy - prev_busy) / dt * 100.0)
            prev_busy = busy
            prev_t = t0
        nv = sample_nvidia()
        if nv is not None:
            nv_samples.append(nv)
        sleep = interval_s - (time.monotonic() - t0)
        if sleep > 0:
            time.sleep(sleep)
    return samples, nv_samples, False


def stop_game(proc, graceful):
    if proc.poll() is not None:
        return
    if graceful and os.name == "nt":
        run_powershell("(Get-Process -Id {p} -ErrorAction SilentlyContinue).CloseMainWindow()".format(p=proc.pid))
        try:
            proc.wait(timeout=10)
            return
        except subprocess.TimeoutExpired:
            pass
    proc.kill()
    try:
        proc.wait(timeout=10)
    except subprocess.TimeoutExpired:
        pass


def agg(values):
    if not values:
        return {"mean": None, "std": None, "min": None, "max": None, "p95": None, "median": None, "samples": []}
    mean = sum(values) / len(values)
    var = sum((v - mean) ** 2 for v in values) / len(values)
    ordered = sorted(values)
    p95 = ordered[max(0, int(math.ceil(0.95 * len(ordered))) - 1)]
    mid = len(ordered) // 2
    median = (ordered[mid] if len(ordered) % 2 else 0.5 * (ordered[mid - 1] + ordered[mid]))
    return {
        "mean": round(mean, 3),
        "std": round(math.sqrt(var), 3),
        "min": round(min(values), 3),
        "max": round(max(values), 3),
        "p95": round(p95, 3),
        "median": round(median, 3),
        "samples": [round(v, 3) for v in values],
    }


def run_once(args, run_index, log_dir):
    os.makedirs(log_dir, exist_ok=True)
    log_path = os.path.join(log_dir, "perf_run_%02d.log" % run_index)
    log_file = open(log_path, "w")

    game_args = [args.exe, "-w", str(args.width), "-h", str(args.height)]
    game_args.append("-F" if args.fullscreen else "-W")
    game_args += ["-l", "stdout", "-r", args.registry]
    if args.tps is not None:
        game_args += ["-t", str(args.tps)]

    proc = subprocess.Popen(
        game_args,
        cwd=args.cwd,
        stdout=log_file,
        stderr=subprocess.STDOUT,
        creationflags=subprocess.CREATE_NO_WINDOW if os.name == "nt" else 0,
    )
    pid = proc.pid

    try:
        if not wait_for_gpu_engine(pid, args.startup_timeout):
            tail = log_tail(log_path, 20)
            raise RuntimeError("game started but never submitted work to the GPU (pid %d)\n%s" % (pid, tail))
        time.sleep(args.warmup)

        left = args.duration
        samples, nv_samples, died = collect_samples(proc, pid, left, args.sample_interval)
        if died:
            raise RuntimeError("game exited during measurement (pid %d)\n%s" % (pid, log_tail(log_path, 20)))
        if len(samples) < 2:
            raise RuntimeError("too few GPU samples collected (got %d)" % len(samples))

        return {
            "run": run_index,
            "pid": pid,
            "sample_count": len(samples),
            "gpu_3d_util_pct": agg(samples),
            "gpu_3d_util_p95": None,
            "nv_util_pct": agg([s[0] for s in nv_samples]) if nv_samples else None,
            "nv_power_w": agg([s[1] for s in nv_samples if s[1] is not None]) if nv_samples else None,
            "log": log_path,
        }, log_path
    finally:
        stop_game(proc, args.graceful)
        log_file.close()


def log_tail(log_path, n):
    try:
        with open(log_path, "r", errors="replace") as f:
            lines = f.readlines()
        return "".join(lines[-n:])
    except OSError as e:
        return "unable to read log: %s" % e


def summarize_report(report, args):
    line = []
    gpu = report["results"][0]["gpu_3d_util_pct"]["samples"]
    all_util = [s for r in report["results"] if r["gpu_3d_util_pct"]["mean"] is not None for s in r["gpu_3d_util_pct"]["samples"]]
    if not all_util:
        line.append("gpu_3d_util: no samples")
    else:
        mean = sum(all_util) / len(all_util)
        est_ms = mean / 100.0 * (1000.0 / args.fps)
        line.append("gpu_3d_util: %.2f%% (est %.2f ms/frame @ %d fps)" % (mean, est_ms, args.fps))
    report["est_gpu_ms_per_frame"] = aggeq(est_ms if all_util else None, all_util)
    return " | ".join(line)


def aggeq(value, samples):
    return {"mean": round(value, 3) if value is not None else None, "samples": [round(s, 3) for s in samples]}


def main():
    ap = argparse.ArgumentParser(description="Measure blockengine client GPU load.")
    ap.add_argument("--exe", default=DEFAULT_EXE)
    ap.add_argument("--cwd", default=DEFAULT_CWD)
    ap.add_argument("--registry", default="engine")
    ap.add_argument("--width", type=int, default=800)
    ap.add_argument("--height", type=int, default=600)
    ap.add_argument("--fullscreen", action="store_true")
    ap.add_argument("--fps", type=int, default=60, help="assumed frame cap for ms/frame estimate")
    ap.add_argument("--tps", type=int, default=None)
    ap.add_argument("--duration", type=float, default=15.0, help="measurement window per run, seconds")
    ap.add_argument("--sample-interval", type=float, default=1.0, help="GPU sampling period, seconds")
    ap.add_argument("--warmup", type=float, default=3.0, help="settle time before sampling starts")
    ap.add_argument("--startup-timeout", type=float, default=30.0)
    ap.add_argument("--runs", type=int, default=2, help="how many game launches to average over")
    ap.add_argument("--graceful", action="store_true", help="close the window (SDL_QUIT) instead of killing the game")
    ap.add_argument("--no-build", action="store_true")
    ap.add_argument("--rebuild", action="store_true", help="always rebuild before measuring")
    ap.add_argument("--tag", default=None, help="arbitrary label attached to the report")
    ap.add_argument("--json", default=None, help="write a detailed JSON report to this path")
    ap.add_argument("--save-baseline", action="store_true", help="store this measurement as baseline.json")
    ap.add_argument(
        "--compare",
        metavar="BASELINE",
        nargs="?",
        const=BASELINE_PATH,
        help="compare against a baseline and exit nonzero on regression (--compare-pct)",
    )
    ap.add_argument("--compare-pct", type=float, default=20.0, help="regression threshold, percent above baseline")
    ap.add_argument("-v", "--verbose", action="store_true")
    args = ap.parse_args()

    if not os.path.isfile(args.exe) or args.rebuild:
        if args.no_build:
            print("error: %s not found and --no-build given" % args.exe)
            return 1
        if not build_client(args.exe, args.rebuild):
            print("error: failed to build the client")
            return 1

    os.makedirs(OUT_DIR, exist_ok=True)
    log_dir = os.path.join(OUT_DIR, time.strftime("run_%Y%m%d_%H%M%S"))
    report = {
        "engine": os.path.basename(args.exe),
        "registry": args.registry,
        "resolution": "%dx%d" % (args.width, args.height),
        "fullscreen": args.fullscreen,
        "measure_window_s": args.duration,
        "sample_interval_s": args.sample_interval,
        "runs": args.runs,
        "tag": args.tag,
        "nvidia_present": find_nvsmi() is not None,
        "results": [],
    }

    failed = False
    for i in range(args.runs):
        if args.verbose:
            print("[run %d/%d] launching %s ..." % (i + 1, args.runs, args.exe))
        try:
            result, _log = run_once(args, i, log_dir)
        except RuntimeError as e:
            print("run %d FAILED: %s" % (i + 1, e))
            failed = True
            break
        report["results"].append(result)
        r = result["gpu_3d_util_pct"]
        if r["mean"] is not None:
            print(
                "run %d: gpu_3d_util mean=%s%% p95=%s%% (n=%d)"
                % (i + 1, r["mean"], r["p95"], r["samples"] and len(r["samples"]) or 0)
            )
        if result["nv_util_pct"] and result["nv_util_pct"]["mean"] is not None:
            print("       nvidia_smi util=%s%% power=%sW" % (result["nv_util_pct"]["mean"], result["nv_power_w"]["mean"]))

    if failed:
        return 1

    mean_str = summarize_report(report, args)

    if args.json:
        with open(args.json, "w") as f:
            json.dump(report, f, indent=2)
        print("[report] written to %s" % args.json)

    if args.save_baseline:
        flat_util = [s for r in report["results"] if r["gpu_3d_util_pct"]["mean"] is not None for s in r["gpu_3d_util_pct"]["samples"]]
        flat_nv = [s for r in report["results"] if r["nv_util_pct"] for s in r["nv_util_pct"]["samples"]]
        baseline = {
            "resolution": report["resolution"],
            "registry": report["registry"],
            "fps_assumed": args.fps,
            "runs": len(report["results"]),
            "mean_gpu_3d_util_pct": round(sum(flat_util) / len(flat_util), 3) if flat_util else None,
            "mean_nv_util_pct": round(sum(flat_nv) / len(flat_nv), 3) if flat_nv else None,
            "est_gpu_ms_per_frame": report["est_gpu_ms_per_frame"]["mean"],
            "recorded": time.strftime("%Y-%m-%d_%H:%M:%S"),
        }
        with open(BASELINE_PATH, "w") as f:
            json.dump(baseline, f, indent=2)
        print("[baseline] saved to %s" % BASELINE_PATH)

    exit_code = 0
    if args.compare is not None:
        if not os.path.isfile(args.compare):
            print("error: baseline %s not found" % args.compare)
            return 1
        with open(args.compare) as f:
            base = json.load(f)
        cur = report["est_gpu_ms_per_frame"]["mean"]
        base_ms = base.get("est_gpu_ms_per_frame")
        if base_ms and cur is not None:
            ratio = cur / base_ms
            limit = 1.0 + args.compare_pct / 100.0
            status = "PASS" if ratio <= limit else "FAIL"
            if status == "FAIL":
                exit_code = 1
            print(
                "[compare] est_gpu_ms_per_frame %s: %.2f ms vs baseline %.2f ms (%.1f%%, limit +%.0f%%)"
                % (status, cur, base_ms, (ratio - 1.0) * 100.0, args.compare_pct)
            )
        else:
            print("[compare] baseline lacks est_gpu_ms_per_frame; skipping")

    print(mean_str)
    return exit_code


if __name__ == "__main__":
    sys.exit(main())