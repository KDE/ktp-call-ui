include_directories(${KDE4_INCLUDES} ${CMAKE_CURRENT_BINARY_DIR})
add_definitions(${KDE4_DEFINITIONS})

set(kgstdevices_SRCS
    devicemanager.cpp
    devicesmodel.cpp
    devicechooser.cpp
)

kde4_add_ui_files(kgstdevices_SRCS audiodevicechooser.ui)
kde4_add_library(kgstdevices STATIC ${kgstdevices_SRCS})
#set_target_properties(kgstdevices PROPERTIES SOVERSION 0 VERSION 0.0.1)
#target_link_libraries(kgstdevices ${KDE4_KDEUI_LIBS} ${KDE4_SOLID_LIBS} -lQtGstreamer)
