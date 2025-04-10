#!/usr/bin/env python3
"""
Offline analysis of collected system metrics. Loads exported metrics CSV
and produces summary statistics, trend analysis, and plots.

Usage:
    python analyze_metrics.py <metrics.csv> [--output-dir ./analysis]

Dependencies:
    pip install pandas matplotlib numpy
"""

import argparse
import os
import sys
from pathlib import Path

try:
    import pandas as pd
    import numpy as np
    import matplotlib
    matplotlib.use("Agg")  # Non-interactive backend for headless environments
    import matplotlib.pyplot as plt
    import matplotlib.dates as mdates
except ImportError as e:
    print(f"Missing dependency: {e}")
    print("Install required packages: pip install pandas matplotlib numpy")
    sys.exit(1)


def load_metrics(csv_path: str) -> pd.DataFrame:
    """Load metrics CSV and parse timestamps."""
    df = pd.read_csv(csv_path)
    if "timestamp" in df.columns:
        df["timestamp"] = pd.to_datetime(df["timestamp"])
    return df


def analyze_cpu(df: pd.DataFrame, output_dir: str):
    """Analyze and plot CPU metrics."""
    cpu_df = df[df["metric_type"] == "cpu"].copy()
    if cpu_df.empty:
        print("  No CPU metrics found.")
        return

    cpu_df = cpu_df.sort_values("timestamp")

    # Summary statistics
    print("\n--- CPU Summary ---")
    total = cpu_df["cpu_total"].astype(float)
    print(f"  Samples:       {len(total)}")
    print(f"  Mean Usage:    {total.mean():.1f}%")
    print(f"  Median Usage:  {total.median():.1f}%")
    print(f"  Std Dev:       {total.std():.1f}%")
    print(f"  Min Usage:     {total.min():.1f}%")
    print(f"  Max Usage:     {total.max():.1f}%")
    print(f"  95th Pctile:   {total.quantile(0.95):.1f}%")

    # Per-core statistics
    core_cols = [c for c in cpu_df.columns if c.startswith("cpu_core_")]
    if core_cols:
        print("\n  Per-Core Averages:")
        for col in core_cols:
            vals = cpu_df[col].astype(float)
            core_name = col.replace("cpu_", "").replace("_", " ").title()
            print(f"    {core_name}: {vals.mean():.1f}% "
                  f"(min={vals.min():.1f}%, max={vals.max():.1f}%)")

    # Plot CPU usage over time
    fig, axes = plt.subplots(2, 1, figsize=(12, 8), sharex=True)
    fig.suptitle("CPU Usage Analysis", fontsize=14, fontweight="bold")

    # Total CPU line chart
    ax1 = axes[0]
    ax1.plot(cpu_df["timestamp"], total, color="#89b4fa", linewidth=1.2,
             label="Total CPU", alpha=0.9)
    ax1.fill_between(cpu_df["timestamp"], 0, total, color="#89b4fa", alpha=0.15)
    ax1.axhline(y=total.mean(), color="#a6e3a1", linestyle="--",
                linewidth=1, label=f"Mean ({total.mean():.1f}%)")
    ax1.set_ylabel("CPU Usage (%)")
    ax1.set_ylim(0, 105)
    ax1.legend(loc="upper right", fontsize=9)
    ax1.grid(True, alpha=0.3)
    ax1.set_facecolor("#1e1e2e")

    # Per-core stacked area
    ax2 = axes[1]
    if core_cols:
        core_data = cpu_df[core_cols].astype(float)
        colors = ["#a6e3a1", "#f9e2af", "#f38ba8", "#cba6f7",
                  "#89dceb", "#fab387", "#94e2d5", "#f5c2e7"]
        for i, col in enumerate(core_cols):
            label = col.replace("cpu_", "").replace("_", " ").title()
            ax2.plot(cpu_df["timestamp"], core_data[col],
                     color=colors[i % len(colors)],
                     linewidth=1, label=label, alpha=0.8)
        ax2.set_ylabel("Core Usage (%)")
        ax2.set_ylim(0, 105)
        ax2.legend(loc="upper right", fontsize=8, ncol=2)
    ax2.set_xlabel("Time")
    ax2.grid(True, alpha=0.3)
    ax2.set_facecolor("#1e1e2e")

    ax2.xaxis.set_major_formatter(mdates.DateFormatter("%H:%M:%S"))
    fig.autofmt_xdate()

    plt.tight_layout()
    output_path = os.path.join(output_dir, "cpu_analysis.png")
    plt.savefig(output_path, dpi=150, facecolor="#1e1e2e", edgecolor="none")
    plt.close()
    print(f"\n  Saved: {output_path}")


def analyze_memory(df: pd.DataFrame, output_dir: str):
    """Analyze and plot memory metrics."""
    mem_df = df[df["metric_type"] == "memory"].copy()
    if mem_df.empty:
        print("  No memory metrics found.")
        return

    mem_df = mem_df.sort_values("timestamp")

    # Convert to GB for readability
    gb = 1024**3
    for col in ["mem_total", "mem_used", "mem_free", "mem_cached",
                "swap_total", "swap_used"]:
        if col in mem_df.columns:
            mem_df[col] = mem_df[col].astype(float) / gb

    print("\n--- Memory Summary ---")
    total_gb = mem_df["mem_total"].iloc[0] if "mem_total" in mem_df else 0
    print(f"  Total RAM:     {total_gb:.1f} GB")

    if "mem_used" in mem_df:
        used = mem_df["mem_used"]
        print(f"  Used (mean):   {used.mean():.2f} GB")
        print(f"  Used (max):    {used.max():.2f} GB")
        print(f"  Used (min):    {used.min():.2f} GB")

    if "mem_cached" in mem_df:
        cached = mem_df["mem_cached"]
        print(f"  Cached (mean): {cached.mean():.2f} GB")

    if "swap_used" in mem_df:
        swap = mem_df["swap_used"]
        print(f"  Swap (mean):   {swap.mean():.2f} GB")
        print(f"  Swap (max):    {swap.max():.2f} GB")

    # Plot memory usage
    fig, axes = plt.subplots(2, 1, figsize=(12, 8), sharex=True)
    fig.suptitle("Memory Usage Analysis", fontsize=14, fontweight="bold")

    ax1 = axes[0]
    if "mem_used" in mem_df:
        ax1.fill_between(mem_df["timestamp"], 0, mem_df["mem_used"],
                         color="#a6e3a1", alpha=0.5, label="Used")
    if "mem_cached" in mem_df:
        ax1.fill_between(mem_df["timestamp"],
                         mem_df["mem_used"],
                         mem_df["mem_used"] + mem_df["mem_cached"],
                         color="#f9e2af", alpha=0.5, label="Cached")
    if "mem_free" in mem_df:
        ax1.fill_between(mem_df["timestamp"],
                         mem_df["mem_used"] + mem_df["mem_cached"],
                         total_gb,
                         color="#6c7086", alpha=0.3, label="Free")
    ax1.set_ylabel("Memory (GB)")
    ax1.set_ylim(0, total_gb * 1.05)
    ax1.legend(loc="upper right", fontsize=9)
    ax1.grid(True, alpha=0.3)
    ax1.set_facecolor("#1e1e2e")

    # Swap usage
    ax2 = axes[1]
    if "swap_used" in mem_df:
        ax2.plot(mem_df["timestamp"], mem_df["swap_used"],
                 color="#f38ba8", linewidth=1.5, label="Swap Used")
        ax2.fill_between(mem_df["timestamp"], 0, mem_df["swap_used"],
                         color="#f38ba8", alpha=0.2)
    ax2.set_ylabel("Swap (GB)")
    ax2.set_xlabel("Time")
    ax2.legend(loc="upper right", fontsize=9)
    ax2.grid(True, alpha=0.3)
    ax2.set_facecolor("#1e1e2e")

    ax2.xaxis.set_major_formatter(mdates.DateFormatter("%H:%M:%S"))
    fig.autofmt_xdate()

    plt.tight_layout()
    output_path = os.path.join(output_dir, "memory_analysis.png")
    plt.savefig(output_path, dpi=150, facecolor="#1e1e2e", edgecolor="none")
    plt.close()
    print(f"  Saved: {output_path}")


def analyze_processes(df: pd.DataFrame, output_dir: str):
    """Analyze process data."""
    proc_df = df[df["metric_type"] == "process"].copy()
    if proc_df.empty:
        print("  No process metrics found.")
        return

    print("\n--- Process Summary ---")
    print(f"  Total records: {len(proc_df)}")

    if "proc_name" in proc_df:
        unique_names = proc_df["proc_name"].nunique()
        print(f"  Unique processes: {unique_names}")

        # Top CPU consumers
        print("\n  Top CPU Consumers (average %):")
        top_cpu = (proc_df.groupby("proc_name")["proc_cpu"]
                   .agg(["mean", "max", "count"])
                   .sort_values("mean", ascending=False)
                   .head(10))
        for name, row in top_cpu.iterrows():
            print(f"    {name:20s}  avg={row['mean']:.1f}%  "
                  f"max={row['max']:.1f}%  samples={int(row['count'])}")

        # Top Memory consumers
        print("\n  Top Memory Consumers (average %):")
        top_mem = (proc_df.groupby("proc_name")["proc_mem"]
                   .agg(["mean", "max", "count"])
                   .sort_values("mean", ascending=False)
                   .head(10))
        for name, row in top_mem.iterrows():
            print(f"    {name:20s}  avg={row['mean']:.1f}%  "
                  f"max={row['max']:.1f}%  samples={int(row['count'])}")

    # Bar chart of top processes
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 6))
    fig.suptitle("Top Processes", fontsize=14, fontweight="bold")

    if "proc_name" in proc_df:
        # Top 10 by CPU
        top_cpu_plot = (proc_df.groupby("proc_name")["proc_cpu"]
                        .mean().sort_values(ascending=True).tail(10))
        colors_cpu = plt.cm.Blues(np.linspace(0.3, 0.9, len(top_cpu_plot)))
        ax1.barh(top_cpu_plot.index, top_cpu_plot.values, color=colors_cpu)
        ax1.set_xlabel("Average CPU %")
        ax1.set_title("Top 10 by CPU Usage")
        ax1.set_facecolor("#1e1e2e")

        # Top 10 by Memory
        top_mem_plot = (proc_df.groupby("proc_name")["proc_mem"]
                        .mean().sort_values(ascending=True).tail(10))
        colors_mem = plt.cm.Greens(np.linspace(0.3, 0.9, len(top_mem_plot)))
        ax2.barh(top_mem_plot.index, top_mem_plot.values, color=colors_mem)
        ax2.set_xlabel("Average Memory %")
        ax2.set_title("Top 10 by Memory Usage")
        ax2.set_facecolor("#1e1e2e")

    plt.tight_layout()
    output_path = os.path.join(output_dir, "process_analysis.png")
    plt.savefig(output_path, dpi=150, facecolor="#1e1e2e", edgecolor="none")
    plt.close()
    print(f"  Saved: {output_path}")


def generate_report(df: pd.DataFrame, output_dir: str):
    """Generate a text summary report."""
    report_path = os.path.join(output_dir, "summary_report.txt")

    with open(report_path, "w") as f:
        f.write("=" * 60 + "\n")
        f.write("  System Metrics Analysis Report\n")
        f.write("=" * 60 + "\n\n")

        if "timestamp" in df.columns:
            ts = df["timestamp"]
            f.write(f"Time Range: {ts.min()} to {ts.max()}\n")
            duration = (ts.max() - ts.min()).total_seconds()
            f.write(f"Duration:   {duration:.0f} seconds\n")

        f.write(f"Total Records: {len(df)}\n\n")

        # CPU stats
        cpu_df = df[df["metric_type"] == "cpu"]
        if not cpu_df.empty:
            total = cpu_df["cpu_total"].astype(float)
            f.write("CPU Statistics\n")
            f.write("-" * 40 + "\n")
            f.write(f"  Mean:   {total.mean():.1f}%\n")
            f.write(f"  Median: {total.median():.1f}%\n")
            f.write(f"  StdDev: {total.std():.1f}%\n")
            f.write(f"  Min:    {total.min():.1f}%\n")
            f.write(f"  Max:    {total.max():.1f}%\n")
            f.write(f"  P95:    {total.quantile(0.95):.1f}%\n\n")

        # Memory stats
        mem_df = df[df["metric_type"] == "memory"]
        if not mem_df.empty:
            gb = 1024**3
            used = mem_df["mem_used"].astype(float) / gb
            f.write("Memory Statistics\n")
            f.write("-" * 40 + "\n")
            f.write(f"  Total:     {mem_df['mem_total'].astype(float).iloc[0] / gb:.1f} GB\n")
            f.write(f"  Used Mean: {used.mean():.2f} GB\n")
            f.write(f"  Used Max:  {used.max():.2f} GB\n\n")

    print(f"\n  Report saved: {report_path}")


def main():
    parser = argparse.ArgumentParser(
        description="Analyze exported system metrics from the dashboard"
    )
    parser.add_argument(
        "csv_file",
        help="Path to the metrics CSV file"
    )
    parser.add_argument(
        "--output-dir", "-o", default="./analysis",
        help="Directory for output plots and reports (default: ./analysis)"
    )
    args = parser.parse_args()

    if not os.path.exists(args.csv_file):
        print(f"Error: File not found: {args.csv_file}")
        sys.exit(1)

    os.makedirs(args.output_dir, exist_ok=True)

    print(f"Loading metrics from: {args.csv_file}")
    df = load_metrics(args.csv_file)
    print(f"Loaded {len(df)} records")

    analyze_cpu(df, args.output_dir)
    analyze_memory(df, args.output_dir)
    analyze_processes(df, args.output_dir)
    generate_report(df, args.output_dir)

    print(f"\nAnalysis complete. Results in: {args.output_dir}/")


if __name__ == "__main__":
    main()
