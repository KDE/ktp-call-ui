include_directories(${CMAKE_CURRENT_BINARY_DIR})

#####

set(libktpcall_SRCS
    callchannelhandler.cpp
    callcontenthandler.cpp
    sourcecontrollers.cpp
    sinkcontrollers.cpp
    volumecontroller.cpp
    deviceelementfactory.cpp
    phononintegration.cpp
)

kde4_add_library(ktpcall STATIC ${libktpcall_SRCS})
target_link_libraries(ktpcall
    ${KDE4_KDECORE_LIBS}
    ${QT_QTDBUS_LIBRARY}
    ${QTGSTREAMER_LIBRARIES}
    ${TELEPATHY_QT4_LIBRARIES}
    qtf
)

install(FILES codec-preferences.ini DESTINATION ${DATA_INSTALL_DIR}/libktpcall)


#####

kde4_add_executable(configuration_test configuration_test.cpp deviceelementfactory.cpp phononintegration.cpp)
target_link_libraries(configuration_test ${KDE4_KDECORE_LIBS} ${QT_QTDBUS_LIBRARY} ${QTGSTREAMER_LIBRARIES})

add_subdirectory(tests)
