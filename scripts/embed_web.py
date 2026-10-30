"""Embed compressed public app assets in flash; no filesystem upload required."""
Import("env")
from pathlib import Path
import gzip
import re

root = Path(env.subst("$PROJECT_DIR"))
target = Path(env.subst("$BUILD_DIR")) / "generated"
target.mkdir(parents=True, exist_ok=True)
parts = ["#pragma once\n#include <Arduino.h>\nnamespace WebAssets {\n"]
for asset in sorted((root / "web").iterdir()):
    if not asset.is_file():
        continue
    name = re.sub(r"[^a-zA-Z0-9_]", "_", asset.name)
    content = gzip.compress(asset.read_bytes(), mtime=0)
    parts.append("static const uint8_t " + name + "[] PROGMEM = {\n")
    parts.extend(",".join(str(v) for v in content[i:i+32]) + ",\n" for i in range(0, len(content), 32))
    parts.append("};\n")
parts.append("}\n")
header = target / "WebAssets.h"
text = "".join(parts)
if not header.exists() or header.read_text() != text:
    header.write_text(text, encoding="utf-8")
env.Append(CPPPATH=[str(target)])
