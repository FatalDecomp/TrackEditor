cmake_minimum_required(VERSION 3.24...4.0)

project(RollerTrackEditor VERSION 0.1.0 LANGUAGES C CXX)

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
endif()
