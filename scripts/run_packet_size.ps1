Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$Root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$Topology = Join-Path $Root 'app/topologies/mesh5.json'
$ResultsDir = Join-Path $Root 'results/packet_size'

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

$PacketSizes = @('1500', '3000', '4500', '6000')

foreach ($Value in $PacketSizes) {
    $OutFile = Join-Path $ResultsDir ("results_packet_size_{0}.csv" -f $Value)
    & $Exe $Topology --out $OutFile --packet_size $Value --seed 42
    if ($LASTEXITCODE -ne 0) {
        throw "Simulation failed for packet_size=$Value (exit code $LASTEXITCODE)."
    }
}
