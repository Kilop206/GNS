$ROOT = "$PSScriptRoot\.."
$EXE = "$ROOT\build\app\Debug\kns_app.exe"

New-Item -ItemType Directory -Force -Path "$ROOT\results\packet_size"

foreach ($value in @(1500, 3000, 4500, 6000)) {
    & $EXE $ROOT\topologies\mesh5.json --out $ROOT\results\packet_size\results_packet_size_$value.csv --packet_size $value --seed 42
}