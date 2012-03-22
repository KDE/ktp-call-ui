include_directories(${CMAKE_CURRENT_BINARY_DIR})

#####

set(libktpcall_SRCS
    call-channel-handler.cpp
    call-content-handler.cpp
    source-controllers.cpp
    sink-controllers.cpp
    volume-controller.cpp
    device-element-factory.cpp
    phonon-integration.cpp
)

kde4_add_library(ktpcall STATIC ${libktpcall_SRCS})
target_link_libraries(ktpcall
    ${KDE4_KDECORE_LIBS}
    ${QT_QTDBUS_LIBRARY}
    ${QTGSTREAMER_LIBRARIES}
    ${TELEPATHY_QT4_LIBRARIES}
    qtf
)

#####

kde4_add_executable(configuration_test configuration_test.cpp device-element-factory.cpp phonon-integration.cpp)
target_link_libraries(configuration_test ${KDE4_KDECORE_LIBS} ${QT_QTDBUS_LIBRARY} ${QTGSTREAMER_LIBRARIES})

add_subdirectory(tests)
