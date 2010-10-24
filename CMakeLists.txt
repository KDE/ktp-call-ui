project(kcall)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules)

find_package(KDE4 REQUIRED)
find_package(TelepathyQt4 REQUIRED)
find_package(TelepathyQt4Farsight REQUIRED)
find_package(QtGstreamer REQUIRED)
find_package(TpFarsight REQUIRED)
find_package(Farsight2 REQUIRED)

include_directories(${KDE4_INCLUDES} ${TELEPATHY_QT4_INCLUDE_DIR})
add_definitions(${KDE4_DEFINITIONS})

add_subdirectory(kcall_handler)
add_subdirectory(libtelepathy-kde-call)
add_subdirectory(test-contactlist)
