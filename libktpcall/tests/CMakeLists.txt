kde4_add_executable(configuration_test
    configuration_test.cpp
    ../private/device-element-factory.cpp
    ../private/phonon-integration.cpp
)
target_link_libraries(configuration_test
    ${KDE4_KDECORE_LIBS}
    ${QT_QTDBUS_LIBRARY}
    ${QTGSTREAMER_LIBRARIES}
)

kde4_add_executable(sourcetest sourcetest.cpp)
target_link_libraries(sourcetest ktpcall)
