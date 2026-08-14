from __future__ import annotations

import pathlib
import re
import unittest


ROOT = pathlib.Path(__file__).resolve().parents[1]
EDITOR = ROOT / "TrackEditor"


def without_comments(source: str) -> str:
    source = re.sub(r"/\*.*?\*/", "", source, flags=re.DOTALL)
    return re.sub(r"//.*", "", source)


class E3S1RenderWorkerContractTests(unittest.TestCase):
    def test_viewport_is_qwidget_and_paint_only_blits_qimage(self) -> None:
        header = (EDITOR / "TrackPreview.h").read_text(encoding="utf-8")
        source = (EDITOR / "TrackPreview.cpp").read_text(encoding="utf-8")
        self.assertIn("class CTrackPreview : public QWidget", header)
        self.assertNotIn("QGLWidget", header)
        self.assertNotIn("QGLWidget", source)
        self.assertNotIn("paintGL", source)
        self.assertNotIn("initializeGL", source)
        paint = re.search(
            r"void CTrackPreview::paintEvent\([^)]*\)\s*\{(?P<body>.*?)\n\}",
            source,
            re.DOTALL,
        )
        self.assertIsNotNone(paint)
        body = paint.group("body")
        self.assertIn("QPainter", body)
        self.assertIn("drawImage", body)
        self.assertNotIn("RollerEd_", body)

    def test_main_thread_lifecycle_brackets_worker_lifecycle(self) -> None:
        source = (EDITOR / "TrackEditor.cpp").read_text(encoding="utf-8")
        bootstrap = source.index("RollerEd_Bootstrap")
        window = source.index("new CMainWindow")
        start = source.index("RenderService.Start")
        event_loop = source.index("app.exec")
        stop = source.index("RenderService.Stop")
        delete_window = source.index("delete pMainWin")
        teardown = source.index("RollerEd_Teardown")
        self.assertLess(bootstrap, window)
        self.assertLess(window, start)
        self.assertLess(start, event_loop)
        self.assertLess(event_loop, stop)
        self.assertLess(stop, delete_window)
        self.assertLess(stop, teardown)

    def test_startup_callbacks_and_worker_exceptions_are_guarded(self) -> None:
        main_window = (EDITOR / "MainWindow.cpp").read_text(encoding="utf-8")
        receiver = main_window.index("g_pLoggingMainWindow.store")
        callback = main_window.index("SetWhipLibLoggingCallback(LogMessageCbStatic)")
        self.assertLess(receiver, callback)
        self.assertIn("if (pMainWindow)", main_window)
        self.assertIn("SetWhipLibLoggingCallback(nullptr)", main_window)
        uncommented_main_window = without_comments(main_window)
        self.assertNotIn("restoreDockWidget(", uncommented_main_window)
        first_default_dock = uncommented_main_window.index("addDockWidget(")
        restore_state = uncommented_main_window.index("restoreState(state)")
        self.assertLess(first_default_dock, restore_state)

        worker = (EDITOR / "EditorRenderService.cpp").read_text(
            encoding="utf-8"
        )
        self.assertIn("catch (const std::exception &Exception)", worker)
        self.assertIn("HandleUnexpectedFailure", worker)
        self.assertIn("ROLLER_ED_RESULT_INTERNAL_ERROR", worker)

    def test_rendering_facade_calls_are_confined_to_worker(self) -> None:
        allowed = {"EditorRenderService.cpp", "TrackEditor.cpp"}
        facade_sources = []
        for path in EDITOR.glob("*.cpp"):
            # Comments are stripped so this measures actual calls: naming an
            # entry point while explaining that you do not call it is not a
            # violation of the boundary.
            text = without_comments(path.read_text(encoding="utf-8"))
            if "RollerEd_" in text:
                facade_sources.append(path.name)
        self.assertEqual(sorted(facade_sources), sorted(allowed))

        worker = (EDITOR / "EditorRenderService.cpp").read_text(encoding="utf-8")
        self.assertIn('AssertWorkerThread("RollerEd_LoadTrackFile")', worker)
        self.assertIn('AssertWorkerThread("RollerEd_RenderFrame")', worker)
        self.assertIn("QThread::currentThread() == this", worker)

    def test_editor_queue_freezes_identity_and_epoch_contract(self) -> None:
        queue = (EDITOR / "EditorRenderQueue.h").read_text(encoding="utf-8")
        state = (EDITOR / "EditorRenderQueue.cpp").read_text(encoding="utf-8")
        self.assertIn("tEdRenderRequestTag", queue)
        self.assertIn("tEdRenderResultTag", queue)
        self.assertIn("ROLLER_ED_REQUEST_HAS_EXPECTED_EPOCH", queue)
        self.assertIn("uint64_t ullDocumentRevision", queue)
        self.assertIn("std::atomic<uint64_t> g_ullNextRequestId", state)
        self.assertIn("std::atomic<uint64_t> g_ullNextDocumentId", state)
        self.assertIn("Tag.ullDocumentRevision == m_ullDocumentRevision", state)
        self.assertIn(
            "Result.Tag.uiActualGeometryEpoch != Result.uiRenderedGeometryEpoch",
            state,
        )
        self.assertIn("std::string sErrorText", queue)

    def test_resize_is_debounced_and_uses_device_pixels(self) -> None:
        source = (EDITOR / "TrackPreview.cpp").read_text(encoding="utf-8")
        self.assertIn("m_pResizeTimer->setSingleShot(true)", source)
        self.assertIn("m_pResizeTimer->setInterval(100)", source)
        self.assertIn("devicePixelRatioF()", source)
        self.assertIn("m_pResizeTimer->start()", source)


if __name__ == "__main__":
    unittest.main()
