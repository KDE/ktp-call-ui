add_subdirectory(dialout)

include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})

set(ktp-call-ui_SRCS
    main.cpp
    call-window.cpp
    call-handler.cpp
    dtmf-handler.cpp
    dtmf-widget.cpp
    status-area.cpp
    call-manager.cpp
    approver.cpp
)

kde4_add_ui_files(ktp-call-ui_SRCS call-window.ui dtmf-widget.ui)
kde4_add_executable(ktp-call-ui ${ktp-call-ui_SRCS})
target_link_libraries(ktp-call-ui
    ${KDE4_KDEUI_LIBS}
    ${TELEPATHY_QT4_LIBRARIES}
    ${KTP_LIBRARIES}
    ${QTGSTREAMER_UI_LIBRARIES}
    ktpcall
)

install(TARGETS ktp-call-ui DESTINATION ${LIBEXEC_INSTALL_DIR})
install(FILES callwindowui.rc DESTINATION ${DATA_INSTALL_DIR}/ktp-call-ui)

configure_file(org.freedesktop.Telepathy.Client.KTp.CallUi.service.in
                org.freedesktop.Telepathy.Client.KTp.CallUi.service)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/org.freedesktop.Telepathy.Client.KTp.CallUi.service
        DESTINATION ${DBUS_SERVICES_INSTALL_DIR})

install(FILES KTp.CallUi.client DESTINATION ${SHARE_INSTALL_PREFIX}/telepathy/clients/)
