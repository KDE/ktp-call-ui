project(ktp-call-ui)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules)

find_package(KDE4 REQUIRED)
find_package(TelepathyQt4 REQUIRED)
find_package(TelepathyQt4Farstream REQUIRED)
find_package(QtGStreamer REQUIRED)
find_package(PkgConfig REQUIRED)

include_directories(
    ${KDE4_INCLUDES}
    ${TELEPATHY_QT4_INCLUDE_DIR}
    ${QTGSTREAMER_INCLUDES}
)
add_definitions(
    ${KDE4_DEFINITIONS}
    ${QTGSTREAMER_DEFINITIONS}
)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${QTGSTREAMER_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}")

add_subdirectory(libqtf)
add_subdirectory(libktpcall)
add_subdirectory(src)
