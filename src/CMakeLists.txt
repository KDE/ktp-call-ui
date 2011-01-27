include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})

set(telepathy-kde-call-ui_SRCS
    main.cpp
    callwindow.cpp
    calllog.cpp
    callhandler.cpp
    dtmfhandler.cpp
    dtmfwidget.cpp
    volumewidget.cpp
    videobalancewidget.cpp
)

kde4_add_ui_files(telepathy-kde-call-ui_SRCS callwindow.ui dtmfwidget.ui)
kde4_add_kcfg_files(telepathy-kde-call-ui_SRCS kcallhandlersettings.kcfgc)
kde4_add_executable(telepathy-kde-call-ui ${telepathy-kde-call-ui_SRCS})
target_link_libraries(telepathy-kde-call-ui ${KDE4_KDEUI_LIBS} ${TELEPATHY_QT4_LIBRARIES}
                                    telepathy-kde-call)

install(TARGETS telepathy-kde-call-ui ${INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES callwindowui.rc DESTINATION ${DATA_INSTALL_DIR}/telepathy-kde-call-ui)

configure_file(org.freedesktop.Telepathy.Client.telepathy_kde_call_ui.service.in
                org.freedesktop.Telepathy.Client.telepathy_kde_call_ui.service)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/org.freedesktop.Telepathy.Client.telepathy_kde_call_ui.service
        DESTINATION ${DBUS_SERVICES_INSTALL_DIR})

# FIXME any better macro for ${XDG_DATA_DIRS} ?
install(FILES telepathy_kde_call_ui.client DESTINATION ${CMAKE_INSTALL_PREFIX}/share/telepathy/clients/)
