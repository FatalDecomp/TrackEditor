"""E6-S1 — reusable editor build workflow.

*AC:* all three OSes green on PR.

That criterion has been met since E2-S3, but as three hand-written jobs whose
steps had drifted into near-duplicates of each other. The word the story turns
on is **reusable**: E6-S2 (per-OS deploy artifacts) and E6-S3 (tagged releases)
both need a built editor on all three platforms, and neither should restate the
provisioning to get one.

So the shape is a `workflow_call` workflow holding one three-platform matrix,
plus a thin caller that is the PR gate. These cases pin that shape, and the
per-platform facts earlier stories paid for -- the Visual Studio generator, the
MSVC Qt package, SDL provisioning, and recursive submodules -- which a matrix
rewrite is exactly the kind of change that could quietly drop.
"""

import re
from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[1]
WORKFLOWS = ROOT / ".github" / "workflows"
REUSABLE = WORKFLOWS / "editor-build.yml"
CALLER = WORKFLOWS / "build.yml"

EXPECTED_PLATFORMS = {
    "Linux": "ubuntu-26.04",
    "macOS": "macos-15-intel",
    "Windows": "windows-2022",
}


def reusable_text() -> str:
    return REUSABLE.read_text(encoding="utf-8")


def matrix_block() -> str:
    """Just the matrix include list, so step names cannot be mistaken for it."""
    text = reusable_text()
    start = text.index("include:")
    return text[start : text.index("\n    env:", start)]


class ReusabilityTests(unittest.TestCase):
    def test_the_build_workflow_is_callable(self) -> None:
        self.assertTrue(REUSABLE.is_file())
        text = reusable_text()
        self.assertIn("\non:\n", text)
        self.assertIn("workflow_call:", text)

    def test_the_pr_gate_is_a_thin_caller(self) -> None:
        caller = CALLER.read_text(encoding="utf-8")
        self.assertIn("uses: ./.github/workflows/editor-build.yml", caller)
        for trigger in ("push:", "pull_request:"):
            self.assertIn(trigger, caller)
        # If the caller grows steps of its own, the provisioning has started
        # to leak back out of the reusable workflow.
        self.assertNotIn("steps:", caller)
        self.assertNotIn("install-qt-action", caller)

    def test_the_configuration_is_an_input_rather_than_a_literal(self) -> None:
        text = reusable_text()
        self.assertIn("build-type:", text)
        self.assertIn("default: Release", text)
        for command in (
            "--config ${{ inputs.build-type }}",
            "-C ${{ inputs.build-type }}",
            "-DCMAKE_BUILD_TYPE=${{ inputs.build-type }}",
        ):
            self.assertIn(command, text)


class MatrixTests(unittest.TestCase):
    def test_all_three_desktop_platforms_are_in_one_matrix(self) -> None:
        text = matrix_block()
        names = re.findall(r"^\s+- name:\s*(\S+)$", text, re.MULTILINE)
        runners = re.findall(r"^\s+os:\s*(\S+)$", text, re.MULTILINE)
        self.assertEqual(list(EXPECTED_PLATFORMS), names)
        self.assertEqual(list(EXPECTED_PLATFORMS.values()), runners)

    def test_a_failing_platform_does_not_cancel_the_others(self) -> None:
        # These were independent jobs before E6-S1. A matrix cancels siblings
        # by default, which would hide two platforms' results behind one
        # failure -- the opposite of what a three-OS matrix is for.
        self.assertIn("fail-fast: false", reusable_text())

    def test_submodules_are_checked_out_recursively(self) -> None:
        # The pinned ROLLER submodule is the build; without this every platform
        # fails at the FATAL_ERROR in the top-level CMakeLists.
        self.assertIn("submodules: recursive", reusable_text())


class PlatformFactsTests(unittest.TestCase):
    def test_windows_keeps_the_visual_studio_generator(self) -> None:
        # Durable E2 state: plain Ninja without an initialized MSVC environment
        # lets CMake select MinGW and windres, which cannot consume the UTF-16
        # TrackEditor.rc.
        self.assertIn('-G "Visual Studio 17 2022" -A x64', reusable_text())

    def test_every_platform_provisions_sdl(self) -> None:
        text = reusable_text()
        self.assertIn("libsdl3-dev libsdl3-image-dev", text)   # Linux
        self.assertIn("brew install --force-bottle sdl3", text)  # macOS
        self.assertIn("install-sdl-windows.ps1", text)           # Windows
        self.assertTrue((ROOT / "scripts" / "install-sdl-windows.ps1").is_file())

    def test_the_platform_only_steps_are_guarded_by_runner_os(self) -> None:
        text = reusable_text()
        guards = set(re.findall(r"if:\s*runner\.os == '(\w+)'", text))
        self.assertEqual({"Linux", "macOS", "Windows"}, guards)

    def test_each_platform_builds_and_tests(self) -> None:
        text = reusable_text()
        self.assertIn("cmake --build build", text)
        self.assertIn("ctest --test-dir build", text)
        self.assertIn("--output-on-failure", text)


if __name__ == "__main__":
    unittest.main()
