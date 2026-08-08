"""E6-S3 — nightly builds and tagged releases.

*AC:* a tag publishes per-OS release assets.

Both halves lean entirely on what E6-S1 and E6-S2 already built. Nightly is a
`schedule:` trigger on the existing gate, because E6-S2 already deploys on every
run. The release workflow contains **no build logic at all**: it calls the same
reusable workflow a pull request calls, so a release cannot be built differently
from what CI has been testing. All it does is rename the artifacts and attach
them.

These cases pin that separation -- the thing most likely to erode, because
adding "just one" build step to the release path is always the expedient fix --
along with the asset naming, the write permission being confined to one job, and
the pre-release rule.
"""

import re
from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[1]
WORKFLOWS = ROOT / ".github" / "workflows"
RELEASE = WORKFLOWS / "release.yml"
CALLER = WORKFLOWS / "build.yml"
REUSABLE = WORKFLOWS / "editor-build.yml"


def release_text() -> str:
    return RELEASE.read_text(encoding="utf-8")


class NightlyTests(unittest.TestCase):
    def test_the_gate_runs_on_a_schedule(self) -> None:
        caller = CALLER.read_text(encoding="utf-8")
        self.assertIn("schedule:", caller)
        cron = re.search(r"cron:\s*'([^']+)'", caller)
        self.assertIsNotNone(cron)
        minute = cron.group(1).split()[0]
        # GitHub asks that scheduled workflows avoid the top of the hour.
        self.assertNotEqual("0", minute)

    def test_nightly_needs_no_second_workflow(self) -> None:
        # E6-S2 already deploys on every run, so a nightly is the same gate on
        # a timer rather than a parallel copy of it that could drift.
        self.assertFalse((WORKFLOWS / "nightly.yml").exists())


class ReleaseTriggerTests(unittest.TestCase):
    def test_a_tag_is_what_publishes(self) -> None:
        text = release_text()
        self.assertIn("tags:", text)
        self.assertIn("- 'v*'", text)

    def test_write_permission_is_confined_to_the_publishing_job(self) -> None:
        text = release_text()
        # Top-level read, escalated only where the release is created.
        self.assertIn("permissions:\n  contents: read", text)
        self.assertEqual(1, text.count("contents: write"))


class NoSecondBuildPathTests(unittest.TestCase):
    def test_the_release_builds_through_the_reusable_workflow(self) -> None:
        self.assertIn("uses: ./.github/workflows/editor-build.yml", release_text())

    def test_the_release_workflow_contains_no_build_logic(self) -> None:
        # If a release ever configures or compiles anything itself, it has
        # stopped shipping what CI tested.
        text = release_text()
        for building in ("cmake ", "ctest ", "install-qt-action", "runs-on: windows",
                         "runs-on: macos"):
            self.assertNotIn(building, text, building)

    def test_the_publishing_job_waits_for_the_build(self) -> None:
        self.assertIn("needs: build", release_text())


class AssetTests(unittest.TestCase):
    def test_one_asset_per_platform_is_named_after_the_tag(self) -> None:
        text = release_text()
        for asset in (
            "TrackEditor-${tag}-windows-x64.zip",
            "TrackEditor-${tag}-macos.tar.gz",
            "TrackEditor-${tag}-linux-x86_64.AppImage",
        ):
            self.assertIn(asset, text, asset)

    def test_only_the_windows_payload_is_re_archived(self) -> None:
        # The AppImage and the tarball already ship as single files; zipping
        # them would only make a user unpack and chmod twice.
        text = release_text()
        self.assertEqual(1, text.count("zip -qr"))
        self.assertIn("TrackEditor-macOS.tar.gz", text)
        self.assertIn("TrackEditor-x86_64.AppImage", text)

    def test_a_missing_platform_fails_rather_than_publishing_a_partial_release(
        self,
    ) -> None:
        self.assertIn('test "$(ls release | wc -l)" -eq 3', release_text())

    def test_a_hyphenated_tag_is_a_pre_release(self) -> None:
        text = release_text()
        self.assertIn("--prerelease", text)
        self.assertIn("steps.assemble.outputs.prerelease", text)

    def test_publishing_uses_the_bundled_cli_rather_than_a_third_party_action(
        self,
    ) -> None:
        # Consistent with the project pinning what it downloads: gh ships on
        # the runner, so there is no extra action to trust or pin.
        text = release_text()
        self.assertIn("gh release create", text)
        self.assertIn("--verify-tag", text)
        self.assertNotIn("softprops/", text)

    def test_the_notes_say_how_to_run_each_asset(self) -> None:
        text = release_text()
        self.assertIn("--notes-file", text)
        for hint in ("right-click", "chmod +x", "redistributable"):
            self.assertIn(hint, text, hint)


class ReusableWorkflowTests(unittest.TestCase):
    def test_the_release_gets_artifacts_without_asking(self) -> None:
        # E6-S2's upload-artifacts input defaults to true, which is what makes
        # the release workflow able to say nothing about deployment.
        reusable = REUSABLE.read_text(encoding="utf-8")
        block = reusable[reusable.index("upload-artifacts:") :]
        self.assertIn("default: true", block[: block.index("permissions:")])


if __name__ == "__main__":
    unittest.main()
