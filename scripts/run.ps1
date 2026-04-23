param(
    [string]$topology
)

if (-not $topology) {
    Write-Host "Uso: ./run.ps1 <topology.json>"
    exit 1
}

$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$exe = Get-ChildItem -Path $root -Recurse -Filter "kns_app.exe" | Select-Object -First 1

if (-not $exe) {
    Write-Host "Executável não encontrado."
    exit 1
}

& $exe.FullName $topology
