# E5-S2. The editor's Qt module set is Core, Gui, and Widgets. The viewport is
# a plain QWidget that blits an immutable QImage (E3-S1) and E3-S4 deleted the
# WhipLib engine, every shader, and GLEW, so nothing in this build graph may ask
# for Qt OpenGL, desktop OpenGL, or GLEW.
#
# A source-text grep cannot see a transitive link edge, so this walks the real
# closure at configure time. It descends only into targets this project defines
# and checks imported targets by name alone: whether Qt's own Gui module uses
# OpenGL internally is Qt's business, and crawling into it would fail the build
# for something that is not this project's contract.
#
# This is the one file in the repository allowed to name these libraries,
# because its whole job is to forbid them. E3-S4's OpenGL audit exempts it by
# name and tests/test_e5_s2_module_set.py pins what it contains.

set(TRACKEDITOR_FORBIDDEN_LINK_PATTERNS
    "Qt[0-9]*::OpenGL"
    "OpenGL::"
    "GLEW::"
    "[Gg][Ll][Ee][Ww]")

function(trackeditor_assert_no_opengl_in_link_closure root)
    set(pending "${root}")
    set(visited "")

    while(pending)
        list(POP_FRONT pending current)
        if(current IN_LIST visited)
            continue()
        endif()
        list(APPEND visited "${current}")

        foreach(pattern IN LISTS TRACKEDITOR_FORBIDDEN_LINK_PATTERNS)
            if(current MATCHES "${pattern}")
                message(FATAL_ERROR
                    "E5-S2: target '${root}' reaches '${current}' through its link "
                    "closure. The editor links Qt Core/Gui/Widgets and no OpenGL.")
            endif()
        endforeach()

        if(NOT TARGET ${current})
            continue()
        endif()

        get_target_property(aliased ${current} ALIASED_TARGET)
        if(aliased)
            list(APPEND pending "${aliased}")
            continue()
        endif()

        get_target_property(imported ${current} IMPORTED)
        if(imported)
            continue()
        endif()

        foreach(property LINK_LIBRARIES INTERFACE_LINK_LIBRARIES)
            get_target_property(dependencies ${current} ${property})
            if(dependencies)
                list(APPEND pending ${dependencies})
            endif()
        endforeach()
    endwhile()
endfunction()
