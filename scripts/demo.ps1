Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$Root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$BuildDir = Join-Path $Root 'build'

$Python = Get-Command python -ErrorAction SilentlyContinue
if (-not $Python) {
    $Python = Get-Command py -ErrorAction SilentlyContinue
}
if (-not $Python) {
    throw 'Python 3 not found. Install it or add python/py to PATH.'
}
$PythonPath = $Python.Source
if (-not $PythonPath) {
    $PythonPath = $Python.Path
}
if (-not $PythonPath) {
    throw 'Unable to resolve the Python executable path.'
}

New-Item -ItemType Directory -Force -Path $BuildDir | Out-Null

cmake -S $Root -B $BuildDir
if ($LASTEXITCODE -ne 0) { throw 'CMake configure failed.' }

cmake --build $BuildDir --config Debug
if ($LASTEXITCODE -ne 0) { throw 'CMake build failed.' }

& (Join-Path $PSScriptRoot 'run_all.ps1')
& (Join-Path $PSScriptRoot 'run_packet_size.ps1')

& $PythonPath (Join-Path $PSScriptRoot 'plot_results.py')
if ($LASTEXITCODE -ne 0) { throw 'plot_results.py failed.' }

& $PythonPath (Join-Path $PSScriptRoot 'plot_results_packet_size.py')
if ($LASTEXITCODE -ne 0) { throw 'plot_results_packet_size.py failed.' }
