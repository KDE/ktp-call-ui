
include_directories(${GSTREAMER_INCLUDE_DIR} ${GLIB2_INCLUDE_DIR}
                    ${LIBXML2_INCLUDE_DIR} ${CMAKE_CURRENT_BINARY_DIR})

set(kgstdevices_SRCS
    elementfactory_p.cpp
    devicefactory.cpp
    mediadevices.cpp
)

kde4_add_library(kgstdevices ${kgstdevices_SRCS})
target_link_libraries(kgstdevices ${KDE4_KDECORE_LIBS}
                                    ${GSTREAMER_LIBRARIES}
                                    ${GOBJECT_LIBRARIES})

#install(TARGETS kgstdevices ${INSTALL_TARGETS_DEFAULT_ARGS})
