add_library(track-assets STATIC
    Palette.cpp
    Palette.h
    Texture.cpp
    Texture.h
    TrackAssets.cpp
    TrackAssets.h)
add_library(TrackEditor::track-assets ALIAS track-assets)

target_compile_features(track-assets PUBLIC cxx_std_17)
trackeditor_set_warnings(track-assets)
target_compile_definitions(track-assets
    PRIVATE
        $<$<PLATFORM_ID:Windows>:IS_WINDOWS>
        $<$<CXX_COMPILER_ID:MSVC>:_CRT_SECURE_NO_WARNINGS>)
target_include_directories(track-assets
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}"
    PRIVATE
        "${PROJECT_SOURCE_DIR}/WhipLib"
        "${PROJECT_SOURCE_DIR}/external/stb")
target_link_libraries(track-assets PRIVATE TrackEditor::track-model)

set_target_properties(track-assets PROPERTIES
    DEBUG_POSTFIX d
    POSITION_INDEPENDENT_CODE ON)
