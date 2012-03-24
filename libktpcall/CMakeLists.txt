include_directories(${CMAKE_CURRENT_BINARY_DIR})

#####

set(libktpcall_SRCS
    call-channel-handler.cpp
    call-content-handler.cpp
    volume-controller.cpp

    private/source-controllers.cpp
    private/sink-controllers.cpp
    private/sink-managers.cpp
    private/device-element-factory.cpp
    private/phonon-integration.cpp
    private/input-control-bin.cpp
    private/video-sink-bin.cpp
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

add_subdirectory(tests)
