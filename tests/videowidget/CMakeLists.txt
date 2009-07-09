include_directories(${GSTREAMER_INCLUDE_DIR} ${GLIB2_INCLUDE_DIR}
                    ${LIBXML2_INCLUDE_DIR} ${CMAKE_CURRENT_BINARY_DIR})

kde4_add_executable(videowidgettest main.cpp)
target_link_libraries(videowidgettest kgstvideowidget
                                      ${KDE4_KDEUI_LIBS}
                                      ${GSTREAMER_LIBRARIES}
                                      ${GSTREAMER_INTERFACE_LIBRARY}
                                      ${GOBJECT_LIBRARIES})
