cmake_minimum_required(VERSION 3.24...4.0)

project(RollerTrackEditor VERSION 0.1.0 LANGUAGES C CXX)

option(TRACKEDITOR_BUILD_APPLICATION "Build the Qt TrackEditor application" ON)

# Included before the first add_subdirectory so every target this repository
# owns can reach trackeditor_set_warnings() and the Qt deprecation gate.
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(TrackEditorNoOpenGL)
include(TrackEditorWarnings)

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
    find_package(Qt6 6.8 REQUIRED COMPONENTS Core Gui Widgets)

    add_subdirectory(TrackEditor)

    trackeditor_assert_no_opengl_in_link_closure(TrackEditor)
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

    add_executable(trackeditor-e7-s5-tower-model-test
        tests/editor_tower_model_test.cpp
        TrackEditor/EditorTowerModel.cpp
        TrackEditor/EditorTowerModel.h)
    target_compile_features(trackeditor-e7-s5-tower-model-test PRIVATE cxx_std_17)
    target_include_directories(trackeditor-e7-s5-tower-model-test PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/TrackEditor")
    target_link_libraries(trackeditor-e7-s5-tower-model-test PRIVATE
        TrackEditor::track-model)
    add_test(NAME trackeditor-e7-s5-tower-model
             COMMAND trackeditor-e7-s5-tower-model-test)

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
        add_test(
            NAME trackeditor-e4-s3-fbx-removal-contract
            COMMAND "${Python3_EXECUTABLE}"
                    "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_e4_s3_fbx_removal.py"
                    -v)
        add_test(
            NAME trackeditor-e4-s4-texture-output-contract
            COMMAND "${Python3_EXECUTABLE}"
                    "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_e4_s4_texture_output.py"
                    -v)
        add_test(
            NAME trackeditor-e4-s5-export-ui-contract
            COMMAND "${Python3_EXECUTABLE}"
                    "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_e4_s5_export_ui.py"
                    -v)
        add_test(
            NAME trackeditor-batch-export-contract
            COMMAND "${Python3_EXECUTABLE}"
                    "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_batch_export.py"
                    -v)
        add_test(
            NAME trackeditor-e4-s6-reference-import-contract
            COMMAND "${Python3_EXECUTABLE}"
                    "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_e4_s6_reference_import.py"
                    -v)
        add_test(
            NAME trackeditor-e4a-s6-scenery-export-contract
            COMMAND "${Python3_EXECUTABLE}"
                    "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_e4a_s6_scenery_export.py"
                    -v)
        add_test(
            NAME trackeditor-e5-s1-qt6-provisioning-contract
            COMMAND "${Python3_EXECUTABLE}"
                    "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_e5_s1_qt6_provisioning.py"
                    -v)
        add_test(
            NAME trackeditor-e5-s2-module-set-contract
            COMMAND "${Python3_EXECUTABLE}"
                    "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_e5_s2_module_set.py"
                    -v)
        add_test(
            NAME trackeditor-e5-s3-api-migrations-contract
            COMMAND "${Python3_EXECUTABLE}"
                    "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_e5_s3_api_migrations.py"
                    -v)
        add_test(
            NAME trackeditor-e6-s1-editor-build-workflow-contract
            COMMAND "${Python3_EXECUTABLE}"
                    "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_e6_s1_editor_build_workflow.py"
                    -v)
        add_test(
            NAME trackeditor-e6-s2-deploy-artifacts-contract
            COMMAND "${Python3_EXECUTABLE}"
                    "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_e6_s2_deploy_artifacts.py"
                    -v)
        add_test(
            NAME trackeditor-e6-s3-releases-contract
            COMMAND "${Python3_EXECUTABLE}"
                    "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_e6_s3_releases.py"
                    -v)
        add_test(
            NAME trackeditor-e7-s4-tower-display-contract
            COMMAND "${Python3_EXECUTABLE}"
                    "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_e7_s4_tower_display.py"
                    -v)
        add_test(
            NAME trackeditor-e7-s5-edit-towers-contract
            COMMAND "${Python3_EXECUTABLE}"
                    "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_e7_s5_edit_towers.py"
                    -v)
        add_test(
            NAME trackeditor-graphics-settings-contract
            COMMAND "${Python3_EXECUTABLE}"
                    "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_graphics_settings.py"
                    -v)
        add_test(
            NAME trackeditor-animated-stunts-contract
            COMMAND "${Python3_EXECUTABLE}"
                    "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_animated_stunts.py"
                    -v)
    endif()

    if(TARGET TrackEditor)
        add_executable(trackeditor-e3-s1-frame-delivery-test
            tests/editor_render_queue_test.cpp
            TrackEditor/EditorRenderQueue.cpp)
        target_compile_features(trackeditor-e3-s1-frame-delivery-test PRIVATE cxx_std_17)
        target_include_directories(trackeditor-e3-s1-frame-delivery-test PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/TrackEditor"
            "${CMAKE_CURRENT_SOURCE_DIR}/external/ROLLER/PROJECTS/ROLLER")
        target_link_libraries(trackeditor-e3-s1-frame-delivery-test PRIVATE
            Qt6::Core
            Qt6::Gui)
        trackeditor_assert_no_opengl_in_link_closure(
            trackeditor-e3-s1-frame-delivery-test)
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
            Qt6::Core
            Qt6::Gui)
        trackeditor_assert_no_opengl_in_link_closure(
            trackeditor-e3-s1-render-service-test)
        add_test(NAME trackeditor-e3-s1-render-service
                 COMMAND trackeditor-e3-s1-render-service-test)

        add_executable(trackeditor-e3a-s2-overlay-settings-test
            tests/editor_overlay_settings_test.cpp
            TrackEditor/EditorOverlaySettings.cpp
            TrackEditor/EditorReferenceMesh.cpp
            TrackEditor/EditorOverlaySettings.h)
        target_compile_features(trackeditor-e3a-s2-overlay-settings-test PRIVATE cxx_std_17)
        # Types.h (the eWhipModel and eShapeSection enums E3A-S6 translates) is
        # an editor header; the target still links no Qt or ROLLER code, so the
        # translation stays unit-testable without a render worker or an event
        # loop.
        target_include_directories(trackeditor-e3a-s2-overlay-settings-test PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/TrackEditor"
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
            "${CMAKE_CURRENT_SOURCE_DIR}/external/ROLLER/PROJECTS/ROLLER")
        add_test(NAME trackeditor-e3-s3-camera-input
                 COMMAND trackeditor-e3-s3-camera-input-test)

        # E4-S1. The OBJ exporter reads ROLLER's canonical geometry and writes
        # text, so it needs the editor_api.h types on the include path and
        # nothing else: no Qt, no linked ROLLER code.
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

        # E4-S5. The export format table and file-name rules: no Qt, so the
        # save-dialog behaviour is exercised without a dialog.
        add_executable(trackeditor-e4-s5-export-format-test
            tests/editor_export_format_test.cpp
            TrackEditor/EditorExportFormat.cpp
            TrackEditor/EditorExportFormat.h)
        target_compile_features(trackeditor-e4-s5-export-format-test PRIVATE cxx_std_17)
        target_include_directories(trackeditor-e4-s5-export-format-test PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/TrackEditor")
        add_test(NAME trackeditor-e4-s5-export-format
                 COMMAND trackeditor-e4-s5-export-format-test)

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
        target_compile_definitions(trackeditor-e4-s2-gltf-exporter-test PRIVATE
            $<$<CXX_COMPILER_ID:MSVC>:_CRT_SECURE_NO_WARNINGS>)
        target_include_directories(trackeditor-e4-s2-gltf-exporter-test PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/TrackEditor"
            "${CMAKE_CURRENT_SOURCE_DIR}/external/ROLLER/PROJECTS/ROLLER")
        target_link_libraries(trackeditor-e4-s2-gltf-exporter-test PRIVATE
            TrackEditor::cgltf)
        add_test(NAME trackeditor-e4-s2-gltf-exporter
                 COMMAND trackeditor-e4-s2-gltf-exporter-test
                 WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

        # Batch car export adapts ROLLER's immutable CarDesigns plans to the
        # same canonical geometry vocabulary used by both file writers.
        add_executable(trackeditor-batch-car-model-test
            tests/editor_car_model_test.cpp
            TrackEditor/EditorCarModel.cpp
            TrackEditor/EditorCarModel.h
            TrackEditor/EditorExportCommon.cpp
            TrackEditor/EditorExportCommon.h)
        target_compile_features(trackeditor-batch-car-model-test PRIVATE cxx_std_17)
        target_include_directories(trackeditor-batch-car-model-test PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/TrackEditor"
            "${CMAKE_CURRENT_SOURCE_DIR}/external/ROLLER/PROJECTS/ROLLER")
        target_link_libraries(trackeditor-batch-car-model-test PRIVATE
            ROLLER::core)
        add_test(NAME trackeditor-batch-car-model
                 COMMAND trackeditor-batch-car-model-test)

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

        # E5-S2 acceptance: the criterion is about the linked artifact, not the
        # build files, so read the module names out of the executable itself.
        if(Python3_Interpreter_FOUND)
            add_test(
                NAME trackeditor-e5-s2-linked-qt-modules
                COMMAND "${Python3_EXECUTABLE}"
                        "${CMAKE_CURRENT_SOURCE_DIR}/tests/check_linked_qt_modules.py"
                        "$<TARGET_FILE:TrackEditor>")
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

    # E5-S3. Test code is code: the executables this repository owns build at
    # the same raised warning level as the libraries they exercise, and the two
    # that compile Qt headers take the same deprecation gate.
    foreach(trackeditor_test_target IN ITEMS
            trackeditor-e3-s5a-track-model-test
            trackeditor-e3-s5b-track-assets-test
            trackeditor-e7-s5-tower-model-test
            trackeditor-e3-s1-frame-delivery-test
            trackeditor-e3-s1-render-service-test
            trackeditor-e3a-s2-overlay-settings-test
            trackeditor-e3-s3-camera-input-test
            trackeditor-e4-s1-obj-exporter-test
            trackeditor-e4-s5-export-format-test
            trackeditor-e4-s2-gltf-exporter-test
            trackeditor-batch-car-model-test)
        if(TARGET ${trackeditor_test_target})
            trackeditor_set_warnings(${trackeditor_test_target})
        endif()
    endforeach()

    foreach(trackeditor_qt_test_target IN ITEMS
            trackeditor-e3-s1-frame-delivery-test
            trackeditor-e3-s1-render-service-test)
        if(TARGET ${trackeditor_qt_test_target})
            trackeditor_gate_qt_deprecations(${trackeditor_qt_test_target})
        endif()
    endforeach()
endif()
