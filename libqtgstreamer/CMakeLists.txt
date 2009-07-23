include_directories(${GSTREAMER_INCLUDE_DIR} ${GLIB2_INCLUDE_DIR}
                    ${LIBXML2_INCLUDE_DIR} ${CMAKE_CURRENT_BINARY_DIR})

set(QtGstreamer_SRCS
    qgstglobal.cpp
    qgstobject.cpp
    qgstelement.cpp
    qgstbin.cpp
    qgstpipeline.cpp
    qgstelementfactory.cpp
    qgstpad.cpp
    qgstghostpad.cpp
    qgstbus.cpp
    qgvalue.cpp
    qgstcaps.cpp
    qgststructure.cpp
)

add_definitions(-fPIC)
kde4_add_library(QtGstreamer STATIC ${QtGstreamer_SRCS})
