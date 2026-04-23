import sys
import subprocess
from pathlib import Path

if len(sys.argv) < 2:
    print("Uso: python run.py <topology.json>")
    sys.exit(1)

topology = sys.argv[1]
root = Path(__file__).resolve().parent

exe = None
for p in root.rglob("kns_app.exe"):
    exe = p
    break

if not exe:
    print("Executável não encontrado.")
    sys.exit(1)

subprocess.run([str(exe), topology])
