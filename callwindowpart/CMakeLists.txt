
include_directories(${KDE4_INCLUDES} ${TELEPATHY_QT4_INCLUDE_DIR} ${CMAKE_CURRENT_BINARY_DIR})

set(callwindowpart_SRCS
    callwindowpart.cpp
    callwindowpart_p.cpp
    pendingoutgoingcall.cpp
)

kde4_add_plugin(kcall_callwindowpart ${callwindowpart_SRCS})
target_link_libraries(kcall_callwindowpart ${KDE4_KPARTS_LIBS} ${TELEPATHY_QT4_LIBRARIES})

install(TARGETS kcall_callwindowpart DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES kcall_callwindowpart.desktop DESTINATION ${SERVICES_INSTALL_DIR})
