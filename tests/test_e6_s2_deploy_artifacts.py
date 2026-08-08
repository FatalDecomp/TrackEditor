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


class DeployedArtifactTests(unittest.TestCase):
    """The rules that came out of the first nightly, which shipped two broken
    artifacts.

    Windows staged a 64-bit exe beside 32-bit SDL DLLs -- the archives carry
    arm64, x64, and x86 copies, and a recursive glob flattened all three into
    one directory with x86 sorting last. It started with 0xc000007b and looked
    complete. Linux built the AppImage on ubuntu-26.04, so it demanded
    GLIBC_2.43 and ran on ubuntu-26.04 and nowhere else.

    Both had the same root cause: the deploy tools reported success and nobody
    ran the result. The build tree's own smoke test does not count -- it finds
    SDL on PATH, so it says nothing about what was staged.
    """

    def test_windows_takes_sdl_from_the_x64_directory_only(self) -> None:
        self.assertIn("$_.Directory.Name -eq 'x64'", workflow())

    def test_windows_verifies_every_staged_binary_is_x64(self) -> None:
        text = workflow()
        self.assertIn("0x8664", text)          # IMAGE_FILE_MACHINE_AMD64
        self.assertIn("not x64:", text)

    def test_every_deployed_artifact_is_executed_before_upload(self) -> None:
        text = workflow()
        # Windows: run the staged tree with only the system directories on PATH.
        self.assertIn('$env:PATH = "$env:SystemRoot\\system32;$env:SystemRoot"', text)
        # And wait for it. TrackEditor is a GUI-subsystem binary, so the call
        # operator returns immediately and leaves $LASTEXITCODE unset -- a
        # check written that way passes no matter what, which is the same class
        # of defect this whole test exists to catch.
        self.assertIn("Start-Process -FilePath \"$stage\\TrackEditor.exe\"", text)
        self.assertIn("-Wait -PassThru", text)
        self.assertIn("$smoke.ExitCode -ne 0", text)
        # Linux: run the AppImage itself.
        self.assertIn(
            "QT_QPA_PLATFORM=offscreen ./dist/TrackEditor-x86_64.AppImage "
            "--cmake-smoke-test",
            text,
        )

    def test_the_appimage_glibc_floor_is_reported(self) -> None:
        # It is the number that decides where the AppImage runs, and it was
        # invisible until an artifact failed to start.
        self.assertIn("GLIBC_", workflow())


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
