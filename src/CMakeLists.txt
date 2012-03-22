include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})

set(ktp-call-ui_SRCS
    main.cpp
    callwindow.cpp
    #calllog.cpp
    callhandler.cpp
    dtmfhandler.cpp
    dtmfwidget.cpp
    #volumewidget.cpp
    #videobalancewidget.cpp
    statusarea.cpp
)

kde4_add_ui_files(ktp-call-ui_SRCS callwindow.ui dtmfwidget.ui)
kde4_add_kcfg_files(ktp-call-ui_SRCS kcallhandlersettings.kcfgc)
kde4_add_executable(ktp-call-ui ${ktp-call-ui_SRCS})
target_link_libraries(ktp-call-ui
    ${KDE4_KDEUI_LIBS}
    ${TELEPATHY_QT4_LIBRARIES}
    ktpcall
)

install(TARGETS ktp-call-ui DESTINATION ${LIBEXEC_INSTALL_DIR})
install(FILES callwindowui.rc DESTINATION ${DATA_INSTALL_DIR}/ktp-call-ui)

configure_file(org.freedesktop.Telepathy.Client.KTp.CallUi.service.in
                org.freedesktop.Telepathy.Client.KTp.CallUi.service)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/org.freedesktop.Telepathy.Client.KTp.CallUi.service
        DESTINATION ${DBUS_SERVICES_INSTALL_DIR})

install(FILES KTp.CallUi.client DESTINATION ${SHARE_INSTALL_PREFIX}/telepathy/clients/)
