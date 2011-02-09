include_directories(${QTGSTREAMER_INCLUDES} ${CMAKE_CURRENT_BINARY_DIR})

#####

add_definitions(${QTGSTREAMER_DEFINITIONS})
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${QTGSTREAMER_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}")

set(libtelepathy-kde-call_SRCS
    callchannelhandler.cpp
    callcontenthandler.cpp
    sourcecontrollers.cpp
    sinkcontrollers.cpp
    volumecontroller.cpp
    deviceelementfactory.cpp
    phononintegration.cpp
)

kde4_add_library(telepathy-kde-call STATIC ${libtelepathy-kde-call_SRCS})
target_link_libraries(telepathy-kde-call
    ${KDE4_KDECORE_LIBS}
    ${QT_QTDBUS_LIBRARY}
    ${QTGSTREAMER_LIBRARIES}
    ${TELEPATHY_QT4_LIBRARIES}
    ${PC_TPQT4_YELL_LDFLAGS}
    qtf
)

install(FILES codec-preferences.ini DESTINATION ${DATA_INSTALL_DIR}/libtelepathy-kde-call)


#####

kde4_add_executable(configuration_test configuration_test.cpp deviceelementfactory.cpp phononintegration.cpp)
target_link_libraries(configuration_test ${KDE4_KDECORE_LIBS} ${QT_QTDBUS_LIBRARY} ${QTGSTREAMER_LIBRARIES})

add_subdirectory(tests)
