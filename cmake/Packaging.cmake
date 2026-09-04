# CPack + extra install files (desktop entry / icon). Runtime Qt deploy
# lives on the Editerako target (qt_generate_deploy_app_script).

include(GNUInstallDirs)

if(UNIX AND NOT APPLE)
    install(FILES "${PROJECT_SOURCE_DIR}/packaging/editerako.desktop"
        DESTINATION "${CMAKE_INSTALL_DATADIR}/applications")
    install(FILES "${PROJECT_SOURCE_DIR}/packaging/editerako.svg"
        DESTINATION "${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps")
endif()

set(CPACK_PACKAGE_NAME "Editerako")
set(CPACK_PACKAGE_VENDOR "Editerako")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Editerako — C++ / Qt 6 code editor")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_INSTALL_DIRECTORY "Editerako")
set(CPACK_PACKAGE_FILE_NAME "Editerako-${PROJECT_VERSION}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")
set(CPACK_VERBATIM_VARIABLES ON)

if(WIN32)
    set(CPACK_GENERATOR "ZIP")
elseif(APPLE)
    set(CPACK_GENERATOR "DragNDrop")
else()
    set(CPACK_GENERATOR "TGZ")
endif()

include(CPack)
