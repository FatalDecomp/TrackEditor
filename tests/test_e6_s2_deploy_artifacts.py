"""E6-S2 — per-OS deploy and artifacts.

*AC:* each OS produces a self-contained runnable artifact with dynamic Qt
bundled.

The motivation is concrete rather than procedural: this project needs a runnable
build for manual acceptance constantly, and four of them so far
(`out/e3-s6/`, `out/e4-s5-manual/`, `out/e4a-s6-manual/`, `out/e5-manual/`) were
staged by hand. E6-S1's reusable workflow is what this hangs off.

These cases pin the three deploy tools the specification names, the build-side
prerequisites they need (a macOS bundle, a desktop file and icon for
linuxdeploy), and the packaging details that are easy to get wrong and hard to
notice: tarring the `.app` so its symlinks survive, and asserting a payload
exists rather than trusting `if-no-files-found`.
"""

import re
from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[1]
REUSABLE = ROOT / ".github" / "workflows" / "editor-build.yml"
DESKTOP = ROOT / "packaging" / "trackeditor.desktop"
ICON = ROOT / "TrackEditor" / "images" / "stunt.png"


def workflow() -> str:
    return REUSABLE.read_text(encoding="utf-8")


class DeployToolTests(unittest.TestCase):
    def test_each_platform_uses_its_own_deploy_tool(self) -> None:
        text = workflow()
        for tool in ("windeployqt", "macdeployqt", "linuxdeploy"):
            self.assertIn(tool, text, tool)

    def test_the_deploy_steps_are_guarded_by_platform_and_by_the_input(self) -> None:
        text = workflow()
        guards = re.findall(
            r"if: inputs\.upload-artifacts && runner\.os == '(\w+)'", text
        )
        self.assertEqual({"Windows", "macOS", "Linux"}, set(guards))
        self.assertEqual(3, len(guards))

    def test_deployment_is_an_input_so_callers_can_decline_it(self) -> None:
        text = workflow()
        self.assertIn("upload-artifacts:", text)
        self.assertIn("type: boolean", text)

    def test_qt_comes_from_the_provisioned_toolchain(self) -> None:
        # install-qt-action exports QT_ROOT_DIR; hardcoding a path would break
        # the moment the Qt version moves.
        text = workflow()
        self.assertIn("QT_ROOT_DIR", text)
        self.assertNotIn("6.8.3/msvc", text)


class PackagingPrerequisiteTests(unittest.TestCase):
    def test_the_macos_target_is_a_bundle(self) -> None:
        # macdeployqt operates on a .app; without MACOSX_BUNDLE there is
        # nothing on macOS for it to fill in.
        cmake = (ROOT / "TrackEditor" / "CMakeLists.txt").read_text(encoding="utf-8")
        self.assertIn("MACOSX_BUNDLE", cmake)
        self.assertIn("MACOSX_BUNDLE_GUI_IDENTIFIER", cmake)

    def test_linuxdeploy_has_a_desktop_file_and_a_matching_icon(self) -> None:
        self.assertTrue(DESKTOP.is_file())
        self.assertTrue(ICON.is_file())
        entry = DESKTOP.read_text(encoding="utf-8")
        for key in ("Type=Application", "Exec=TrackEditor", "Categories="):
            self.assertIn(key, entry)

        # linuxdeploy keys the icon on its filename, so the Icon= value and the
        # name the workflow copies it to have to agree.
        icon_name = re.search(r"^Icon=(\S+)$", entry, re.MULTILINE)
        self.assertIsNotNone(icon_name)
        self.assertIn(f"{icon_name.group(1)}.png", workflow())


class ArtifactTests(unittest.TestCase):
    def test_one_named_artifact_per_platform_is_uploaded(self) -> None:
        text = workflow()
        self.assertIn("actions/upload-artifact@v4", text)
        self.assertIn("name: TrackEditor-${{ matrix.name }}", text)
        self.assertIn("retention-days:", text)

    def test_the_macos_bundle_is_tarred_rather_than_zipped(self) -> None:
        # upload-artifact zips its payload, and the framework symlinks inside a
        # .app do not survive that.
        text = workflow()
        self.assertRegex(text, r"tar czf \S+\.tar\.gz .*TrackEditor\.app")

    def test_an_empty_deploy_fails_rather_than_uploading_a_readme(self) -> None:
        # if-no-files-found cannot catch a failed deploy once the description
        # file exists, so the payload is asserted separately.
        text = workflow()
        self.assertIn("the deploy step produced no payload", text)
        self.assertIn("if-no-files-found: error", text)

    def test_the_artifact_says_what_it_is_and_how_to_run_it(self) -> None:
        text = workflow()
        self.assertIn("READ-ME-FIRST.txt", text)
        for fact in ("Commit:", "Branch:", "Qt:"):
            self.assertIn(fact, text)


if __name__ == "__main__":
    unittest.main()
