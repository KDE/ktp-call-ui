
include_directories(${CMAKE_CURRENT_BINARY_DIR})

kde4_add_plugin(kcm_kcalldevices kcmkcalldevices.cpp)
target_link_libraries(kcm_kcalldevices ${KDE4_KDEUI_LIBS}
                                    kgstdevices
                                    #extra dependencies of libkgstdevices
                                    ${KDE4_SOLID_LIBS}
                                    QtGstreamer
                                    #extra dependencies of libQtGstreamer
                                    ${GSTREAMER_LIBRARIES}
                                    ${GOBJECT_LIBRARIES}
)

install(TARGETS kcm_kcalldevices DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES kcm_kcalldevices.desktop DESTINATION ${SERVICES_INSTALL_DIR})
