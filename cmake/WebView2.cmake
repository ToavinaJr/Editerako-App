# Optional Microsoft Edge WebView2 SDK (NuGet) for in-app account chat on Windows.
# Headers/DLL live in the build tree (*.dll is gitignored). MinGW loads the loader at runtime.

set(EDITERAKO_WEBVIEW2_VERSION "1.0.2903.40")
set(EDITERAKO_WEBVIEW2_ROOT "${CMAKE_BINARY_DIR}/_deps/webview2")
set(EDITERAKO_WEBVIEW2_HEADER "${EDITERAKO_WEBVIEW2_ROOT}/build/native/include/WebView2.h")
set(EDITERAKO_HAS_WEBVIEW2 OFF)

if(WIN32)
    if(NOT EXISTS "${EDITERAKO_WEBVIEW2_HEADER}")
        set(_nupkg "${EDITERAKO_WEBVIEW2_ROOT}/webview2.nupkg")
        file(MAKE_DIRECTORY "${EDITERAKO_WEBVIEW2_ROOT}")
        message(STATUS "Downloading WebView2 SDK ${EDITERAKO_WEBVIEW2_VERSION}")
        file(DOWNLOAD
            "https://www.nuget.org/api/v2/package/Microsoft.Web.WebView2/${EDITERAKO_WEBVIEW2_VERSION}"
            "${_nupkg}"
            STATUS _dl
            TIMEOUT 60
        )
        list(GET _dl 0 _dl_code)
        if(_dl_code EQUAL 0)
            file(ARCHIVE_EXTRACT INPUT "${_nupkg}" DESTINATION "${EDITERAKO_WEBVIEW2_ROOT}")
        else()
            message(WARNING "WebView2 SDK download failed (${_dl}) — account chat will use the system browser")
        endif()
    endif()
    if(EXISTS "${EDITERAKO_WEBVIEW2_HEADER}")
        set(EDITERAKO_HAS_WEBVIEW2 ON)
        if(CMAKE_SIZEOF_VOID_P EQUAL 8)
            set(EDITERAKO_WEBVIEW2_LOADER
                "${EDITERAKO_WEBVIEW2_ROOT}/runtimes/win-x64/native/WebView2Loader.dll")
        else()
            set(EDITERAKO_WEBVIEW2_LOADER
                "${EDITERAKO_WEBVIEW2_ROOT}/runtimes/win-x86/native/WebView2Loader.dll")
        endif()
        if(NOT EXISTS "${EDITERAKO_WEBVIEW2_LOADER}")
            set(EDITERAKO_WEBVIEW2_LOADER
                "${EDITERAKO_WEBVIEW2_ROOT}/build/native/x64/WebView2Loader.dll")
        endif()
    endif()
endif()

function(editerako_use_webview2 target)
    if(NOT EDITERAKO_HAS_WEBVIEW2)
        return()
    endif()
    target_include_directories(${target} SYSTEM PRIVATE "${EDITERAKO_WEBVIEW2_ROOT}/build/native/include")
    target_compile_definitions(${target} PRIVATE EDITERAKO_HAS_WEBVIEW2)
    target_link_libraries(${target} PUBLIC ole32 oleaut32 uuid)
endfunction()

function(editerako_deploy_webview2 target)
    if(NOT EDITERAKO_HAS_WEBVIEW2 OR NOT EXISTS "${EDITERAKO_WEBVIEW2_LOADER}")
        return()
    endif()
    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${EDITERAKO_WEBVIEW2_LOADER}"
            "$<TARGET_FILE_DIR:${target}>/WebView2Loader.dll"
        COMMENT "Copy WebView2Loader.dll next to ${target}"
    )
endfunction()
