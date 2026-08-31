# Qt < 6.8.4 / < 6.9.2 still links the AGL framework. Apple removed it from
# the macOS 26 / Xcode 26 SDK, which breaks the linker (ld: framework 'AGL'
# not found). An empty imported target intercepts the "AGL" link item; leftover
# -framework AGL flags are stripped from Qt imported targets after find_package.

function(editerako_strip_agl_from_qt)
    if(NOT APPLE)
        return()
    endif()

    foreach(_tgt IN ITEMS
            Qt6::Gui
            Qt6::GuiPrivate
            Qt6::Widgets
            Qt6::OpenGL
            Qt6::OpenGLWidgets
            Qt6::Pdf
            Qt6::PdfWidgets
            WrapOpenGL::WrapOpenGL)
        if(NOT TARGET "${_tgt}")
            continue()
        endif()
        foreach(_prop IN ITEMS INTERFACE_LINK_LIBRARIES INTERFACE_LINK_OPTIONS)
            get_target_property(_items "${_tgt}" ${_prop})
            if(NOT _items)
                continue()
            endif()
            set(_out)
            set(_pending "")
            foreach(_item IN LISTS _items)
                if(_pending STREQUAL "-framework")
                    set(_pending "")
                    if(_item STREQUAL "AGL")
                        continue()
                    endif()
                    list(APPEND _out "-framework" "${_item}")
                    continue()
                endif()
                if(_item STREQUAL "-framework")
                    set(_pending "-framework")
                    continue()
                endif()
                if(_item MATCHES "AGL")
                    continue()
                endif()
                list(APPEND _out "${_item}")
            endforeach()
            if(_pending STREQUAL "-framework")
                list(APPEND _out "-framework")
            endif()
            set_property(TARGET "${_tgt}" PROPERTY ${_prop} "${_out}")
        endforeach()
    endforeach()
endfunction()

if(APPLE AND NOT TARGET AGL)
    add_library(AGL INTERFACE IMPORTED)
endif()
