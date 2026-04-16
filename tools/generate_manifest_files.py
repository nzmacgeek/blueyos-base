#!/usr/bin/env python3

import json
import sys
from pathlib import Path


def main() -> int:
    if len(sys.argv) != 3:
        print("Usage: generate_manifest_files.py <payload_dir> <manifest_path>", file=sys.stderr)
        return 1

    payload_root = Path(sys.argv[1])
    manifest_path = Path(sys.argv[2])

    with manifest_path.open("r", encoding="utf-8") as f:
        manifest = json.load(f)

    manifest["files"] = [
        {"path": "/" + path.relative_to(payload_root).as_posix()}
        for path in sorted(payload_root.rglob("*"))
        if path.is_file() or path.is_symlink()
    ]

    with manifest_path.open("w", encoding="utf-8") as f:
        json.dump(manifest, f, indent=2)
        f.write("\n")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
