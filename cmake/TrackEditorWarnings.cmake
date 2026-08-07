# E5-S3. "Builds warning-clean against Qt 6.8" needs a warning level to be clean
# at, and this project had none: `cmake_minimum_required(VERSION 3.24)` makes
# CMP0092 NEW, which removes CMake's historical `/W3` from the default MSVC
# flags, so every target was compiling at MSVC's default `/W1`.
#
# `trackeditor_set_warnings()` raises the targets this repository owns — never
# the ROLLER submodule or vendored cgltf, which are not this project's code to
# clean.

option(TRACKEDITOR_WARNINGS_AS_ERRORS
       "Treat compiler warnings in TrackEditor's own targets as errors" OFF)

# The Qt version whose deprecated API is compiled out entirely. This is the
# cross-platform half of the story: it is enforced by Qt's own headers rather
# than by a compiler's warning set, so it holds identically on MSVC, GCC, and
# Clang. Using anything Qt deprecated up to 6.8 is a compile error, not a
# warning that a different compiler might not emit.
set(TRACKEDITOR_QT_DISABLE_DEPRECATED_UP_TO 0x060800)

function(trackeditor_set_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4)
        if(TRACKEDITOR_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE /WX)
        endif()
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra)
        if(TRACKEDITOR_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE -Werror)
        endif()
    endif()
endfunction()

# Applied to every target that compiles Qt headers.
function(trackeditor_gate_qt_deprecations target)
    target_compile_definitions(${target} PRIVATE
        QT_DISABLE_DEPRECATED_UP_TO=${TRACKEDITOR_QT_DISABLE_DEPRECATED_UP_TO})
endfunction()
