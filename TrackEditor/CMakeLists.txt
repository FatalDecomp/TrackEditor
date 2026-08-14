set(trackeditor_sources
    AssignBacksDialog.cpp
    AssignBacksDialog.h
    AssignBacksDialog.ui
    BackWidget.cpp
    BackWidget.h
    BackWidget.ui
    DebugChunkData.cpp
    DebugChunkData.h
    DebugChunkData.ui
    DebugSlider.cpp
    DebugSlider.h
    DisplaySettings.cpp
    DisplaySettings.h
    DisplaySettingsFlags.h
    DisplaySettings.ui
    EditAudioWidget.cpp
    EditAudioWidget.h
    EditAudioWidget.ui
    EditGeometryWidget.cpp
    EditGeometryWidget.h
    EditGeometryWidget.ui
    EditSeriesDialog.cpp
    EditSeriesDialog.h
    EditSeriesDialog.ui
    EditSignWidget.cpp
    EditSignWidget.h
    EditSignWidget.ui
    EditStuntWidget.cpp
    EditStuntWidget.h
    EditStuntWidget.ui
    EditTowerWidget.cpp
    EditTowerWidget.h
    EditTowerWidget.ui
    EditSurfaceDialog.cpp
    EditSurfaceDialog.h
    EditSurfaceDialog.ui
    EditorCameraController.cpp
    EditorCameraController.h
    EditorBatchExporter.cpp
    EditorBatchExporter.h
    EditorCarModel.cpp
    EditorCarModel.h
    EditorExportCommon.cpp
    EditorExportCommon.h
    EditorExportFormat.cpp
    EditorExportFormat.h
    EditorGltfExporter.cpp
    EditorGltfExporter.h
    EditorObjExporter.cpp
    EditorObjExporter.h
    EditorObjImporter.cpp
    EditorObjImporter.h
    EditorOverlaySettings.cpp
    EditorOverlaySettings.h
    EditorReferenceMesh.cpp
    EditorReferenceMesh.h
    EditorSignModel.cpp
    EditorSignModel.h
    EditorTowerModel.cpp
    EditorTowerModel.h
    EditorRenderQueue.cpp
    EditorRenderQueue.h
    EditorRenderService.cpp
    EditorRenderService.h
    ExportWizard.cpp
    ExportWizard.h
    ExportWizard.ui
    GlobalTrackSettings.cpp
    GlobalTrackSettings.h
    GlobalTrackSettings.ui
    GraphicsDialog.cpp
    GraphicsDialog.h
    GraphicsDialog.ui
    LogDialog.cpp
    LogDialog.h
    LogDialog.ui
    MainWindow.cpp
    MainWindow.h
    MainWindow.ui
    MathHelpers.cpp
    MathHelpers.h
    NewTrackDialog.cpp
    NewTrackDialog.h
    NewTrackDialog.ui
    PreferencesDialog.cpp
    PreferencesDialog.h
    PreferencesDialog.ui
    QtHelpers.cpp
    QtHelpers.h
    QtUserKeyMapper.cpp
    QtUserKeyMapper.h
    SignType.h
    TilePicker.cpp
    TilePicker.h
    TilePicker.ui
    Track.cpp
    Track.h
    TrackEditor.cpp
    TrackEditor.h
    TrackPreview.cpp
    TrackPreview.h
    Types.h
    resource.qrc)

if(WIN32)
    list(APPEND trackeditor_sources TrackEditor.rc)
endif()

# E6-S2. WIN32 and MACOSX_BUNDLE each apply only on their own platform. The
# bundle is what macdeployqt operates on; without it there is nothing on macOS
# for the deploy step to fill in.
add_executable(TrackEditor WIN32 MACOSX_BUNDLE ${trackeditor_sources})

set_target_properties(TrackEditor PROPERTIES
    MACOSX_BUNDLE_BUNDLE_NAME "TrackEditor"
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.fataldecomp.trackeditor"
    MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}")

set_target_properties(TrackEditor PROPERTIES
    AUTOMOC ON
    AUTORCC ON
    AUTOUIC ON
    AUTOUIC_SEARCH_PATHS "${CMAKE_CURRENT_SOURCE_DIR}")

target_compile_features(TrackEditor PRIVATE cxx_std_17)
trackeditor_set_warnings(TrackEditor)
trackeditor_gate_qt_deprecations(TrackEditor)
target_compile_definitions(TrackEditor
    PRIVATE
        QT_NO_PDF
        $<$<PLATFORM_ID:Windows>:IS_WINDOWS>
        $<$<CXX_COMPILER_ID:MSVC>:_CRT_SECURE_NO_WARNINGS>)

target_include_directories(TrackEditor PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}")

target_link_libraries(TrackEditor
    PRIVATE
        TrackEditor::track-model
        TrackEditor::track-assets
        TrackEditor::cgltf
        ROLLER::core
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets)
