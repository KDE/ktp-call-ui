include_directories(${CMAKE_CURRENT_BINARY_DIR})

set(kgstdevices_SRCS
    devicemanager.cpp
    devicesmodel.cpp
    tabledevicechooser.cpp
    abstractrenderer.cpp
    overlayrenderer.cpp
    videowidget.cpp
)

add_definitions(-fPIC)
kde4_add_ui_files(kgstdevices_SRCS tabledevicechooser.ui)
kde4_add_library(kgstdevices STATIC ${kgstdevices_SRCS})
