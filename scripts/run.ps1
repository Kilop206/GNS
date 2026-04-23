<#
.SYNOPSIS
    KNS Runner v0.9

.DESCRIPTION
    - Salva resultados em ../results/v0.9/test_#N/
    - Funciona em Windows, Linux e macOS (PowerShell 7+ / pwsh)
    - Roda o simulador em segundo plano, sem abrir janela grafica
    - Gera metricas CSV, summary JSON e graficos via Python

.PARAMETER TopologyDirectory
    Diretorio com os arquivos .json de topologia.

.PARAMETER MaxProcs
    Numero maximo de simulacoes paralelas (padrao: 1).

.PARAMETER TimeoutSeconds
    Timeout por processo em segundos. 0 = sem limite (padrao: 0).

.EXAMPLE
    pwsh -File run.ps1 -TopologyDirectory ..\app\topologies
    pwsh -File run.ps1 ..\app\topologies -MaxProcs 2 -TimeoutSeconds 60
#>

[CmdletBinding()]
param(
    [Parameter(Mandatory = $true, Position = 0)]
    [string] $TopologyDirectory,

    [Parameter(Mandatory = $false)]
    [int] $MaxProcs = 1,

    [Parameter(Mandatory = $false)]
    [int] $TimeoutSeconds = 0
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

# =============================================================
# Utilitarios
# =============================================================

function Get-OsName {
    if ($IsWindows) { return "Windows" }
    if ($IsLinux)   { return "Linux"   }
    if ($IsMacOS)   { return "macOS"   }
    return "Unknown"
}

function Find-KnsExecutable {
    param([string]$Root)
    $names = @("kns_app.exe", "kns_app")
    foreach ($name in $names) {
        $found = Get-ChildItem -Path $Root -Recurse -Filter $name -File `
                     -ErrorAction SilentlyContinue |
                 Select-Object -First 1
        if ($found) {
            if ($IsWindows) { return $found.FullName }
            # Linux/macOS: verifica bit de execucao (73 decimal = octal 111)
            $mode = & stat -c "%a" $found.FullName 2>$null
            if ($null -ne $mode -and ([int]$mode -band 73)) {
                return $found.FullName
            }
            return $found.FullName
        }
    }
    return $null
}

function Get-NextTestDir {
    param([string]$Base)
    $null = New-Item -ItemType Directory -Path $Base -Force
    $i = 1
    while ($true) {
        $dir = Join-Path $Base "test_#$i"
        if (-not (Test-Path $dir)) {
            $null = New-Item -ItemType Directory -Path $dir
            return $dir
        }
        $i++
    }
}

function Test-Display {
    if ($IsWindows) { return $true }
    return ($null -ne $env:DISPLAY -and $env:DISPLAY -ne "") -or
           ($null -ne $env:WAYLAND_DISPLAY -and $env:WAYLAND_DISPLAY -ne "")
}

function Test-XvfbRun {
    if (-not $IsLinux) { return $false }
    try { $null = & xvfb-run --help 2>&1; return $true }
    catch { return $false }
}

# =============================================================
# Parsing de log
# =============================================================

function Get-LogStats {
    param([string]$LogFile)

    $result = @{
        Drops      = 0
        Deliveries = 0
        Latencies  = [System.Collections.Generic.List[double]]::new()
    }

    if (-not (Test-Path $LogFile)) { return $result }

    $reDropped   = [regex]'\[DROPPED\]'
    $reDelivered = [regex]'(?i)\[DELIVERED\].*latency[=:\s]+([\d.]+)'
    $reLatency   = [regex]'(?i)\[LATENCY\]\s+([\d.]+)'

    foreach ($line in [System.IO.File]::ReadLines($LogFile)) {
        if ($reDropped.IsMatch($line)) {
            $result.Drops++
            continue
        }
        $m = $reDelivered.Match($line)
        if ($m.Success) {
            $result.Deliveries++
            $result.Latencies.Add([double]$m.Groups[1].Value)
            continue
        }
        $m = $reLatency.Match($line)
        if ($m.Success) {
            $result.Latencies.Add([double]$m.Groups[1].Value)
        }
    }

    return $result
}

function Get-Metrics {
    param([hashtable]$Stats, [double]$DurationSec)

    $total = $Stats.Drops + $Stats.Deliveries
    $m = [ordered]@{
        packets_total     = $total
        packets_dropped   = $Stats.Drops
        packets_delivered = $Stats.Deliveries
        drop_rate         = if ($total -gt 0) { [math]::Round($Stats.Drops      / $total, 4) } else { $null }
        delivery_rate     = if ($total -gt 0) { [math]::Round($Stats.Deliveries / $total, 4) } else { $null }
        duration_seconds  = [math]::Round($DurationSec, 4)
        throughput_pps    = if ($DurationSec -gt 0) {
                                [math]::Round($Stats.Deliveries / $DurationSec, 4)
                            } else { $null }
    }

    if ($Stats.Latencies.Count -gt 0) {
        $sorted = $Stats.Latencies | Sort-Object
        $n      = $sorted.Count
        $sum    = ($sorted | Measure-Object -Sum).Sum
        $m.latency_mean_s   = [math]::Round($sum / $n, 6)
        $m.latency_min_s    = [math]::Round($sorted[0], 6)
        $m.latency_max_s    = [math]::Round($sorted[$n - 1], 6)
        $m.latency_median_s = if ($n % 2 -eq 0) {
            [math]::Round(($sorted[$n/2 - 1] + $sorted[$n/2]) / 2, 6)
        } else {
            [math]::Round($sorted[[math]::Floor($n/2)], 6)
        }
        $p95idx = [math]::Max(0, [math]::Ceiling($n * 0.95) - 1)
        $m.latency_p95_s = [math]::Round($sorted[$p95idx], 6)
    } else {
        $m.latency_mean_s   = $null
        $m.latency_min_s    = $null
        $m.latency_max_s    = $null
        $m.latency_median_s = $null
        $m.latency_p95_s    = $null
    }

    return $m
}

# =============================================================
# Inicializacao do processo do simulador
# =============================================================

function Start-SimProcess {
    param([string]$Exe, [string]$TopoPath, [string]$LogFile)

    $si = [System.Diagnostics.ProcessStartInfo]::new()
    $si.WorkingDirectory       = Split-Path $TopoPath -Parent
    $si.RedirectStandardOutput = $true
    $si.RedirectStandardError  = $true
    $si.RedirectStandardInput  = $true
    $si.UseShellExecute        = $false

    if ($IsLinux -and (-not (Test-Display)) -and (Test-XvfbRun)) {
        $si.FileName  = "xvfb-run"
        $si.Arguments = "--auto-servernum -- `"$Exe`" `"$TopoPath`""
    } else {
        $si.FileName  = $Exe
        $si.Arguments = "`"$TopoPath`""
    }

    if ($IsWindows) { $si.CreateNoWindow = $true }

    $logStream = [System.IO.StreamWriter]::new(
        $LogFile, $false, [System.Text.Encoding]::UTF8)

    $proc = [System.Diagnostics.Process]::new()
    $proc.StartInfo = $si
    $proc.add_OutputDataReceived({ param($s,$e); if ($null -ne $e.Data) { $logStream.WriteLine($e.Data) } })
    $proc.add_ErrorDataReceived({  param($s,$e); if ($null -ne $e.Data) { $logStream.WriteLine($e.Data) } })

    $null = $proc.Start()
    $proc.BeginOutputReadLine()
    $proc.BeginErrorReadLine()

    return @{ Process = $proc; LogStream = $logStream; StartTime = [datetime]::UtcNow }
}

# =============================================================
# CSV consolidado
# =============================================================

function Write-MetricsCsv {
    param([array]$Runs, [string]$OutFile)

    $lines = [System.Collections.Generic.List[string]]::new()
    $lines.Add(
        "topology,duration_s,packets_total,packets_dropped,packets_delivered," +
        "drop_rate_pct,delivery_rate_pct,throughput_pps," +
        "latency_mean_s,latency_median_s,latency_min_s,latency_max_s,latency_p95_s,returncode"
    )

    foreach ($r in $Runs) {
        $m   = $r.Metrics
        $dr  = if ($null -ne $m.drop_rate)      { [math]::Round($m.drop_rate * 100, 2)     } else { "" }
        $dlr = if ($null -ne $m.delivery_rate)  { [math]::Round($m.delivery_rate * 100, 2) } else { "" }
        $thr = if ($null -ne $m.throughput_pps) { $m.throughput_pps                         } else { "" }
        $lines.Add(
            "$($r.TopoName),$($m.duration_seconds),$($m.packets_total)," +
            "$($m.packets_dropped),$($m.packets_delivered)," +
            "$dr,$dlr,$thr," +
            "$($m.latency_mean_s),$($m.latency_median_s)," +
            "$($m.latency_min_s),$($m.latency_max_s),$($m.latency_p95_s)," +
            "$($r.ReturnCode)"
        )
    }

    [System.IO.File]::WriteAllLines($OutFile, $lines, [System.Text.Encoding]::UTF8)
}

# =============================================================
# Funcao auxiliar para finalizar processo
# =============================================================

$allRuns = [System.Collections.Generic.List[hashtable]]::new()

function Complete-Entry {
    param([hashtable]$Entry)

    $Entry.LogStream.Flush()
    $Entry.LogStream.Close()
    $duration = ([datetime]::UtcNow - $Entry.StartTime).TotalSeconds
    $rc       = $Entry.Process.ExitCode
    $stats    = Get-LogStats $Entry.LogFile
    $metrics  = Get-Metrics  $stats $duration

    $status = if ($rc -eq 0) { "OK" } else { "ERRO ($rc)" }
    Write-Host ("       -> {0}: {1}  |  {2:F2}s  |  {3} drops  |  {4} entregues" -f `
        $Entry.TopoName, $status, $duration, $stats.Drops, $stats.Deliveries)

    $allRuns.Add(@{
        TopoName   = $Entry.TopoName
        LogFile    = $Entry.LogFile
        ReturnCode = $rc
        Metrics    = $metrics
    })
}

# =============================================================
# INICIO
# =============================================================

$osName      = Get-OsName
$scriptDir   = Split-Path -Parent $MyInvocation.MyCommand.Path
$projectRoot = Split-Path -Parent $scriptDir

Write-Host "[INFO] KNS Runner v0.9  |  Plataforma: $osName"

$topoResolved = Resolve-Path $TopologyDirectory -ErrorAction SilentlyContinue
if (-not $topoResolved) {
    Write-Error "Diretorio invalido: $TopologyDirectory"
    exit 1
}
$topoDirPath = $topoResolved.Path
Write-Host "[INFO] Topologias: $topoDirPath"

$exe = Find-KnsExecutable $projectRoot
if (-not $exe) {
    Write-Error "kns_app(.exe) nao encontrado em '$projectRoot'. Compile o projeto com CMake."
    exit 1
}
Write-Host "[INFO] Executavel: $exe"

$resultsBase = Join-Path $projectRoot "results" "v0.9"
$testDir     = Get-NextTestDir $resultsBase
Write-Host "[INFO] Resultados: $testDir"

$topologies = Get-ChildItem -Path $topoDirPath -Recurse -Filter "*.json" -File |
              Sort-Object FullName
if (-not $topologies) {
    Write-Error "Nenhum .json encontrado em '$topoDirPath'"
    exit 1
}
Write-Host "[INFO] Topologias encontradas: $($topologies.Count)"

$osDesc = [System.Runtime.InteropServices.RuntimeInformation]::OSDescription
$cfgObj = [ordered]@{
    project_root    = $projectRoot
    topologies_dir  = $topoDirPath
    executable      = $exe
    max_procs       = $MaxProcs
    timeout_seconds = if ($TimeoutSeconds -gt 0) { $TimeoutSeconds } else { $null }
    platform        = "$osName - $osDesc"
}
$cfgObj | ConvertTo-Json |
    Set-Content -Path (Join-Path $testDir "run_config.json") -Encoding UTF8

# =============================================================
# Lancamento e monitoramento
# =============================================================

$active  = [System.Collections.Generic.List[hashtable]]::new()
$runIdx  = 0

foreach ($topo in $topologies) {
    $logFile = Join-Path $testDir "run_$runIdx.log"
    Write-Host "[RUN $runIdx] $($topo.Name) ..."

    try {
        $entry = Start-SimProcess $exe $topo.FullName $logFile
        $entry["TopoName"] = $topo.BaseName
        $entry["LogFile"]  = $logFile
        $active.Add($entry)
    } catch {
        Write-Warning "Falha ao iniciar $($topo.Name): $_"
        $allRuns.Add(@{
            TopoName   = $topo.BaseName
            LogFile    = $logFile
            ReturnCode = -1
            Metrics    = Get-Metrics @{ Drops = 0; Deliveries = 0; Latencies = @() } 0
        })
    }

    $runIdx++

    while ($active.Count -ge $MaxProcs) {
        Start-Sleep -Milliseconds 300
        $done = @($active | Where-Object { $_.Process.HasExited })
        foreach ($d in $done) {
            Complete-Entry $d
            $active.Remove($d) | Out-Null
        }
    }
}

while ($active.Count -gt 0) {
    $entry = $active[0]
    if ($TimeoutSeconds -gt 0) {
        $ok = $entry.Process.WaitForExit($TimeoutSeconds * 1000)
        if (-not $ok) {
            Write-Warning "Timeout atingido para $($entry.TopoName) - encerrando."
            $entry.Process.Kill()
            $entry.Process.WaitForExit(3000) | Out-Null
        }
    } else {
        $entry.Process.WaitForExit()
    }
    Complete-Entry $entry
    $active.RemoveAt(0)
}

# =============================================================
# Relatorios
# =============================================================

$csvPath = Join-Path $testDir "metrics.csv"
Write-MetricsCsv $allRuns $csvPath

$ok  = @($allRuns | Where-Object { $_.ReturnCode -eq 0 }).Count
$err = $allRuns.Count - $ok

$summaryObj = [ordered]@{
    generated_at    = (Get-Date -Format "o")
    run_config      = $cfgObj
    topology_count  = $allRuns.Count
    successful_runs = $ok
    failed_runs     = $err
    graphs          = @()
    runs            = @(foreach ($r in $allRuns) {
        [ordered]@{
            topology   = $r.TopoName
            log        = $r.LogFile
            returncode = $r.ReturnCode
            stats      = $r.Metrics
        }
    })
}
$summaryPath = Join-Path $testDir "summary.json"
$summaryObj | ConvertTo-Json -Depth 6 |
    Set-Content -Path $summaryPath -Encoding UTF8

# =============================================================
# Graficos via Python
# =============================================================

$pyScript  = Join-Path $scriptDir "run.py"
$graphNote = ""

if (Test-Path $pyScript) {
    $py = $null
    foreach ($c in @("python3", "python", "py")) {
        if (Get-Command $c -ErrorAction SilentlyContinue) { $py = $c; break }
    }

    if ($py) {
        Write-Host "[INFO] Gerando graficos via Python ..."

        $sd = $scriptDir   -replace '\\', '/'
        $td = $testDir     -replace '\\', '/'
        $sf = $summaryPath -replace '\\', '/'

        # Linhas do script auxiliar (sem aspas simples em posicao problematica)
        $genLines = @(
            "import sys, json, pathlib",
            "sys.path.insert(0, r'" + $sd + "')",
            "from run import (",
            "    plot_summary_dashboard, plot_drops_over_time,",
            "    plot_latency_distribution, plot_latency_over_time,",
            "    plot_drop_heatmap, parse_log",
            ")",
            "test_dir = pathlib.Path(r'" + $td + "')",
            "sf = pathlib.Path(r'" + $sf + "')",
            "data = json.loads(sf.read_text(encoding='utf-8'))",
            "runs = []",
            "for r in data['runs']:",
            "    log = pathlib.Path(r['log'])",
            "    events = parse_log(log)",
            "    runs.append({",
            "        'topo': pathlib.Path(r['topology']),",
            "        'log': log,",
            "        'returncode': r['returncode'],",
            "        'duration_s': r['stats'].get('duration_seconds', 0),",
            "        'events': events,",
            "    })",
            "graphs = []",
            "out = plot_summary_dashboard(runs, test_dir)",
            "graphs.append(str(out))",
            "out = plot_drops_over_time(runs, test_dir)",
            "if out: graphs.append(str(out))",
            "out = plot_latency_distribution(runs, test_dir)",
            "if out: graphs.append(str(out))",
            "out = plot_latency_over_time(runs, test_dir)",
            "if out: graphs.append(str(out))",
            "for h in plot_drop_heatmap(runs, test_dir):",
            "    graphs.append(str(h))",
            "data['graphs'] = graphs",
            "sf.write_text(json.dumps(data, indent=2, ensure_ascii=False), encoding='utf-8')",
            "print('Graficos gerados:', len(graphs))"
        )

        $genPath = Join-Path $testDir "_gen_graphs.py"
        [System.IO.File]::WriteAllLines(
            $genPath, $genLines, [System.Text.Encoding]::UTF8)

        try {
            & $py $genPath 2>&1 | ForEach-Object { Write-Host "       $_" }
            $graphNote = "  Graficos salvos em $testDir"
        } catch {
            $graphNote = "  Falha ao gerar graficos: $_"
        } finally {
            Remove-Item $genPath -ErrorAction SilentlyContinue
        }
    } else {
        $graphNote = "  Python nao encontrado. Instale para graficos automaticos."
    }
} else {
    $graphNote = "  run.py nao encontrado ao lado deste script."
}

# =============================================================
# Resumo final
# =============================================================

Write-Host ""
Write-Host ("=" * 52)
$errPart = if ($err -gt 0) { "  ($err com erro)" } else { "" }
Write-Host "  Simulacoes concluidas: $ok/$($allRuns.Count)$errPart"
Write-Host "  Resultados : $testDir"
Write-Host "  Metricas   : metrics.csv"
Write-Host "  Relatorio  : summary.json"
if ($graphNote) { Write-Host $graphNote }
Write-Host ("=" * 52)