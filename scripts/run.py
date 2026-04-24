#!/usr/bin/env python3
"""
KNS Runner v0.9
---------------
- Gera resultados em ../results/v0.9/test_#N/
- Funciona em Windows, Linux e macOS (cross-platform)
- Gera gráficos e relatórios de latência, tempo e outros dados do sistema
"""

from __future__ import annotations

import argparse
import csv
import json
import os
import platform
import re
import subprocess
import sys
import time
from collections import defaultdict
from datetime import datetime
from pathlib import Path

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import matplotlib.gridspec as gridspec
import numpy as np


# ──────────────────────────────────────────────────────────────
# Utilitários de plataforma
# ──────────────────────────────────────────────────────────────

def is_windows() -> bool:
    return platform.system().lower().startswith("win")

def is_linux() -> bool:
    return platform.system().lower() == "linux"

def has_display() -> bool:
    """Verifica se há display disponível (relevante para Linux headless)."""
    if is_windows():
        return True
    return bool(os.environ.get("DISPLAY") or os.environ.get("WAYLAND_DISPLAY"))

def xvfb_available() -> bool:
    """Verifica se xvfb-run está disponível (Linux)."""
    try:
        subprocess.run(["xvfb-run", "--help"], capture_output=True, timeout=3)
        return True
    except (FileNotFoundError, subprocess.TimeoutExpired):
        return False


# ──────────────────────────────────────────────────────────────
# Busca do executável
# ──────────────────────────────────────────────────────────────

def find_executable(root: Path) -> Path | None:
    """Localiza kns_app(.exe) na árvore de diretórios."""
    candidates = ["kns_app.exe", "kns_app"]
    for name in candidates:
        for p in root.rglob(name):
            if p.is_file() and (is_windows() or os.access(p, os.X_OK)):
                return p.resolve()
    return None


# ──────────────────────────────────────────────────────────────
# Gerenciamento de diretório de resultados
# ──────────────────────────────────────────────────────────────

def get_test_dir(base: Path) -> Path:
    """Cria e retorna um diretório test_#N numerado automaticamente."""
    base.mkdir(parents=True, exist_ok=True)
    i = 1
    while True:
        d = base / f"test_#{i}"
        if not d.exists():
            d.mkdir(parents=True)
            return d
        i += 1


# ──────────────────────────────────────────────────────────────
# Execução silenciosa do simulador
# ──────────────────────────────────────────────────────────────

def build_command(exe: Path, topo: Path, is_linux_headless: bool) -> list[str]:
    """Monta o comando de execução, usando xvfb-run se necessário."""
    cmd = [str(exe), str(topo)]
    if is_linux_headless and xvfb_available():
        # xvfb-run cria um framebuffer virtual — a janela ImGui abre
        # mas não é exibida em lugar algum (totalmente invisível)
        cmd = ["xvfb-run", "--auto-servernum", "--"] + cmd
    return cmd

def run_silent(exe: Path, topo: Path, log_file: Path,
               timeout: float | None = None) -> tuple[subprocess.Popen, float]:
    """
    Inicia o simulador em segundo plano, sem janela visível.
    Retorna (processo, timestamp_início).
    """
    needs_vfb = is_linux() and not has_display()
    cmd = build_command(exe, topo, needs_vfb)

    kwargs: dict = dict(
        stdout=open(log_file, "w", encoding="utf-8"),
        stderr=subprocess.STDOUT,
        stdin=subprocess.DEVNULL,
        cwd=str(topo.parent),
    )

    if is_windows():
        # Sem janela, processo independente
        kwargs["creationflags"] = (
            subprocess.CREATE_NO_WINDOW |
            subprocess.DETACHED_PROCESS
        )
    else:
        # Novo grupo de processo (não recebe sinais do terminal pai)
        kwargs["start_new_session"] = True

    proc = subprocess.Popen(cmd, **kwargs)
    return proc, time.perf_counter()


# ──────────────────────────────────────────────────────────────
# Parsing do log de saída do simulador
# ──────────────────────────────────────────────────────────────

# Padrões reconhecidos no log
_RE_DROPPED   = re.compile(
    r"\[DROPPED\]\s+Packet\s+from\s+(\d+)\s+to\s+(\d+)\s+at\s+time\s+([\d.]+)"
)
_RE_DELIVERED = re.compile(
    r"\[DELIVERED\]\s+Packet\s+from\s+(\d+)\s+to\s+(\d+)\s+.*?latency[=:\s]+([\d.]+)",
    re.IGNORECASE
)
_RE_LATENCY   = re.compile(
    r"\[LATENCY\]\s+([\d.]+)",
    re.IGNORECASE
)

def parse_log(log_file: Path) -> dict:
    """
    Extrai eventos do log e retorna dicionário com métricas:
      - drops:      lista de (src, dst, time)
      - deliveries: lista de (src, dst, latency)
      - latencies:  lista de valores de latência
    """
    drops:      list[tuple[int, int, float]] = []
    deliveries: list[tuple[int, int, float]] = []
    latencies:  list[float] = []

    if not log_file.exists():
        return {"drops": drops, "deliveries": deliveries, "latencies": latencies}

    with open(log_file, encoding="utf-8", errors="replace") as f:
        for line in f:
            m = _RE_DROPPED.search(line)
            if m:
                drops.append((int(m.group(1)), int(m.group(2)), float(m.group(3))))
                continue
            m = _RE_DELIVERED.search(line)
            if m:
                deliveries.append((int(m.group(1)), int(m.group(2)), float(m.group(3))))
                latencies.append(float(m.group(3)))
                continue
            m = _RE_LATENCY.search(line)
            if m:
                latencies.append(float(m.group(1)))

    return {"drops": drops, "deliveries": deliveries, "latencies": latencies}


def parse_csv_if_exists(csv_file: Path) -> dict | None:
    """Tenta ler CSV gerado pelo simulador (se houver)."""
    if not csv_file or not csv_file.exists():
        return None
    rows = []
    with open(csv_file, encoding="utf-8", errors="replace", newline="") as f:
        reader = csv.DictReader(f)
        for row in reader:
            rows.append({k: v.strip() for k, v in row.items()})
    return rows if rows else None


# ──────────────────────────────────────────────────────────────
# Geração de gráficos
# ──────────────────────────────────────────────────────────────

COLORS = plt.rcParams["axes.prop_cycle"].by_key()["color"]

def _label(topo: Path) -> str:
    return topo.stem


def plot_drops_over_time(runs: list[dict], out_dir: Path) -> Path | None:
    """Pacotes descartados por janela de tempo (taxa de descarte)."""
    fig, ax = plt.subplots(figsize=(10, 5))
    any_data = False

    for i, run in enumerate(runs):
        events = run["events"]
        drop_times = sorted(t for _, _, t in events["drops"])
        if not drop_times:
            continue

        # histograma acumulativo → taxa por janela
        t_max = max(drop_times)
        bins = np.linspace(0, t_max, min(50, len(drop_times) // 2 + 2))
        counts, edges = np.histogram(drop_times, bins=bins)
        centers = (edges[:-1] + edges[1:]) / 2
        window = edges[1] - edges[0] if len(edges) > 1 else 1

        label = _label(run["topo"])
        ax.plot(centers, counts / window, label=label,
                color=COLORS[i % len(COLORS)])
        any_data = True

    if not any_data:
        plt.close(fig)
        return None

    ax.set_xlabel("Tempo de simulação (s)")
    ax.set_ylabel("Pacotes descartados / s")
    ax.set_title("Taxa de descarte ao longo do tempo")
    ax.legend()
    ax.grid(alpha=0.3)
    fig.tight_layout()

    out = out_dir / "drops_over_time.png"
    fig.savefig(out, dpi=120)
    plt.close(fig)
    return out


def plot_latency_distribution(runs: list[dict], out_dir: Path) -> Path | None:
    """Histograma de latências (quando disponíveis)."""
    fig, ax = plt.subplots(figsize=(10, 5))
    any_data = False

    for i, run in enumerate(runs):
        lats = run["events"]["latencies"]
        if not lats:
            continue
        ax.hist(lats, bins=30, alpha=0.65,
                label=_label(run["topo"]),
                color=COLORS[i % len(COLORS)], edgecolor="white")
        any_data = True

    if not any_data:
        plt.close(fig)
        return None

    ax.set_xlabel("Latência (s)")
    ax.set_ylabel("Frequência")
    ax.set_title("Distribuição de latências")
    ax.legend()
    ax.grid(alpha=0.3)
    fig.tight_layout()

    out = out_dir / "latency_distribution.png"
    fig.savefig(out, dpi=120)
    plt.close(fig)
    return out


def plot_drop_heatmap(runs: list[dict], out_dir: Path) -> list[Path]:
    """Mapa de calor de descartes por par (src, dst) — um por topologia."""
    outs = []
    for run in runs:
        drops = run["events"]["drops"]
        if not drops:
            continue

        # matriz src×dst
        nodes = sorted({n for d in drops for n in d[:2]})
        n = len(nodes)
        idx = {v: i for i, v in enumerate(nodes)}
        mat = np.zeros((n, n), dtype=int)
        for src, dst, _ in drops:
            if src in idx and dst in idx:
                mat[idx[src], idx[dst]] += 1

        fig, ax = plt.subplots(figsize=(6, 5))
        im = ax.imshow(mat, cmap="Reds", aspect="auto")
        ax.set_xticks(range(n)); ax.set_xticklabels([f"N{v}" for v in nodes])
        ax.set_yticks(range(n)); ax.set_yticklabels([f"N{v}" for v in nodes])
        ax.set_xlabel("Destino"); ax.set_ylabel("Origem")
        ax.set_title(f"Descartes por par — {_label(run['topo'])}")
        plt.colorbar(im, ax=ax, label="Pacotes descartados")

        # anotações numéricas
        for r in range(n):
            for c in range(n):
                if mat[r, c]:
                    ax.text(c, r, str(mat[r, c]),
                            ha="center", va="center",
                            fontsize=8,
                            color="white" if mat[r, c] > mat.max() * 0.6 else "black")

        fig.tight_layout()
        out = out_dir / f"drop_heatmap_{_label(run['topo'])}.png"
        fig.savefig(out, dpi=120)
        plt.close(fig)
        outs.append(out)

    return outs


def plot_latency_over_time(runs: list[dict], out_dir: Path) -> Path | None:
    """Latência entregue ao longo do tempo (quando disponível)."""
    fig, ax = plt.subplots(figsize=(10, 5))
    any_data = False

    for i, run in enumerate(runs):
        deliveries = run["events"]["deliveries"]
        if not deliveries:
            continue
        times = [t for _, _, t in deliveries]
        lats  = [l for _, _, l in deliveries]   # noqa: E741
        ax.scatter(times, lats, s=8, alpha=0.5,
                   label=_label(run["topo"]),
                   color=COLORS[i % len(COLORS)])
        any_data = True

    if not any_data:
        plt.close(fig)
        return None

    ax.set_xlabel("Tempo de chegada (s)")
    ax.set_ylabel("Latência (s)")
    ax.set_title("Latência por pacote entregue ao longo do tempo")
    ax.legend()
    ax.grid(alpha=0.3)
    fig.tight_layout()

    out = out_dir / "latency_over_time.png"
    fig.savefig(out, dpi=120)
    plt.close(fig)
    return out


def plot_summary_dashboard(runs: list[dict], out_dir: Path) -> Path:
    """
    Painel consolidado com:
      - Taxa de descarte por topologia (barras)
      - Duração de cada execução (barras)
      - Latência média (se disponível)
    """
    labels     = [_label(r["topo"]) for r in runs]
    n          = len(labels)
    drop_rates = []
    durations  = []
    mean_lats  = []

    for r in runs:
        drops      = len(r["events"]["drops"])
        deliveries = len(r["events"]["deliveries"])
        total      = drops + deliveries
        drop_rates.append(drops / total * 100 if total > 0 else 0.0)
        durations.append(r["duration_s"])
        lats = r["events"]["latencies"]
        mean_lats.append(float(np.mean(lats)) if lats else float("nan"))

    rows = 2 if any(not np.isnan(v) for v in mean_lats) else 2
    fig = plt.figure(figsize=(12, 8))
    gs  = gridspec.GridSpec(2, 2, figure=fig, hspace=0.45, wspace=0.35)

    x = np.arange(n)
    width = 0.5

    # 1. Taxa de descarte
    ax1 = fig.add_subplot(gs[0, 0])
    bars = ax1.bar(x, drop_rates, width, color=COLORS[:n])
    ax1.set_xticks(x); ax1.set_xticklabels(labels, rotation=20, ha="right")
    ax1.set_ylabel("Taxa de descarte (%)")
    ax1.set_title("Taxa de descarte por topologia")
    ax1.grid(axis="y", alpha=0.3)
    for bar, val in zip(bars, drop_rates):
        ax1.text(bar.get_x() + bar.get_width() / 2, bar.get_height() + 0.3,
                 f"{val:.1f}%", ha="center", va="bottom", fontsize=9)

    # 2. Duração da execução
    ax2 = fig.add_subplot(gs[0, 1])
    bars2 = ax2.bar(x, durations, width, color=COLORS[:n])
    ax2.set_xticks(x); ax2.set_xticklabels(labels, rotation=20, ha="right")
    ax2.set_ylabel("Duração (s)")
    ax2.set_title("Tempo de execução por topologia")
    ax2.grid(axis="y", alpha=0.3)
    for bar, val in zip(bars2, durations):
        ax2.text(bar.get_x() + bar.get_width() / 2, bar.get_height() + 0.1,
                 f"{val:.2f}s", ha="center", va="bottom", fontsize=9)

    # 3. Latência média (se disponível)
    ax3 = fig.add_subplot(gs[1, 0])
    valid = [(l, c) for l, c in zip(mean_lats, COLORS[:n]) if not np.isnan(l)]
    if valid:
        vals, clrs = zip(*valid)
        valid_labels = [labels[i] for i, v in enumerate(mean_lats) if not np.isnan(v)]
        bars3 = ax3.bar(np.arange(len(vals)), vals, width, color=clrs)
        ax3.set_xticks(np.arange(len(vals)))
        ax3.set_xticklabels(valid_labels, rotation=20, ha="right")
        ax3.set_ylabel("Latência média (s)")
        ax3.set_title("Latência média por topologia")
        ax3.grid(axis="y", alpha=0.3)
    else:
        ax3.text(0.5, 0.5, "Sem dados de latência", ha="center", va="center",
                 transform=ax3.transAxes, color="gray")
        ax3.set_title("Latência média por topologia")

    # 4. Eventos totais (pizza)
    ax4 = fig.add_subplot(gs[1, 1])
    total_drops = sum(len(r["events"]["drops"]) for r in runs)
    total_deliv = sum(len(r["events"]["deliveries"]) for r in runs)
    if total_drops + total_deliv > 0:
        ax4.pie(
            [total_drops, total_deliv],
            labels=["Descartados", "Entregues"],
            colors=["#e74c3c", "#2ecc71"],
            autopct="%1.1f%%",
            startangle=90,
        )
        ax4.set_title("Distribuição global de pacotes")
    else:
        ax4.text(0.5, 0.5, "Sem dados de eventos", ha="center", va="center",
                 transform=ax4.transAxes, color="gray")
        ax4.set_title("Distribuição global de pacotes")

    fig.suptitle("Dashboard — KNS v0.9", fontsize=14, y=0.98)

    out = out_dir / "dashboard.png"
    fig.savefig(out, dpi=130)
    plt.close(fig)
    return out


# ──────────────────────────────────────────────────────────────
# Relatórios
# ──────────────────────────────────────────────────────────────

def compute_stats(events: dict, duration_s: float) -> dict:
    drops      = events["drops"]
    deliveries = events["deliveries"]
    lats       = events["latencies"]
    total      = len(drops) + len(deliveries)

    result = {
        "packets_total":    total,
        "packets_dropped":  len(drops),
        "packets_delivered": len(deliveries),
        "drop_rate":        len(drops) / total if total else None,
        "delivery_rate":    len(deliveries) / total if total else None,
        "duration_seconds": duration_s,
        "throughput_pps":   len(deliveries) / duration_s if duration_s > 0 else None,
    }

    if lats:
        arr = np.array(lats)
        result.update({
            "latency_mean_s":    float(np.mean(arr)),
            "latency_median_s":  float(np.median(arr)),
            "latency_min_s":     float(np.min(arr)),
            "latency_max_s":     float(np.max(arr)),
            "latency_p95_s":     float(np.percentile(arr, 95)),
            "latency_std_s":     float(np.std(arr)),
        })
    else:
        result.update({
            "latency_mean_s":   None,
            "latency_median_s": None,
            "latency_min_s":    None,
            "latency_max_s":    None,
            "latency_p95_s":    None,
            "latency_std_s":    None,
        })

    return result


def write_summary_json(runs: list[dict], test_dir: Path,
                       run_config: dict, graphs: list[str]) -> Path:
    run_results = []
    for r in runs:
        run_results.append({
            "topology":  str(r["topo"]),
            "log":       str(r["log"]),
            "csv":       str(r.get("csv")) if r.get("csv") else None,
            "returncode": r["returncode"],
            "stats":     compute_stats(r["events"], r["duration_s"]),
        })

    summary = {
        "generated_at": datetime.now().isoformat(),
        "run_config":   run_config,
        "topology_count": len(runs),
        "successful_runs": sum(1 for r in runs if r["returncode"] == 0),
        "failed_runs":    sum(1 for r in runs if r["returncode"] != 0),
        "graphs":  graphs,
        "runs":    run_results,
    }

    out = test_dir / "summary.json"
    with open(out, "w", encoding="utf-8") as f:
        json.dump(summary, f, indent=2, ensure_ascii=False)
    return out


def write_csv_report(runs: list[dict], test_dir: Path) -> Path:
    """CSV consolidado com estatísticas de todas as topologias."""
    out = test_dir / "metrics.csv"
    fieldnames = [
        "topology", "duration_s",
        "packets_total", "packets_dropped", "packets_delivered",
        "drop_rate_pct", "delivery_rate_pct", "throughput_pps",
        "latency_mean_s", "latency_median_s",
        "latency_min_s", "latency_max_s", "latency_p95_s", "latency_std_s",
        "returncode",
    ]
    with open(out, "w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=fieldnames, extrasaction="ignore")
        writer.writeheader()
        for r in runs:
            stats = compute_stats(r["events"], r["duration_s"])
            row = {
                "topology":          r["topo"].name,
                "duration_s":        f"{r['duration_s']:.4f}",
                "packets_total":     stats["packets_total"],
                "packets_dropped":   stats["packets_dropped"],
                "packets_delivered": stats["packets_delivered"],
                "drop_rate_pct":
                    f"{stats['drop_rate']*100:.2f}" if stats["drop_rate"] is not None else "",
                "delivery_rate_pct":
                    f"{stats['delivery_rate']*100:.2f}" if stats["delivery_rate"] is not None else "",
                "throughput_pps":
                    f"{stats['throughput_pps']:.2f}" if stats["throughput_pps"] is not None else "",
                "latency_mean_s":   stats["latency_mean_s"] or "",
                "latency_median_s": stats["latency_median_s"] or "",
                "latency_min_s":    stats["latency_min_s"] or "",
                "latency_max_s":    stats["latency_max_s"] or "",
                "latency_p95_s":    stats["latency_p95_s"] or "",
                "latency_std_s":    stats["latency_std_s"] or "",
                "returncode":       r["returncode"],
            }
            writer.writerow(row)
    return out


# ──────────────────────────────────────────────────────────────
# Fluxo principal
# ──────────────────────────────────────────────────────────────

def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(
        description="KNS Runner v0.9 — executa simulações e gera relatórios"
    )
    parser.add_argument(
        "topologies_dir",
        help="Diretório com arquivos .json de topologia"
    )
    parser.add_argument(
        "--max-procs", "-j", type=int, default=1,
        help="Número máximo de simulações paralelas (padrão: 1)"
    )
    parser.add_argument(
        "--timeout", "-t", type=float, default=None,
        help="Timeout em segundos por execução (padrão: sem limite)"
    )
    args = parser.parse_args(argv)

    topo_dir = Path(args.topologies_dir).resolve()
    if not topo_dir.is_dir():
        print(f"[ERRO] Diretório inválido: {topo_dir}", file=sys.stderr)
        return 1

    root = Path(__file__).resolve().parent.parent
    exe  = find_executable(root)
    if not exe:
        print(
            "[ERRO] Executável kns_app(.exe) não encontrado.\n"
            "       Compile o projeto com CMake antes de executar este script.",
            file=sys.stderr
        )
        return 1

    results_root = root / "results" / "v0.9"
    test_dir     = get_test_dir(results_root)
    print(f"[INFO] Resultados serão salvos em: {test_dir}")

    topologies = sorted(topo_dir.rglob("*.json"))
    if not topologies:
        print(f"[ERRO] Nenhum arquivo .json encontrado em: {topo_dir}", file=sys.stderr)
        return 1

    print(f"[INFO] Topologias encontradas: {len(topologies)}")
    print(f"[INFO] Plataforma: {platform.platform()}")
    if is_linux() and not has_display():
        if xvfb_available():
            print("[INFO] Linux sem display — usando xvfb-run para modo headless")
        else:
            print("[AVISO] Linux sem display e sem xvfb-run. "
                  "A execução pode falhar se o binário exigir janela.")

    # ── Lançamento dos processos ──────────────────────────────
    pending: list[tuple[subprocess.Popen, float, Path, Path, Path | None]] = []
    run_records: list[dict] = []

    for i, topo in enumerate(topologies):
        log_file = test_dir / f"run_{i}.log"
        csv_file = test_dir / f"run_{i}.csv"     # o simulador pode gerar; se não, ignoramos

        print(f"[RUN {i}] {topo.name}")
        try:
            proc, t0 = run_silent(exe, topo, log_file, args.timeout)
            pending.append((proc, t0, topo, log_file, csv_file))
        except Exception as exc:
            print(f"[ERRO] Falha ao iniciar {topo.name}: {exc}", file=sys.stderr)
            run_records.append({
                "topo":       topo,
                "log":        log_file,
                "csv":        None,
                "returncode": -1,
                "duration_s": 0.0,
                "events":     {"drops": [], "deliveries": [], "latencies": []},
            })

        # Controle de paralelismo
        while len(pending) >= args.max_procs:
            _flush_one(pending, run_records, args.timeout)

    # ── Aguarda todos os processos restantes ─────────────────
    while pending:
        _flush_one(pending, run_records, args.timeout, wait=True)

    # ── Geração de gráficos ───────────────────────────────────
    graphs: list[str] = []

    print("[INFO] Gerando gráficos …")
    dash = plot_summary_dashboard(run_records, test_dir)
    graphs.append(str(dash))
    print(f"       dashboard.png")

    out = plot_drops_over_time(run_records, test_dir)
    if out:
        graphs.append(str(out)); print(f"       drops_over_time.png")

    out = plot_latency_distribution(run_records, test_dir)
    if out:
        graphs.append(str(out)); print(f"       latency_distribution.png")

    out = plot_latency_over_time(run_records, test_dir)
    if out:
        graphs.append(str(out)); print(f"       latency_over_time.png")

    heatmaps = plot_drop_heatmap(run_records, test_dir)
    for h in heatmaps:
        graphs.append(str(h)); print(f"       {h.name}")

    # ── Relatórios ────────────────────────────────────────────
    run_config = {
        "project_root":       str(root),
        "topologies_dir":     str(topo_dir),
        "executable":         str(exe),
        "max_procs":          args.max_procs,
        "timeout_seconds":    args.timeout,
        "platform":           platform.platform(),
    }
    with open(test_dir / "run_config.json", "w", encoding="utf-8") as f:
        json.dump(run_config, f, indent=2)

    summary_path = write_summary_json(run_records, test_dir, run_config, graphs)
    csv_path     = write_csv_report(run_records, test_dir)

    # ── Resumo final ──────────────────────────────────────────
    ok  = sum(1 for r in run_records if r["returncode"] == 0)
    err = len(run_records) - ok
    print()
    print("=" * 50)
    print(f"  Simulações concluídas: {ok}/{len(run_records)}"
          + (f"  ({err} com erro)" if err else ""))
    print(f"  Resultados:  {test_dir}")
    print(f"  Métricas:    {csv_path.name}")
    print(f"  Relatório:   {summary_path.name}")
    print(f"  Gráficos:    {len(graphs)} arquivo(s)")
    print("=" * 50)
    return 0


def _flush_one(
    pending:     list,
    run_records: list[dict],
    timeout:     float | None,
    wait:        bool = False
) -> None:
    """Verifica/aguarda a conclusão do primeiro processo pendente."""
    if not pending:
        return

    proc, t0, topo, log_file, csv_file = pending[0]

    if wait:
        try:
            proc.wait(timeout=timeout)
        except subprocess.TimeoutExpired:
            proc.kill()
            proc.wait()
    else:
        # Tenta pegar sem bloquear
        try:
            proc.wait(timeout=0.05)
        except subprocess.TimeoutExpired:
            return   # ainda rodando

    # Processo terminou → registrar
    duration = time.perf_counter() - t0
    rc       = proc.returncode if proc.returncode is not None else -1
    events   = parse_log(log_file)

    # Fecha o arquivo de log aberto pelo Popen
    if proc.stdout and not proc.stdout.closed:
        proc.stdout.close()

    status = "OK" if rc == 0 else f"ERRO (código {rc})"
    print(f"       → {topo.name}: {status}  |  {duration:.2f}s  "
          f"|  {len(events['drops'])} drops  "
          f"|  {len(events['deliveries'])} entregues")

    run_records.append({
        "topo":       topo,
        "log":        log_file,
        "csv":        csv_file if csv_file and csv_file.exists() else None,
        "returncode": rc,
        "duration_s": duration,
        "events":     events,
    })
    pending.pop(0)


if __name__ == "__main__":
    raise SystemExit(main())