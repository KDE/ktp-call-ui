include_directories(${CMAKE_CURRENT_BINARY_DIR})

kde4_add_plugin(kcm_kcalldevices kcmkcalldevices.cpp)
target_link_libraries(kcm_kcalldevices ${KDE4_KDEUI_LIBS}
                                    kgstdevices
                                    #extra dependencies of libkgstdevices
                                    ${KDE4_SOLID_LIBS}
                                    QtGstreamer
                                    #extra dependencies of libQtGstreamer
                                    ${GSTREAMER_LIBRARIES}
                                    ${GSTREAMER_INTERFACE_LIBRARY}
                                    ${GOBJECT_LIBRARIES}
)

install(TARGETS kcm_kcalldevices DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES kcm_kcalldevices.desktop DESTINATION ${SERVICES_INSTALL_DIR})

###########

set(kcm_kcallsettings_SRCS kcmkcallsettings.cpp)
kde4_add_ui_files(kcm_kcallsettings_SRCS callwindowsettingspage.ui)
kde4_add_kcfg_files(kcm_kcallsettings_SRCS ../kcall_handler/kcallhandlersettings.kcfgc)
kde4_add_plugin(kcm_kcallsettings ${kcm_kcallsettings_SRCS})
target_link_libraries(kcm_kcallsettings ${KDE4_KDEUI_LIBS})

install(TARGETS kcm_kcallsettings DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES kcm_kcallsettings.desktop DESTINATION ${SERVICES_INSTALL_DIR})
