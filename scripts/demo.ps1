$ROOT = "$PSScriptRoot\.."
$env:PATH = [System.Environment]::GetEnvironmentVariable("PATH", "Machine") + ";" + [System.Environment]::GetEnvironmentVariable("PATH", "User")
$PYTHON = (Get-Command python -ErrorAction SilentlyContinue).Source
if (-not $PYTHON) {
    Write-Error "Python 3 not found. Please add Python to PATH."
    exit 1
}

New-Item -ItemType Directory -Force -Path "$ROOT\build"
Set-Location $ROOT\build
cmake $ROOT
cmake --build .
Set-Location $ROOT\scripts
& .\run_all.ps1
& .\run_packet_size.ps1
& $PYTHON plot_results.py
& $PYTHON plot_results_packet_size.py