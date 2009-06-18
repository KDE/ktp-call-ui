
include_directories(${CMAKE_CURRENT_BINARY_DIR})

set(kcall_handler_SRCS
    main.cpp
    callwindow.cpp
    callhandler.cpp
)

configure_file(org.freedesktop.Telepathy.Client.kcall_handler.service.in
               org.freedesktop.Telepathy.Client.kcall_handler.service)

kde4_add_executable(kcall_handler ${kcall_handler_SRCS})
target_link_libraries(kcall_handler ${KDE4_KPARTS_LIBS} ${TELEPATHY_QT4_LIBRARIES})

install(TARGETS kcall_handler ${INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES ${CMAKE_BINARY_DIR}/org.freedesktop.Telepathy.Client.kcall_handler.service
        DESTINATION ${DBUS_SERVICES_INSTALL_DIR})

# FIXME any better macro for ${XDG_DATA_DIRS} ?
install(FILES kcall_handler.client DESTINATION ${CMAKE_INSTALL_PREFIX}/share/telepathy/clients/)
