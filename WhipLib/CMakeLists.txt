set(whiplib_sources
    CarHelpers.cpp
    DisasmHelpers.cpp
    MathHelpers.cpp
    ObjExporter.cpp
    ObjImporter.cpp
    Palette.cpp
    ShapeData.cpp
    ShapeFactory.cpp
    Texture.cpp
    Track.cpp
    WhipLib.cpp)

if(TRACKEDITOR_ENABLE_FBX)
    list(APPEND whiplib_sources FBXExporter.cpp)
endif()

add_library(WhipLib STATIC ${whiplib_sources})
add_library(TrackEditor::WhipLib ALIAS WhipLib)

target_compile_features(WhipLib PUBLIC cxx_std_17)
target_compile_definitions(WhipLib
    PUBLIC
        GLM_ENABLE_EXPERIMENTAL
        TRACKEDITOR_ENABLE_FBX=$<BOOL:${TRACKEDITOR_ENABLE_FBX}>
    PRIVATE
        WHIPLIB_LIB
        $<$<PLATFORM_ID:Windows>:IS_WINDOWS>
        $<$<CXX_COMPILER_ID:MSVC>:_CRT_SECURE_NO_WARNINGS>)

target_include_directories(WhipLib
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${PROJECT_SOURCE_DIR}/external/glm"
    PRIVATE
        "${PROJECT_SOURCE_DIR}/external/stb"
        "${CMAKE_CURRENT_SOURCE_DIR}/CarPlans"
        "${CMAKE_CURRENT_SOURCE_DIR}/SignPlans")

target_link_libraries(WhipLib PUBLIC TrackEditor::track-model)

if(WIN32)
    target_link_libraries(WhipLib PRIVATE Shlwapi)
endif()

if(TRACKEDITOR_ENABLE_FBX)
    include("${PROJECT_SOURCE_DIR}/cmake/TrackEditorFBX.cmake")
    trackeditor_enable_fbx(WhipLib)
endif()

set_target_properties(WhipLib PROPERTIES
    DEBUG_POSTFIX d
    POSITION_INDEPENDENT_CODE ON)
