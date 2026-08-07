# Vendored cgltf, pinned at v1.15 (360db1a95480fe102ae9c69b27c5d101167ff5ba).
# Header-only upstream; this target owns the one translation unit that
# instantiates it, so every consumer just links TrackEditor::cgltf.
add_library(cgltf STATIC cgltf_impl.c)
add_library(TrackEditor::cgltf ALIAS cgltf)

target_include_directories(cgltf PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

# Third-party code: do not hold it to this project's warning settings.
if(MSVC)
    target_compile_options(cgltf PRIVATE /W0)
else()
    target_compile_options(cgltf PRIVATE -w)
endif()
