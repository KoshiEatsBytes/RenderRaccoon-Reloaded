import re
import subprocess
import sys
from pathlib import Path

def is_shader_path(text: str) -> bool:
    lowered = text.lower()
    return lowered.endswith((".vert", ".frag", ".comp", ".geom", ".tesc", ".tese", ".glsl"))

def main() -> int:
    if len(sys.argv) != 3:
        print("usage: lintShader.py <glslang_exe> <shader_file>")
        return 2

    glslang_exe = sys.argv[1]
    shader_file = sys.argv[2]

    result = subprocess.run(
        [glslang_exe, shader_file],
        capture_output=True,
        text=True
    )

    output = "\n".join(part for part in [result.stdout, result.stderr] if part).strip()

    current_file = None

    for raw_line in output.splitlines():
        line = raw_line.strip()
        if not line:
            continue

        # glslang often prints the file path on its own line first
        if is_shader_path(line):
            current_file = str(Path(line).resolve())
            continue

        match = re.match(r"^(ERROR|WARNING):\s+\d+:(\d+):\s+(.*)$", line)
        if match and current_file:
            severity, line_no, message = match.groups()
            print(f"{current_file}:{line_no}: {severity}: {message}")
        else:
            print(line)

    return result.returncode


if __name__ == "__main__":
    raise SystemExit(main())