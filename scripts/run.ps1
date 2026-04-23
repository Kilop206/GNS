param(
    [Parameter(Mandatory = $true)]
    [string]$TopologyDirectory
)

$topoDir = Resolve-Path $TopologyDirectory -ErrorAction SilentlyContinue
if (-not $topoDir) {
    Write-Host "Invalid direcoty: $TopologyDirectory"
    exit 1
}

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$projectRoot = Split-Path -Parent $scriptDir

$exe = Get-ChildItem -Path $projectRoot -Recurse -Filter "kns_app.exe" -File | Select-Object -First 1
if (-not $exe) {
    Write-Host "Executable kns_app.exe not found."
    exit 1
}

$topologies = Get-ChildItem -Path $topoDir.Path -Recurse -Filter "*.json" -File | Sort-Object FullName
if (-not $topologies) {
    Write-Host "No .json topology found in: $($topoDir.Path)"
    exit 1
}

$ პროცესses = @()

foreach ($topo in $topologies) {
    Write-Host "Starting: $($topo.Name)"
    $proc = Start-Process -FilePath $exe.FullName -ArgumentList @($topo.FullName) -PassThru
    $processes += $proc
}

if ($processes.Count -gt 0) {
    Wait-Process -Id $processes.Id
}

Write-Host "Finished. Total of executed topologies: $($topologies.Count)"