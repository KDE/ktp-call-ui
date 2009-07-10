
include_directories(${GSTREAMER_INCLUDE_DIR} ${GLIB2_INCLUDE_DIR} ${DBUSGLIB_INCLUDE_DIR}
                    ${LIBXML2_INCLUDE_DIR} ${TPFARSIGHT_INCLUDE_DIR} ${CMAKE_CURRENT_BINARY_DIR}
                    ${CMAKE_CURRENT_SOURCE_DIR})

set(kcall_handler_SRCS
    main.cpp
    callwindow.cpp
    calllog.cpp
    callhandler.cpp
    channelhandler.cpp
    abstractmediahandler.cpp
    dtmfhandler.cpp
    volumewidget.cpp
    farsight/mediahandler.cpp
)

kde4_add_ui_files(kcall_handler_SRCS callwindow.ui)
kde4_add_kcfg_files(kcall_handler_SRCS kcallhandlersettings.kcfgc)
kde4_add_executable(kcall_handler ${kcall_handler_SRCS})
target_link_libraries(kcall_handler ${KDE4_KDEUI_LIBS}
                                    ${TELEPATHY_QT4_LIBRARIES}
                                    ${TELEPATHY_QT4_FARSIGHT_LIBRARIES}
                                    ${GSTREAMER_LIBRARIES}
                                    ${GOBJECT_LIBRARIES}
                                    ${TPFARSIGHT_LIBRARIES}
                                    kcallprivate
                                    kgstdevices
                                    kgstvideowidget)

install(TARGETS kcall_handler ${INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES callwindowui.rc DESTINATION ${DATA_INSTALL_DIR}/kcall_handler)

configure_file(org.freedesktop.Telepathy.Client.kcall_handler.service.in
                org.freedesktop.Telepathy.Client.kcall_handler.service)
install(FILES ${CMAKE_BINARY_DIR}/org.freedesktop.Telepathy.Client.kcall_handler.service
        DESTINATION ${DBUS_SERVICES_INSTALL_DIR})

# FIXME any better macro for ${XDG_DATA_DIRS} ?
install(FILES kcall_handler.client DESTINATION ${CMAKE_INSTALL_PREFIX}/share/telepathy/clients/)
