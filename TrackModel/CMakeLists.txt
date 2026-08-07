add_library(track-model STATIC
    TrackModel.cpp
    TrackModel.h
    Logging.cpp
    Logging.h
    Unmangler.cpp
    Unmangler.h)
add_library(TrackEditor::track-model ALIAS track-model)

target_compile_features(track-model PUBLIC cxx_std_17)
trackeditor_set_warnings(track-model)
target_compile_definitions(track-model
    PRIVATE
        $<$<PLATFORM_ID:Windows>:IS_WINDOWS>
        $<$<CXX_COMPILER_ID:MSVC>:_CRT_SECURE_NO_WARNINGS>)
target_include_directories(track-model
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}")

set_target_properties(track-model PROPERTIES
    DEBUG_POSTFIX d
    POSITION_INDEPENDENT_CODE ON)
