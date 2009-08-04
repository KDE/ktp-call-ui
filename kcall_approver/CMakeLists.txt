include_directories(${CMAKE_CURRENT_BINARY_DIR})

set (kcall_approver_SRCS
    main.cpp
    knotifyapprover.cpp
)

kde4_add_executable(kcall_approver ${kcall_approver_SRCS})
target_link_libraries(kcall_approver ${KDE4_KDEUI_LIBS}
                                    ${TELEPATHY_QT4_LIBRARIES}
                                    kcallprivate
)

install(TARGETS kcall_approver ${INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES kcall_approver.notifyrc DESTINATION ${DATA_INSTALL_DIR}/kcall_approver)

configure_file(org.freedesktop.Telepathy.Client.kcall_approver.service.in
                org.freedesktop.Telepathy.Client.kcall_approver.service)
install(FILES ${CMAKE_BINARY_DIR}/org.freedesktop.Telepathy.Client.kcall_approver.service
        DESTINATION ${DBUS_SERVICES_INSTALL_DIR})

# FIXME any better macro for ${XDG_DATA_DIRS} ?
install(FILES kcall_approver.client DESTINATION ${CMAKE_INSTALL_PREFIX}/share/telepathy/clients/)
