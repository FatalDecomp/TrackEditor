cmake_minimum_required(VERSION 3.24...4.0)

project(RollerTrackEditor VERSION 0.1.0 LANGUAGES C CXX)

option(TRACKEDITOR_BUILD_APPLICATION "Build the Qt TrackEditor application" ON)
option(TRACKEDITOR_ENABLE_FBX "Enable FBX export with a locally installed Autodesk FBX SDK" OFF)

set(ROLLER_BUILD_GAME OFF CACHE BOOL "Build the ROLLER game" FORCE)
set(ROLLER_BUILD_EDITOR_CORE ON CACHE BOOL "Build roller-core for TrackEditor" FORCE)

if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/ROLLER/CMakeLists.txt")
    message(FATAL_ERROR
        "The ROLLER submodule is missing. Run: git submodule update --init --recursive")
endif()

add_subdirectory(external/ROLLER)
add_subdirectory(external/cgltf)
add_subdirectory(TrackModel)
add_subdirectory(TrackAssets)

if(NOT TARGET ROLLER::core)
    message(FATAL_ERROR "The ROLLER submodule did not provide ROLLER::core")
endif()

if(TRACKEDITOR_BUILD_APPLICATION)
    find_package(Qt5 5.15 REQUIRED COMPONENTS Core Gui Widgets)

    add_subdirectory(WhipLib)
    add_subdirectory(TrackEditor)
endif()

include(CTest)
if(BUILD_TESTING)
    add_executable(trackeditor-e3-s5a-track-model-test
        tests/track_model_test.cpp)
    target_compile_features(trackeditor-e3-s5a-track-model-test PRIVATE cxx_std_17)
    target_link_libraries(trackeditor-e3-s5a-track-model-test PRIVATE
        TrackEditor::track-model)
    add_test(NAME trackeditor-e3-s5a-track-model
             COMMAND trackeditor-e3-s5a-track-model-test)

    add_executable(trackeditor-e3-s5b-track-assets-test
        tests/track_assets_test.cpp)
    target_compile_features(trackeditor-e3-s5b-track-assets-test PRIVATE cxx_std_17)
    target_link_libraries(trackeditor-e3-s5b-track-assets-test PRIVATE
        TrackEditor::track-assets)
    add_test(NAME trackeditor-e3-s5b-track-assets
             COMMAND trackeditor-e3-s5b-track-assets-test)

    find_package(Python3 COMPONENTS Interpreter QUIET)
    if(Python3_Interpreter_FOUND)
        add_test(
            NAME trackeditor-e2-s1-roller-submodule
            COMMAND "${Python3_EXECUTABLE}"
                    "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_e2_s1_roller_submodule.py"
                    -v)
        add_test(
            NAME trackeditor-e3-s1-render-worker-contract
            COMMAND "${Python3_EXECUTABLE}"
                    "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_e3_s1_render_worker.py"
                    -v)
        add_test(
            NAME trackeditor-e3-s2-edit-display-contract
            COMMAND "${Python3_EXECUTABLE}"
                    "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_e3_s2_edit_display.py"
                    -v)
        add_test(
            NAME trackeditor-e3-s3-camera-input-contract
            COMMAND "${Python3_EXECUTABLE}"
                    "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_e3_s3_camera_input.py"
                    -v)
        add_test(
            NAME trackeditor-e3-s4-whiplib-engine-contract
            COMMAND "${Python3_EXECUTABLE}"
                    "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_e3_s4_whiplib_engine.py"
                    -v)
        add_test(
            NAME trackeditor-e3-s5a-track-model-contract
            COMMAND "${Python3_EXECUTABLE}"
                    "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_e3_s5a_track_model.py"
                    -v)
        add_test(
            NAME trackeditor-e3-s5b-track-assets-contract
            COMMAND "${Python3_EXECUTABLE}"
                    "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_e3_s5b_track_assets.py"
                    -v)
        add_test(
            NAME trackeditor-e3-s5c-geometry-cleanup-contract
            COMMAND "${Python3_EXECUTABLE}"
                    "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_e3_s5c_geometry_cleanup.py"
                    -v)
        add_test(
            NAME trackeditor-e3-s6-multi-document-copy-paste-contract
            COMMAND "${Python3_EXECUTABLE}"
                    "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_e3_s6_multi_document_copy_paste.py"
                    -v)
        add_test(
            NAME trackeditor-e3a-s2-overlay-toggles-contract
            COMMAND "${Python3_EXECUTABLE}"
                    "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_e3a_s2_overlay_toggles.py"
                    -v)
        add_test(
            NAME trackeditor-e3a-s3-selection-highlight-contract
            COMMAND "${Python3_EXECUTABLE}"
                    "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_e3a_s3_selection_highlight.py"
                    -v)
        add_test(
            NAME trackeditor-e3a-s4-helper-overlays-contract
            COMMAND "${Python3_EXECUTABLE}"
                    "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_e3a_s4_helper_overlays.py"
                    -v)
        add_test(
            NAME trackeditor-e3a-s5-markers-contract
            COMMAND "${Python3_EXECUTABLE}"
                    "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_e3a_s5_markers.py"
                    -v)
        add_test(
            NAME trackeditor-e3a-s6-test-car-contract
            COMMAND "${Python3_EXECUTABLE}"
                    "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_e3a_s6_test_car.py"
                    -v)
        add_test(
            NAME trackeditor-e3a-s7-reference-mesh-contract
            COMMAND "${Python3_EXECUTABLE}"
                    "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_e3a_s7_reference_mesh.py"
                    -v)
        add_test(
            NAME trackeditor-e4-s1-obj-export-contract
            COMMAND "${Python3_EXECUTABLE}"
                    "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_e4_s1_obj_export.py"
                    -v)
        add_test(
            NAME trackeditor-e4-s2-gltf-export-contract
            COMMAND "${Python3_EXECUTABLE}"
                    "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_e4_s2_gltf_export.py"
                    -v)
    endif()

    if(TARGET TrackEditor)
        add_executable(trackeditor-e3-s5c-track-geometry-test
            tests/track_geometry_test.cpp)
        target_compile_features(trackeditor-e3-s5c-track-geometry-test PRIVATE cxx_std_17)
        target_link_libraries(trackeditor-e3-s5c-track-geometry-test PRIVATE
            TrackEditor::WhipLib)
        add_test(NAME trackeditor-e3-s5c-track-geometry
                 COMMAND trackeditor-e3-s5c-track-geometry-test)

        add_executable(trackeditor-e3-s1-frame-delivery-test
            tests/editor_render_queue_test.cpp
            TrackEditor/EditorRenderQueue.cpp)
        target_compile_features(trackeditor-e3-s1-frame-delivery-test PRIVATE cxx_std_17)
        target_include_directories(trackeditor-e3-s1-frame-delivery-test PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/TrackEditor"
            "${CMAKE_CURRENT_SOURCE_DIR}/external/ROLLER/PROJECTS/ROLLER")
        target_link_libraries(trackeditor-e3-s1-frame-delivery-test PRIVATE
            Qt5::Core
            Qt5::Gui)
        add_test(NAME trackeditor-e3-s1-frame-delivery
                 COMMAND trackeditor-e3-s1-frame-delivery-test)

        add_executable(trackeditor-e3-s1-render-service-test
            tests/editor_render_service_test.cpp
            TrackEditor/EditorRenderQueue.cpp
            TrackEditor/EditorRenderQueue.h
            TrackEditor/EditorRenderService.cpp
            TrackEditor/EditorRenderService.h)
        set_target_properties(trackeditor-e3-s1-render-service-test PROPERTIES
            AUTOMOC ON)
        target_compile_features(trackeditor-e3-s1-render-service-test PRIVATE cxx_std_17)
        target_include_directories(trackeditor-e3-s1-render-service-test PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/TrackEditor"
            "${CMAKE_CURRENT_SOURCE_DIR}/external/ROLLER/PROJECTS/ROLLER")
        target_link_libraries(trackeditor-e3-s1-render-service-test PRIVATE
            Qt5::Core
            Qt5::Gui)
        add_test(NAME trackeditor-e3-s1-render-service
                 COMMAND trackeditor-e3-s1-render-service-test)

        add_executable(trackeditor-e3a-s2-overlay-settings-test
            tests/editor_overlay_settings_test.cpp
            TrackEditor/EditorOverlaySettings.cpp
            TrackEditor/EditorReferenceMesh.cpp
            TrackEditor/EditorOverlaySettings.h)
        target_compile_features(trackeditor-e3a-s2-overlay-settings-test PRIVATE cxx_std_17)
        # WhipLib is on the include path for Types.h alone (the eWhipModel and
        # eShapeSection enums E3A-S6 translates); the target still links no
        # WhipLib, Qt, or ROLLER code, so the translation stays unit-testable
        # without a render worker or an event loop.
        target_include_directories(trackeditor-e3a-s2-overlay-settings-test PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/TrackEditor"
            "${CMAKE_CURRENT_SOURCE_DIR}/WhipLib"
            "${CMAKE_CURRENT_SOURCE_DIR}/external/ROLLER/PROJECTS/ROLLER")
        add_test(NAME trackeditor-e3a-s2-overlay-settings
                 COMMAND trackeditor-e3a-s2-overlay-settings-test)

        add_executable(trackeditor-e3-s3-camera-input-test
            tests/editor_camera_controller_test.cpp
            TrackEditor/EditorCameraController.cpp
            TrackEditor/EditorCameraController.h)
        target_compile_features(trackeditor-e3-s3-camera-input-test PRIVATE cxx_std_17)
        target_include_directories(trackeditor-e3-s3-camera-input-test PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/TrackEditor"
            "${CMAKE_CURRENT_SOURCE_DIR}/external/glm"
            "${CMAKE_CURRENT_SOURCE_DIR}/external/ROLLER/PROJECTS/ROLLER")
        add_test(NAME trackeditor-e3-s3-camera-input
                 COMMAND trackeditor-e3-s3-camera-input-test)

        # E4-S1. The OBJ exporter reads ROLLER's canonical geometry and writes
        # text, so it needs the editor_api.h types on the include path and
        # nothing else: no Qt, no WhipLib, no linked ROLLER code.
        add_executable(trackeditor-e4-s1-obj-exporter-test
            tests/editor_obj_exporter_test.cpp
            TrackEditor/EditorExportCommon.cpp
            TrackEditor/EditorExportCommon.h
            TrackEditor/EditorObjExporter.cpp
            TrackEditor/EditorObjExporter.h)
        target_compile_features(trackeditor-e4-s1-obj-exporter-test PRIVATE cxx_std_17)
        target_include_directories(trackeditor-e4-s1-obj-exporter-test PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/TrackEditor"
            "${CMAKE_CURRENT_SOURCE_DIR}/external/ROLLER/PROJECTS/ROLLER")
        add_test(NAME trackeditor-e4-s1-obj-exporter
                 COMMAND trackeditor-e4-s1-obj-exporter-test)

        # E4-S2. Same shape, plus the vendored cgltf writer. It writes the
        # sample documents the optional Blender import check below reads, so it
        # runs from the binary directory.
        add_executable(trackeditor-e4-s2-gltf-exporter-test
            tests/editor_gltf_exporter_test.cpp
            TrackEditor/EditorExportCommon.cpp
            TrackEditor/EditorExportCommon.h
            TrackEditor/EditorGltfExporter.cpp
            TrackEditor/EditorGltfExporter.h)
        target_compile_features(trackeditor-e4-s2-gltf-exporter-test PRIVATE cxx_std_17)
        target_include_directories(trackeditor-e4-s2-gltf-exporter-test PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/TrackEditor"
            "${CMAKE_CURRENT_SOURCE_DIR}/external/ROLLER/PROJECTS/ROLLER")
        target_link_libraries(trackeditor-e4-s2-gltf-exporter-test PRIVATE
            TrackEditor::cgltf)
        add_test(NAME trackeditor-e4-s2-gltf-exporter
                 COMMAND trackeditor-e4-s2-gltf-exporter-test
                 WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

        # E4-S2 acceptance: the story's criterion is that the output imports
        # natively in Blender, so check it when a Blender is available. Hosted
        # CI has none, and the rest of the suite must not depend on one, so
        # this entry only exists when TRACKEDITOR_BLENDER_EXECUTABLE resolves.
        find_program(TRACKEDITOR_BLENDER_EXECUTABLE
            NAMES blender
            PATHS "$ENV{ProgramFiles}/Blender Foundation"
            PATH_SUFFIXES "Blender 4.2" "Blender 4.1" "Blender 4.0"
            DOC "Blender used by the optional E4-S2 glTF import check")
        if(TRACKEDITOR_BLENDER_EXECUTABLE AND Python3_Interpreter_FOUND)
            add_test(
                NAME trackeditor-e4-s2-gltf-blender-import
                COMMAND "${Python3_EXECUTABLE}"
                        "${CMAKE_CURRENT_SOURCE_DIR}/tests/check_gltf_in_blender.py"
                        "${TRACKEDITOR_BLENDER_EXECUTABLE}"
                        "${CMAKE_CURRENT_BINARY_DIR}")
            set_tests_properties(trackeditor-e4-s2-gltf-blender-import
                PROPERTIES
                    DEPENDS trackeditor-e4-s2-gltf-exporter
                    TIMEOUT 300)
        endif()

        add_test(NAME trackeditor-smoke COMMAND TrackEditor --cmake-smoke-test)
        if(WIN32)
            set(trackeditor_qpa_platform windows)
        else()
            set(trackeditor_qpa_platform offscreen)
        endif()
        set_tests_properties(trackeditor-smoke PROPERTIES
            ENVIRONMENT "QT_QPA_PLATFORM=${trackeditor_qpa_platform}"
            TIMEOUT 30)
    endif()
endif()
