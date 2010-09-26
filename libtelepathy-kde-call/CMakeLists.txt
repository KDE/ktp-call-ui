include_directories(${QTGSTREAMER_INCLUDES} ${CMAKE_CURRENT_BINARY_DIR})

add_definitions(${QTGSTREAMER_DEFINITIONS})
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${QTGSTREAMER_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}")

set(libkcall_SRCS
    callchannelhandler.cpp
    callparticipant.cpp
    devicemanager.cpp
)

kde4_add_library(telepathy-kde-call STATIC ${libkcall_SRCS})
target_link_libraries(telepathy-kde-call qtpfarsight ${KDE4_KDECORE_LIBS} ${KDE4_SOLID_LIBS}
                                         ${QTGSTREAMER_UI_LIBRARIES} ${TELEPATHY_QT4_LIBRARIES})
install(TARGETS telepathy-kde-call ${INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES codec-preferences.ini DESTINATION ${DATA_INSTALL_DIR}/libtelepathy-kde-call)
