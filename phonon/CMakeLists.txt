include_directories(${GSTREAMER_INCLUDE_DIR} ${GLIB2_INCLUDE_DIR}
                    ${LIBXML2_INCLUDE_DIR} ${CMAKE_CURRENT_BINARY_DIR})

set(kgstvideowidget_SRCS
    videowidget.cpp
    abstractrenderer.cpp
    x11renderer.cpp
)

kde4_add_library(kgstvideowidget ${kgstvideowidget_SRCS})
