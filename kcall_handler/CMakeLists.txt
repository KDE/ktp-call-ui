include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})

set(kcall_handler_SRCS
    main.cpp
    callwindow.cpp
    calllog.cpp
    callhandler.cpp
    dtmfhandler.cpp
    dtmfwidget.cpp
    volumewidget.cpp
    videobalancewidget.cpp
)

kde4_add_ui_files(kcall_handler_SRCS callwindow.ui dtmfwidget.ui)
kde4_add_kcfg_files(kcall_handler_SRCS kcallhandlersettings.kcfgc)
kde4_add_executable(kcall_handler ${kcall_handler_SRCS})
target_link_libraries(kcall_handler ${KDE4_KDEUI_LIBS} ${TELEPATHY_QT4_LIBRARIES}
                                    telepathy-kde-call)

install(TARGETS kcall_handler ${INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES callwindowui.rc DESTINATION ${DATA_INSTALL_DIR}/kcall_handler)

configure_file(org.freedesktop.Telepathy.Client.kcall_handler.service.in
                org.freedesktop.Telepathy.Client.kcall_handler.service)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/org.freedesktop.Telepathy.Client.kcall_handler.service
        DESTINATION ${DBUS_SERVICES_INSTALL_DIR})

# FIXME any better macro for ${XDG_DATA_DIRS} ?
install(FILES kcall_handler.client DESTINATION ${CMAKE_INSTALL_PREFIX}/share/telepathy/clients/)
