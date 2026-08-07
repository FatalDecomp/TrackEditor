"""E5-S2 — assert a built binary links exactly the Qt Core/Gui/Widgets set.

The CMake guard in the top-level `CMakeLists.txt` proves the build graph never
asks for Qt OpenGL. This proves it of the artifact that actually shipped, which
is what the acceptance criterion is about, and it does so without any platform
tool: a dynamically linked executable names its dependencies as plain byte
strings, whether they are PE import-table entries, ELF `DT_NEEDED` sonames, or
Mach-O load-command framework paths.

Usage: check_linked_qt_modules.py <executable>
"""

import re
import sys
from pathlib import Path


# Windows import table, ELF soname, macOS framework load command.
MODULE_PATTERNS = (
    re.compile(rb"Qt6([A-Za-z]+)\.dll"),
    re.compile(rb"libQt6([A-Za-z]+)\.so"),
    re.compile(rb"Qt([A-Za-z]+)\.framework"),
)

REQUIRED_MODULES = {"Core", "Gui", "Widgets"}


def linked_qt_modules(binary: Path) -> set[str]:
    image = binary.read_bytes()
    modules = set()
    for pattern in MODULE_PATTERNS:
        for match in pattern.findall(image):
            modules.add(match.decode("ascii"))
    return modules


def main(argv: list[str]) -> int:
    if len(argv) != 2:
        print(__doc__.strip().splitlines()[-1], file=sys.stderr)
        return 2

    binary = Path(argv[1])
    if not binary.is_file():
        print(f"not a file: {binary}", file=sys.stderr)
        return 2

    modules = linked_qt_modules(binary)
    print(f"{binary.name} links Qt modules: {sorted(modules) or '<none found>'}")

    if not modules:
        # A static Qt would defeat the scan entirely; the supported build is
        # dynamic Qt (docs/building.md), so finding nothing means the check
        # did not run rather than that it passed.
        print(
            "no Qt module names found in the binary; this check assumes the "
            "supported dynamic Qt build",
            file=sys.stderr,
        )
        return 1

    unexpected = sorted(modules - REQUIRED_MODULES)
    if unexpected:
        print(
            f"E5-S2: {binary.name} links Qt modules outside Core/Gui/Widgets: "
            f"{unexpected}",
            file=sys.stderr,
        )
        return 1

    missing = sorted(REQUIRED_MODULES - modules)
    if missing:
        print(f"E5-S2: {binary.name} is missing Qt modules: {missing}", file=sys.stderr)
        return 1

    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
