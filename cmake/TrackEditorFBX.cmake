set(TRACKEDITOR_FBX_SDK_ROOT "" CACHE PATH
    "Root of a local Autodesk FBX SDK installation (contains include/fbxsdk.h)")

function(trackeditor_enable_fbx target)
    if(NOT TRACKEDITOR_FBX_SDK_ROOT)
        message(FATAL_ERROR
            "TRACKEDITOR_ENABLE_FBX=ON requires TRACKEDITOR_FBX_SDK_ROOT to point "
            "to a local Autodesk FBX SDK installation")
    endif()

    set(fbx_include "${TRACKEDITOR_FBX_SDK_ROOT}/include")
    if(NOT EXISTS "${fbx_include}/fbxsdk.h")
        message(FATAL_ERROR
            "No include/fbxsdk.h found beneath TRACKEDITOR_FBX_SDK_ROOT: "
            "${TRACKEDITOR_FBX_SDK_ROOT}")
    endif()

    target_include_directories(${target} PRIVATE "${fbx_include}")
    target_compile_definitions(${target} PRIVATE K_PLUGIN K_FBXSDK K_NODLL)

    if(WIN32)
        set(fbx_library_root "${TRACKEDITOR_FBX_SDK_ROOT}/lib/x64")
        find_library(fbx_debug_library
            NAMES libfbxsdk-mt libfbxsdk
            PATHS "${fbx_library_root}/debug"
            NO_DEFAULT_PATH REQUIRED)
        find_library(fbx_release_library
            NAMES libfbxsdk-mt libfbxsdk
            PATHS "${fbx_library_root}/release"
            NO_DEFAULT_PATH REQUIRED)

        target_link_libraries(${target} PRIVATE
            "$<$<CONFIG:Debug>:${fbx_debug_library}>"
            "$<$<NOT:$<CONFIG:Debug>>:${fbx_release_library}>"
            wininet crypt32)

        foreach(fbx_support_library IN ITEMS libxml2-mt zlib-mt)
            find_library(${fbx_support_library}_debug
                NAMES ${fbx_support_library}
                PATHS "${fbx_library_root}/debug"
                NO_DEFAULT_PATH)
            find_library(${fbx_support_library}_release
                NAMES ${fbx_support_library}
                PATHS "${fbx_library_root}/release"
                NO_DEFAULT_PATH)
            if(${fbx_support_library}_debug AND ${fbx_support_library}_release)
                target_link_libraries(${target} PRIVATE
                    "$<$<CONFIG:Debug>:${${fbx_support_library}_debug}>"
                    "$<$<NOT:$<CONFIG:Debug>>:${${fbx_support_library}_release}>")
            endif()
        endforeach()
    else()
        find_library(fbx_library
            NAMES fbxsdk libfbxsdk
            PATHS
                "${TRACKEDITOR_FBX_SDK_ROOT}/lib"
                "${TRACKEDITOR_FBX_SDK_ROOT}/lib/clang/release"
                "${TRACKEDITOR_FBX_SDK_ROOT}/lib/gcc/x64/release"
            NO_DEFAULT_PATH REQUIRED)
        find_package(LibXml2 REQUIRED)
        find_package(ZLIB REQUIRED)
        target_link_libraries(${target} PRIVATE
            "${fbx_library}" LibXml2::LibXml2 ZLIB::ZLIB)
    endif()
endfunction()
