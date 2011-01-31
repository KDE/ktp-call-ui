project(telepathy-kde-call-ui)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules)

find_package(KDE4 REQUIRED)
find_package(TelepathyQt4 REQUIRED)
find_package(TelepathyQt4Farsight REQUIRED)
find_package(QtGStreamer REQUIRED)
find_package(TpFarsight REQUIRED)
find_package(Farsight2 REQUIRED)

include_directories(${KDE4_INCLUDES} ${TELEPATHY_QT4_INCLUDE_DIR})
add_definitions(${KDE4_DEFINITIONS})

add_subdirectory(libqtf)
add_subdirectory(libtelepathy-kde-call)
add_subdirectory(src)
add_subdirectory(test-contactlist)
