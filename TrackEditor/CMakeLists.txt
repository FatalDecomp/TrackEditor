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
    EditSurfaceDialog.cpp
    EditSurfaceDialog.h
    EditSurfaceDialog.ui
    ExportWizard.cpp
    ExportWizard.h
    ExportWizard.ui
    GlobalTrackSettings.cpp
    GlobalTrackSettings.h
    GlobalTrackSettings.ui
    LogDialog.cpp
    LogDialog.h
    LogDialog.ui
    MainWindow.cpp
    MainWindow.h
    MainWindow.ui
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
    TilePicker.cpp
    TilePicker.h
    TilePicker.ui
    TrackEditor.cpp
    TrackEditor.h
    TrackPreview.cpp
    TrackPreview.h
    resource.qrc)

if(WIN32)
    list(APPEND trackeditor_sources TrackEditor.rc)
endif()

add_executable(TrackEditor WIN32 ${trackeditor_sources})

set_target_properties(TrackEditor PROPERTIES
    AUTOMOC ON
    AUTORCC ON
    AUTOUIC ON
    AUTOUIC_SEARCH_PATHS "${CMAKE_CURRENT_SOURCE_DIR}")

target_compile_features(TrackEditor PRIVATE cxx_std_17)
target_compile_definitions(TrackEditor
    PRIVATE
        QT_NO_PDF
        TRACKEDITOR_ENABLE_FBX=$<BOOL:${TRACKEDITOR_ENABLE_FBX}>
        $<$<PLATFORM_ID:Windows>:IS_WINDOWS>
        $<$<CXX_COMPILER_ID:MSVC>:_CRT_SECURE_NO_WARNINGS>)

target_include_directories(TrackEditor PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${PROJECT_SOURCE_DIR}/WhipLib"
    "${PROJECT_SOURCE_DIR}/external/glm")

target_link_libraries(TrackEditor
    PRIVATE
        TrackEditor::WhipLib
        ROLLER::core
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        Qt5::OpenGL
        GLEW::GLEW
        OpenGL::GL)

add_custom_command(TARGET TrackEditor POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E make_directory
            "$<TARGET_FILE_DIR:TrackEditor>/Shaders"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${PROJECT_SOURCE_DIR}/WhipLib/Shaders/WhiplashVertexShader.glsl"
            "${PROJECT_SOURCE_DIR}/WhipLib/Shaders/WhiplashFragmentShader.glsl"
            "$<TARGET_FILE_DIR:TrackEditor>/Shaders"
    COMMENT "Copying TrackEditor runtime shaders")
