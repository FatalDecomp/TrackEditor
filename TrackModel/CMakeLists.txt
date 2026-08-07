add_library(track-model STATIC
    TrackModel.cpp
    TrackModel.h
    "${PROJECT_SOURCE_DIR}/WhipLib/Logging.cpp"
    "${PROJECT_SOURCE_DIR}/WhipLib/Unmangler.cpp")
add_library(TrackEditor::track-model ALIAS track-model)

target_compile_features(track-model PUBLIC cxx_std_17)
trackeditor_set_warnings(track-model)
target_compile_definitions(track-model
    PRIVATE
        $<$<PLATFORM_ID:Windows>:IS_WINDOWS>
        $<$<CXX_COMPILER_ID:MSVC>:_CRT_SECURE_NO_WARNINGS>)
target_include_directories(track-model
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}"
    PRIVATE
        "${PROJECT_SOURCE_DIR}/WhipLib")

set_target_properties(track-model PROPERTIES
    DEBUG_POSTFIX d
    POSITION_INDEPENDENT_CODE ON)
