include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})

set(kcall_handler_SRCS
    main.cpp
    callwindow.cpp
    calllog.cpp
    callhandler.cpp
    channelhandler.cpp
    abstractmediahandler.cpp
    dtmfhandler.cpp
    dtmfwidget.cpp
    volumewidget.cpp
    farsight/audiobin.cpp
    farsight/videoinputbin.cpp
    farsight/farsightmediahandler.cpp
)

kde4_add_ui_files(kcall_handler_SRCS callwindow.ui dtmfwidget.ui)
kde4_add_kcfg_files(kcall_handler_SRCS kcallhandlersettings.kcfgc)
kde4_add_executable(kcall_handler ${kcall_handler_SRCS})
target_link_libraries(kcall_handler ${KDE4_KDEUI_LIBS}
                                    ${TELEPATHY_QT4_LIBRARIES}
                                    kcallprivate
                                    qtpfarsight
                                    kgstdevices
                                    kgstvideowidget
                                    QtGstreamer
                                    #extra dependencies of libQtGstreamer
                                    ${GSTREAMER_LIBRARIES}
                                    ${GSTREAMER_INTERFACE_LIBRARY}
                                    ${GOBJECT_LIBRARIES}
                                    #extra dependencies of libqtpfarsight
                                    ${TELEPATHY_QT4_FARSIGHT_LIBRARIES}
                                    ${TPFARSIGHT_LIBRARIES}
                                    #extra dependencies of libkgstdevices
                                    ${KDE4_SOLID_LIBS}
)

install(TARGETS kcall_handler ${INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES callwindowui.rc DESTINATION ${DATA_INSTALL_DIR}/kcall_handler)
install(FILES farsight/codec-preferences.ini DESTINATION ${DATA_INSTALL_DIR}/kcall_handler)

configure_file(org.freedesktop.Telepathy.Client.kcall_handler.service.in
                org.freedesktop.Telepathy.Client.kcall_handler.service)
install(FILES ${CMAKE_BINARY_DIR}/org.freedesktop.Telepathy.Client.kcall_handler.service
        DESTINATION ${DBUS_SERVICES_INSTALL_DIR})

# FIXME any better macro for ${XDG_DATA_DIRS} ?
install(FILES kcall_handler.client DESTINATION ${CMAKE_INSTALL_PREFIX}/share/telepathy/clients/)
