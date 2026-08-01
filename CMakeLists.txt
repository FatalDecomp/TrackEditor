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
add_subdirectory(TrackModel)

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
