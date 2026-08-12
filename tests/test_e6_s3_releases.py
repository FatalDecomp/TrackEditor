"""E6-S3 — nightly builds and tagged releases.

*AC:* a tag publishes per-OS release assets.

Both halves lean entirely on what E6-S1 and E6-S2 already built. Neither the
nightly nor the tagged release contains **any build logic**: both call the same
reusable workflow a pull request calls, so a release cannot be built differently
from what CI has been testing. All they do is assemble the artifacts and publish.

The nightly began as a bare `schedule:` trigger on the gate, which produced
14-day artifacts and no release -- so getting a runnable build still meant
digging into a workflow run. It is now its own workflow publishing a rolling
pre-release, matching what ROLLER's nightly does.

These cases pin the separation of build from publish -- the thing most likely to
erode, because adding "just one" build step to a release path is always the
expedient fix -- along with the asset naming, the single shared assembly script,
the write permission being confined to one job, and the pre-release rules.
"""

import re
from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[1]
WORKFLOWS = ROOT / ".github" / "workflows"
RELEASE = WORKFLOWS / "release.yml"
NIGHTLY = WORKFLOWS / "nightly.yml"
CALLER = WORKFLOWS / "build.yml"
REUSABLE = WORKFLOWS / "editor-build.yml"
ASSEMBLE = ROOT / "scripts" / "assemble-release-assets.sh"


def release_text() -> str:
    return RELEASE.read_text(encoding="utf-8")


def nightly_text() -> str:
    return NIGHTLY.read_text(encoding="utf-8")


def assemble_text() -> str:
    return ASSEMBLE.read_text(encoding="utf-8")


class NightlyTests(unittest.TestCase):
    def test_the_nightly_runs_on_a_schedule_and_by_hand(self) -> None:
        text = nightly_text()
        self.assertIn("schedule:", text)
        cron = re.search(r"cron:\s*'([^']+)'", text)
        self.assertIsNotNone(cron)
        # GitHub asks that scheduled workflows avoid the top of the hour.
        self.assertNotEqual("0", cron.group(1).split()[0])
        # Nothing here has ever run; a manual trigger is how you find out
        # whether it works without waiting a day per attempt.
        self.assertIn("workflow_dispatch:", text)

    def test_master_is_built_once_a_night_not_twice(self) -> None:
        # This assertion used to be its own reverse: the schedule lived on the
        # gate and there was no nightly workflow at all. Now that the nightly
        # calls the reusable workflow itself, a schedule on the gate as well
        # would build master twice for one set of artifacts.
        self.assertNotIn("schedule:", CALLER.read_text(encoding="utf-8"))

    def test_an_unchanged_master_publishes_nothing(self) -> None:
        text = nightly_text()
        self.assertIn("needs.changes.outputs.should_build == 'true'", text)
        self.assertIn("git/ref/tags/nightly", text)

    def test_the_nightly_publishes_a_rolling_prerelease(self) -> None:
        text = nightly_text()
        self.assertIn("gh release create nightly", text)
        self.assertIn("--prerelease", text)
        # Delete-and-recreate moves the tag and leaves exactly one nightly
        # behind, so there is no cleanup job to get wrong.
        self.assertIn("gh release delete nightly --cleanup-tag --yes", text)
        self.assertIn('--target "${GITHUB_SHA}"', text)


class ReleaseTriggerTests(unittest.TestCase):
    def test_a_tag_is_what_publishes(self) -> None:
        text = release_text()
        self.assertIn("tags:", text)
        self.assertIn("- 'v*'", text)

    def test_write_permission_is_confined_to_the_publishing_job(self) -> None:
        for text in (release_text(), nightly_text()):
            # Top-level read, escalated only where the release is created.
            self.assertIn("permissions:\n  contents: read", text)
            self.assertEqual(1, text.count("contents: write"))


class NoSecondBuildPathTests(unittest.TestCase):
    def test_both_release_paths_build_through_the_reusable_workflow(self) -> None:
        for text in (release_text(), nightly_text()):
            self.assertIn("uses: ./.github/workflows/editor-build.yml", text)

    def test_neither_release_path_contains_build_logic(self) -> None:
        # If a release ever configures or compiles anything itself, it has
        # stopped shipping what CI tested.
        for text in (release_text(), nightly_text()):
            for building in (
                "cmake ",
                "ctest ",
                "install-qt-action",
                "runs-on: windows",
                "runs-on: macos",
            ):
                self.assertNotIn(building, text, building)

    def test_the_publishing_jobs_wait_for_the_build(self) -> None:
        self.assertIn("needs: build", release_text())
        self.assertIn("needs: [changes, build]", nightly_text())


class AssetTests(unittest.TestCase):
    def test_both_release_paths_assemble_assets_the_same_way(self) -> None:
        # An asset that appears on only one of the two is a difference nobody
        # discovers until they need it, so assembly is one shared script rather
        # than two copies of the same shell.
        self.assertTrue(ASSEMBLE.is_file())
        for text in (release_text(), nightly_text()):
            self.assertIn("scripts/assemble-release-assets.sh", text)
            self.assertNotIn("zip -qr", text)

    def test_one_asset_per_platform_is_named_after_the_label(self) -> None:
        text = assemble_text()
        for asset in (
            "TrackEditor-${label}-windows-x64.zip",
            "TrackEditor-${label}-macos-${arch}.tar.gz",
            "TrackEditor-${label}-linux-x86_64.AppImage",
        ):
            self.assertIn(asset, text, asset)
        # Both macOS architectures ship; a bundle cannot carry them at once.
        self.assertIn("for arch in arm64 x86_64", text)

    def test_only_the_windows_payload_is_re_archived(self) -> None:
        # The AppImage and the tarball already ship as single files; zipping
        # them would only make a user unpack and chmod twice.
        text = assemble_text()
        self.assertEqual(1, text.count("zip -qr"))
        self.assertIn("TrackEditor-macOS.tar.gz", text)
        self.assertIn("TrackEditor-x86_64.AppImage", text)

    def test_a_missing_platform_fails_rather_than_publishing_a_partial_release(
        self,
    ) -> None:
        self.assertIn('-ne "$expected"', assemble_text())
        self.assertIn("expected=4", assemble_text())

    def test_a_hyphenated_tag_is_a_pre_release(self) -> None:
        text = release_text()
        self.assertIn("--prerelease", text)
        self.assertIn("steps.assemble.outputs.prerelease", text)

    def test_publishing_uses_the_bundled_cli_rather_than_a_third_party_action(
        self,
    ) -> None:
        # Consistent with the project pinning what it downloads: gh ships on
        # the runner, so there is no extra action to trust or pin.
        for text in (release_text(), nightly_text()):
            self.assertIn("gh release create", text)
            self.assertNotIn("softprops/", text)
        self.assertIn("--verify-tag", release_text())

    def test_the_notes_say_how_to_run_each_asset(self) -> None:
        for text in (release_text(), nightly_text()):
            self.assertIn("--notes-file", text)
            for hint in ("right-click", "chmod +x", "redistributable"):
                self.assertIn(hint, text, hint)


class ReusableWorkflowTests(unittest.TestCase):
    def test_the_release_paths_get_artifacts_without_asking(self) -> None:
        # E6-S2's upload-artifacts input defaults to true, which is what makes
        # both publishing workflows able to say nothing about deployment.
        reusable = REUSABLE.read_text(encoding="utf-8")
        block = reusable[reusable.index("upload-artifacts:") :]
        self.assertIn("default: true", block[: block.index("permissions:")])


if __name__ == "__main__":
    unittest.main()
