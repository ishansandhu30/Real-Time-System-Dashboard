#!/usr/bin/env python3
"""
Generate synthetic system metrics data for testing the Real-Time System
Dashboard without requiring a live system. Outputs CSV that matches the
dashboard's expected import format.

Usage:
    python data_generator.py [--output metrics.csv] [--duration 300] [--interval 1.0]
"""

import argparse
import csv
import math
import random
import sys
import time
from datetime import datetime, timedelta
from dataclasses import dataclass, field
from typing import List


@dataclass
class CpuSample:
    timestamp: str
    total_usage: float
    per_core: List[float] = field(default_factory=list)
    core_count: int = 4


@dataclass
class MemorySample:
    timestamp: str
    total_bytes: int = 16 * 1024**3  # 16 GB
    used_bytes: int = 0
    free_bytes: int = 0
    cached_bytes: int = 0
    swap_total_bytes: int = 4 * 1024**3
    swap_used_bytes: int = 0


@dataclass
class ProcessSample:
    timestamp: str
    pid: int = 0
    name: str = ""
    cpu_percent: float = 0.0
    memory_percent: float = 0.0
    memory_bytes: int = 0
    status: str = "Running"


PROCESS_NAMES = [
    "python3", "chrome", "firefox", "code", "node", "java",
    "postgres", "redis-server", "nginx", "docker", "systemd",
    "Xorg", "pulseaudio", "ssh-agent", "cron", "bash",
    "vim", "git", "make", "gcc", "cargo", "rustc",
    "webpack", "eslint", "prettier", "pytest", "gunicorn",
    "celery", "rabbitmq", "mongodb", "mysql", "sqlite3",
]


def generate_cpu_pattern(t: float, core_count: int) -> CpuSample:
    """Generate realistic CPU usage with diurnal patterns and random spikes."""
    # Base load with sinusoidal pattern (simulates workload cycles)
    base_load = 15.0 + 10.0 * math.sin(t * 0.02)

    # Occasional spikes
    spike = 0.0
    if random.random() < 0.05:
        spike = random.uniform(20.0, 60.0)

    # Per-core usage with some variance
    per_core = []
    for i in range(core_count):
        core_base = base_load + random.gauss(0, 5.0)
        core_spike = spike * random.uniform(0.5, 1.5) if spike > 0 else 0
        core_usage = max(0.0, min(100.0, core_base + core_spike + random.gauss(0, 3.0)))

        # Some cores tend to run hotter
        if i == 0:
            core_usage *= 1.2

        per_core.append(round(core_usage, 1))

    total = sum(per_core) / len(per_core) if per_core else 0.0

    return CpuSample(
        timestamp=datetime.now().isoformat(),
        total_usage=round(total, 1),
        per_core=per_core,
        core_count=core_count,
    )


def generate_memory_pattern(t: float, total_gb: float = 16.0) -> MemorySample:
    """Generate realistic memory usage with gradual growth and cache behavior."""
    total_bytes = int(total_gb * 1024**3)

    # Memory usage grows gradually with some noise
    base_used_pct = 40.0 + 15.0 * math.sin(t * 0.005) + random.gauss(0, 2.0)
    base_used_pct = max(20.0, min(85.0, base_used_pct))

    # Cache usage varies inversely with used memory
    cache_pct = max(5.0, min(30.0, 25.0 - base_used_pct * 0.2 + random.gauss(0, 2.0)))

    used_bytes = int(total_bytes * base_used_pct / 100.0)
    cached_bytes = int(total_bytes * cache_pct / 100.0)
    free_bytes = total_bytes - used_bytes - cached_bytes

    # Swap usage: minimal unless memory is high
    swap_total = int(4 * 1024**3)
    swap_used = 0
    if base_used_pct > 70:
        swap_pct = (base_used_pct - 70) * 2.0 + random.gauss(0, 1.0)
        swap_used = int(swap_total * max(0, swap_pct) / 100.0)

    return MemorySample(
        timestamp=datetime.now().isoformat(),
        total_bytes=total_bytes,
        used_bytes=used_bytes,
        free_bytes=max(0, free_bytes),
        cached_bytes=cached_bytes,
        swap_total_bytes=swap_total,
        swap_used_bytes=swap_used,
    )


def generate_processes(t: float, count: int = 30) -> List[ProcessSample]:
    """Generate a realistic process list snapshot."""
    processes = []
    for i in range(count):
        name = random.choice(PROCESS_NAMES)
        cpu = max(0.0, random.expovariate(0.3))
        mem_pct = max(0.1, random.expovariate(0.5))

        # Some processes are consistently heavy
        if name in ("chrome", "java", "code", "webpack"):
            cpu *= 2.0
            mem_pct *= 1.5

        status = random.choices(
            ["Running", "Sleeping", "Stopped", "Zombie"],
            weights=[30, 65, 3, 2],
        )[0]

        processes.append(ProcessSample(
            timestamp=datetime.now().isoformat(),
            pid=random.randint(1000, 65535),
            name=name,
            cpu_percent=round(min(100.0, cpu), 1),
            memory_percent=round(min(50.0, mem_pct), 1),
            memory_bytes=int(mem_pct * 16 * 1024**3 / 100.0),
            status=status,
        ))

    # Sort by CPU descending
    processes.sort(key=lambda p: p.cpu_percent, reverse=True)
    return processes


def write_csv(output_path: str, duration: int, interval: float,
              core_count: int, total_memory_gb: float, stream: bool = False):
    """Generate and write metrics CSV data."""
    steps = int(duration / interval)

    with open(output_path, "w", newline="") as f:
        writer = csv.writer(f)
        writer.writerow([
            "timestamp", "metric_type",
            "cpu_total", "cpu_core_0", "cpu_core_1", "cpu_core_2", "cpu_core_3",
            "mem_total", "mem_used", "mem_free", "mem_cached",
            "swap_total", "swap_used",
            "proc_pid", "proc_name", "proc_cpu", "proc_mem", "proc_status",
        ])

        for step in range(steps):
            t = step * interval
            ts = (datetime.now() + timedelta(seconds=t)).isoformat()

            # CPU metrics
            cpu = generate_cpu_pattern(t, core_count)
            cores = cpu.per_core + [0.0] * (4 - len(cpu.per_core))
            writer.writerow([
                ts, "cpu",
                cpu.total_usage, cores[0], cores[1], cores[2], cores[3],
                "", "", "", "", "", "",
                "", "", "", "", "",
            ])

            # Memory metrics
            mem = generate_memory_pattern(t, total_memory_gb)
            writer.writerow([
                ts, "memory",
                "", "", "", "", "",
                mem.total_bytes, mem.used_bytes, mem.free_bytes, mem.cached_bytes,
                mem.swap_total_bytes, mem.swap_used_bytes,
                "", "", "", "", "",
            ])

            # Process list (every 2 seconds)
            if step % max(1, int(2.0 / interval)) == 0:
                for proc in generate_processes(t, count=20):
                    writer.writerow([
                        ts, "process",
                        "", "", "", "", "",
                        "", "", "", "", "", "",
                        proc.pid, proc.name, proc.cpu_percent,
                        proc.memory_percent, proc.status,
                    ])

            if stream:
                sys.stdout.write(f"\rGenerated {step + 1}/{steps} samples...")
                sys.stdout.flush()

    if stream:
        print(f"\nDone. Wrote {output_path}")


def main():
    parser = argparse.ArgumentParser(
        description="Generate synthetic system metrics for dashboard testing"
    )
    parser.add_argument(
        "--output", "-o", default="synthetic_metrics.csv",
        help="Output CSV file path (default: synthetic_metrics.csv)"
    )
    parser.add_argument(
        "--duration", "-d", type=int, default=300,
        help="Duration in seconds of simulated data (default: 300)"
    )
    parser.add_argument(
        "--interval", "-i", type=float, default=1.0,
        help="Sampling interval in seconds (default: 1.0)"
    )
    parser.add_argument(
        "--cores", "-c", type=int, default=4,
        help="Number of CPU cores to simulate (default: 4)"
    )
    parser.add_argument(
        "--memory", "-m", type=float, default=16.0,
        help="Total memory in GB (default: 16.0)"
    )
    args = parser.parse_args()

    print(f"Generating {args.duration}s of synthetic metrics "
          f"({args.cores} cores, {args.memory}GB RAM)...")
    write_csv(
        args.output, args.duration, args.interval,
        args.cores, args.memory, stream=True
    )


if __name__ == "__main__":
    main()
