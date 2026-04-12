$ROOT = "$PSScriptRoot\.."
$PYTHON = "C:\Users\guilh\AppData\Local\Programs\Python\Python313\python.exe"

New-Item -ItemType Directory -Force -Path "$ROOT\build"
Set-Location $ROOT\build
cmake $ROOT
cmake --build .
Set-Location $ROOT\scripts
& .\run_all.ps1
& .\run_packet_size.ps1
& $PYTHON plot_results.py
& $PYTHON plot_results_packet_size.py