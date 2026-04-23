from __future__ import annotations

from pathlib import Path
import re

import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import pandas as pd

SCRIPT_DIR = Path(__file__).resolve().parent
ROOT_DIR = SCRIPT_DIR.parent
LOSS_DIR = ROOT_DIR / 'results' / 'loss_prob'
OUTPUT_DIR = ROOT_DIR / 'results'

PATTERN = re.compile(r'^results_loss_prob_(?P<value>.+)$')


def _load_series() -> tuple[list[float], list[float], list[float]]:
    loss_probs: list[float] = []
    loss_rates: list[float] = []
    latencies: list[float] = []

    for file_path in sorted(LOSS_DIR.glob('results_loss_prob_*.csv')):
        match = PATTERN.match(file_path.stem)
        if not match:
            continue

        df = pd.read_csv(file_path)
        if df.empty:
            continue

        loss_probs.append(float(match.group('value')))
        loss_rates.append(float(df['Loss Rate'].iloc[0]))
        latencies.append(float(df['Average Latency'].iloc[0]))

    if not loss_probs:
        raise FileNotFoundError(f'No CSV files found in {LOSS_DIR}')

    ordered = sorted(zip(loss_probs, loss_rates, latencies), key=lambda item: item[0])
    probs, rates, lats = zip(*ordered)
    return list(probs), list(rates), list(lats)


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
    loss_probs, loss_rates, latencies = _load_series()

    _save_plot(
        loss_probs,
        loss_rates,
        'Loss Probabilities x Loss Rates',
        'Loss Probabilities',
        'Loss Rates',
        OUTPUT_DIR / 'loss_rate_vs_loss_prob.png',
    )
    _save_plot(
        loss_probs,
        latencies,
        'Loss Probabilities x Latencies',
        'Loss Probabilities',
        'Latencies',
        OUTPUT_DIR / 'latency_vs_loss_prob.png',
    )


if __name__ == '__main__':
    main()
