Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$Root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$Topology = Join-Path $Root 'app/topologies/mesh5.json'
$ResultsDir = Join-Path $Root 'results/loss_prob'

if (-not (Test-Path $Topology)) {
    throw "Topology file not found: $Topology"
}

New-Item -ItemType Directory -Force -Path $ResultsDir | Out-Null

$ExeCandidates = @(
    (Join-Path $Root 'build/app/Debug/kns_app.exe'),
    (Join-Path $Root 'build/app/Release/kns_app.exe'),
    (Join-Path $Root 'build/app/RelWithDebInfo/kns_app.exe'),
    (Join-Path $Root 'build/app/MinSizeRel/kns_app.exe')
)

$Exe = $ExeCandidates | Where-Object { Test-Path $_ } | Select-Object -First 1
if (-not $Exe) {
    $found = Get-ChildItem -Path (Join-Path $Root 'build') -Filter 'kns_app*' -File -Recurse -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($found) {
        $Exe = $found.FullName
    }
}

if (-not $Exe) {
    throw "kns_app executable not found under: $(Join-Path $Root 'build')"
}

$LossProbabilities = @('0.0', '0.01', '0.05', '0.10', '0.20', '0.30')

foreach ($Value in $LossProbabilities) {
    $OutFile = Join-Path $ResultsDir ("results_loss_prob_{0}.csv" -f $Value)
    & $Exe $Topology --out $OutFile --loss_prob $Value --seed 42
    if ($LASTEXITCODE -ne 0) {
        throw "Simulation failed for loss_prob=$Value (exit code $LASTEXITCODE)."
    }
}
