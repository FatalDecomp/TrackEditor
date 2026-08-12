import configparser
import json
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest


REPOSITORY_ROOT = Path(__file__).resolve().parents[1]
ROLLER_ROOT = REPOSITORY_ROOT / "external" / "ROLLER"
EXPECTED_ROLLER_COMMIT = "bff1d18c778f2b563b2119b9d42961ded7b884f2"


def run_git(*arguments: str, cwd: Path = REPOSITORY_ROOT) -> subprocess.CompletedProcess:
    return subprocess.run(
        ["git", "-c", f"safe.directory={REPOSITORY_ROOT}", *arguments],
        cwd=cwd,
        check=False,
        capture_output=True,
        text=True,
    )


def write_fake_package(
    package_root: Path, package_name: str, target_name: str, version: str
) -> None:
    (package_root / f"{package_name}Config.cmake").write_text(
        f"add_library({target_name} INTERFACE IMPORTED)\n", encoding="ascii"
    )
    (package_root / f"{package_name}ConfigVersion.cmake").write_text(
        f'set(PACKAGE_VERSION "{version}")\n'
        "if(PACKAGE_FIND_VERSION VERSION_GREATER PACKAGE_VERSION)\n"
        "  set(PACKAGE_VERSION_COMPATIBLE FALSE)\n"
        "else()\n"
        "  set(PACKAGE_VERSION_COMPATIBLE TRUE)\n"
        "endif()\n",
        encoding="ascii",
    )


class RollerSubmoduleTests(unittest.TestCase):
    def test_submodule_is_canonical_initialized_and_pinned(self) -> None:
        modules = configparser.ConfigParser()
        modules.read(REPOSITORY_ROOT / ".gitmodules", encoding="ascii")
        section = 'submodule "external/ROLLER"'

        self.assertIn(section, modules)
        self.assertEqual(modules[section]["path"], "external/ROLLER")
        self.assertEqual(
            modules[section]["url"],
            "https://github.com/FatalDecomp/ROLLER.git",
        )
        self.assertTrue(
            (ROLLER_ROOT / "CMakeLists.txt").is_file(),
            "initialize submodules with: git submodule update --init --recursive",
        )

        index_entry = run_git("ls-files", "--stage", "external/ROLLER")
        self.assertEqual(index_entry.returncode, 0, msg=index_entry.stderr)

        nested_head = subprocess.run(
            [
                "git",
                "-c",
                f"safe.directory={ROLLER_ROOT}",
                "rev-parse",
                "HEAD",
            ],
            cwd=ROLLER_ROOT,
            check=False,
            capture_output=True,
            text=True,
        )
        self.assertEqual(nested_head.returncode, 0, msg=nested_head.stderr)
        self.assertEqual(EXPECTED_ROLLER_COMMIT, nested_head.stdout.strip())

        fields = index_entry.stdout.strip().split()
        if fields:
            self.assertGreaterEqual(len(fields), 3)
            self.assertEqual(fields[0], "160000")
            self.assertEqual(fields[1], nested_head.stdout.strip())

    def test_cmake_configures_only_roller_core(self) -> None:
        cmake = shutil.which("cmake")
        self.assertIsNotNone(cmake, "CMake is required for the E2-S1 check")

        with tempfile.TemporaryDirectory() as temporary_directory:
            temporary_root = Path(temporary_directory)
            package_root = temporary_root / "packages"
            package_root.mkdir()
            write_fake_package(package_root, "SDL3", "SDL3::SDL3", "3.2.22")
            write_fake_package(
                package_root,
                "SDL3_image",
                "SDL3_image::SDL3_image",
                "3.2.4",
            )

            build_root = temporary_root / "build"
            query_root = build_root / ".cmake" / "api" / "v1" / "query"
            query_root.mkdir(parents=True)
            (query_root / "codemodel-v2").write_text("", encoding="ascii")

            command = [
                cmake,
                "-S",
                str(REPOSITORY_ROOT),
                "-B",
                str(build_root),
                f"-DCMAKE_PREFIX_PATH={package_root}",
                "-DTRACKEDITOR_BUILD_APPLICATION=OFF",
                "-DCMAKE_DISABLE_FIND_PACKAGE_WildMidi=TRUE",
                "-DCMAKE_DISABLE_FIND_PACKAGE_libcdio=TRUE",
            ]
            if sys.platform != "win32":
                command.extend(["-G", "Ninja"])

            result = subprocess.run(
                command,
                cwd=REPOSITORY_ROOT,
                check=False,
                capture_output=True,
                text=True,
            )
            self.assertEqual(
                result.returncode,
                0,
                msg=f"CMake configure failed:\n{result.stdout}\n{result.stderr}",
            )

            cache = (build_root / "CMakeCache.txt").read_text(
                encoding="utf-8", errors="replace"
            )
            self.assertIn("ROLLER_BUILD_EDITOR_CORE:BOOL=ON", cache)
            self.assertIn("ROLLER_BUILD_GAME:BOOL=OFF", cache)
            self.assertNotIn("WildMidi_DIR", cache)
            self.assertNotIn("libcdio_DIR", cache)

            reply_root = build_root / ".cmake" / "api" / "v1" / "reply"
            index_path = next(reply_root.glob("index-*.json"))
            index = json.loads(index_path.read_text(encoding="utf-8"))
            codemodel_path = reply_root / index["reply"]["codemodel-v2"]["jsonFile"]
            codemodel = json.loads(codemodel_path.read_text(encoding="utf-8"))
            target_names = {
                target["name"]
                for target in codemodel["configurations"][0]["targets"]
            }
            self.assertIn("roller-core", target_names)
            self.assertNotIn("roller", target_names)


if __name__ == "__main__":
    unittest.main()
