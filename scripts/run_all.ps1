$ROOT = "$PSScriptRoot\.."
$EXE = "$ROOT\build\app\Debug\kns_app.exe"

New-Item -ItemType Directory -Force -Path "$ROOT\results\loss_prob"

foreach ($value in @(0.0, 0.01, 0.05, 0.10, 0.20, 0.30)) {
    & $EXE $ROOT\topologies\mesh5.json --out $ROOT\results\loss_prob\results_loss_prob_$value.csv --loss_prob $value --seed 42
}