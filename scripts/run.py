import sys
import subprocess
from pathlib import Path

def find_executable(start: Path) -> Path | None:
    candidates = list(start.rglob("kns_app.exe"))
    return candidates[0] if candidates else None

def main() -> int:
    if len(sys.argv) != 2:
        print("Usage: python run.py <direcotyr_with_topologies>")
        return 1

    topo_dir = Path(sys.argv[1]).resolve()
    if not topo_dir.is_dir():
        print(f"Invalid directory: {topo_dir}")
        return 1

    script_dir = Path(__file__).resolve().parent
    project_root = script_dir.parent
    exe = find_executable(project_root)

    if exe is None:
        print("Executable kns_app.exe not found.")
        return 1

    topologies = sorted(
        p for p in topo_dir.rglob("*.json")
        if p.is_file()
    )

    if not topologies:
        print(f"No topology .json found in: {topo_dir}")
        return 1

    processes = []
    for topo in topologies:
        print(f"Starting: {topo.name}")
        processes.append(subprocess.Popen([str(exe), str(topo)]))

    for p in processes:
        p.wait()

    print(f"Finished. Total of topologies executed: {len(topologies)}")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())