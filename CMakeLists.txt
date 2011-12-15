project(telepathy-kde-call-ui)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules)

find_package(KDE4 REQUIRED)
find_package(TelepathyQt4 REQUIRED)
find_package(QtGStreamer REQUIRED)

# this module won't be used for long and I'm too lazy to write a cmake module right now :P
pkg_check_modules(PC_TPQT4_YELL REQUIRED TelepathyQt4Yell)

include_directories(${KDE4_INCLUDES} ${TELEPATHY_QT4_INCLUDE_DIR} ${PC_TPQT4_YELL_INCLUDE_DIRS})
add_definitions(${KDE4_DEFINITIONS})

add_subdirectory(libqtf)
add_subdirectory(libtelepathy-kde-call)
add_subdirectory(src)
add_subdirectory(test-contactlist)
