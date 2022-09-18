add_subdirectory(dialout)

include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})

set(ktp-call-ui_SRCS
    main.cpp
    call-window.cpp
    call-handler.cpp
    dtmf-handler.cpp
    status-area.cpp
    call-manager.cpp
    approver.cpp
    systemtray-icon.cpp
    qml-interface.cpp
    dtmf-qml.cpp
    ktp_call_ui_debug.cpp
)

add_executable(ktp-call-ui ${ktp-call-ui_SRCS})
target_link_libraries(ktp-call-ui
    ${QTGSTREAMER_QUICK_LIBRARY}
    KTp::CommonInternals
    KF5::Declarative
    KF5::I18n
    KF5::IconThemes
    KF5::Notifications
    KF5::XmlGui
    Qt5::Quick
    Qt5::Qml
    ktpcall
)

install(TARGETS ktp-call-ui DESTINATION ${KDE_INSTALL_LIBEXECDIR})
install(FILES callwindowui.rc DESTINATION ${KDE_INSTALL_KXMLGUI5DIR}/ktp-call-ui)

install( FILES
        qml/Main.qml
        DESTINATION ${KDE_INSTALL_DATADIR}/ktp-call-ui
)
install(DIRECTORY
        qml/core
        DESTINATION ${KDE_INSTALL_DATADIR}/ktp-call-ui
)

configure_file(org.freedesktop.Telepathy.Client.KTp.CallUi.service.in
                org.freedesktop.Telepathy.Client.KTp.CallUi.service)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/org.freedesktop.Telepathy.Client.KTp.CallUi.service
        DESTINATION ${KDE_INSTALL_DBUSSERVICEDIR})

install(FILES KTp.CallUi.client DESTINATION ${KDE_INSTALL_DATAROOTDIR}/telepathy/clients/)
