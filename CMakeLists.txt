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

if(NOT TARGET ROLLER::core)
    message(FATAL_ERROR "The ROLLER submodule did not provide ROLLER::core")
endif()

if(TRACKEDITOR_BUILD_APPLICATION)
    find_package(Qt5 5.15 REQUIRED COMPONENTS Core Gui Widgets OpenGL)
    find_package(OpenGL REQUIRED)
    find_package(GLEW 2.1 REQUIRED)

    add_subdirectory(WhipLib)
    add_subdirectory(TrackEditor)
endif()

include(CTest)
if(BUILD_TESTING)
    find_package(Python3 COMPONENTS Interpreter QUIET)
    if(Python3_Interpreter_FOUND)
        add_test(
            NAME trackeditor-e2-s1-roller-submodule
            COMMAND "${Python3_EXECUTABLE}"
                    "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_e2_s1_roller_submodule.py"
                    -v)
    endif()

    if(TARGET TrackEditor)
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
