from __future__ import annotations

from pathlib import Path
import re

import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import pandas as pd

SCRIPT_DIR = Path(__file__).resolve().parent
ROOT_DIR = SCRIPT_DIR.parent
DATA_DIR = ROOT_DIR / 'results' / 'packet_size'
OUTPUT_DIR = ROOT_DIR / 'results'

PATTERN = re.compile(r'^results_packet_size_(?P<value>.+)$')


def _load_series() -> tuple[list[float], list[float]]:
    packet_sizes: list[float] = []
    latencies: list[float] = []

    for file_path in sorted(DATA_DIR.glob('results_packet_size_*.csv')):
        match = PATTERN.match(file_path.stem)
        if not match:
            continue

        df = pd.read_csv(file_path)
        if df.empty:
            continue

        packet_sizes.append(float(match.group('value')))
        latencies.append(float(df['Average Latency'].iloc[0]))

    if not packet_sizes:
        raise FileNotFoundError(f'No CSV files found in {DATA_DIR}')

    ordered = sorted(zip(packet_sizes, latencies), key=lambda item: item[0])
    sizes, lats = zip(*ordered)
    return list(sizes), list(lats)


def _save_plot(x: list[float], y: list[float], title: str, xlabel: str, ylabel: str, output: Path) -> None:
    plt.figure()
    plt.plot(x, y)
    plt.title(title)
    plt.xlabel(xlabel)
    plt.ylabel(ylabel)
    output.parent.mkdir(parents=True, exist_ok=True)
    plt.savefig(output, bbox_inches='tight')
    plt.close()


def main() -> None:
    packet_sizes, latencies = _load_series()
    _save_plot(
        packet_sizes,
        latencies,
        'Packet Sizes x Latencies',
        'Packet Sizes',
        'Latencies',
        OUTPUT_DIR / 'latency_vs_packet_size.png',
    )


if __name__ == '__main__':
    main()
