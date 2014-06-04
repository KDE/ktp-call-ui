project(ktp-call-ui)

set(KTP_CALL_UI_VERSION "0.8.0")
set(IS_KTP_INTERNAL_MODULE TRUE)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules)
cmake_policy(SET CMP0002 OLD)

find_package(KDE4 REQUIRED)
find_package(TelepathyQt4 REQUIRED)
find_package(TelepathyQt4Farstream REQUIRED)
find_package(KTp REQUIRED)
find_package(QtGStreamer REQUIRED)
find_package(Qt4 COMPONENTS QtDeclarative)
find_package(PkgConfig REQUIRED)

include_directories(
    ${KDE4_INCLUDES}
    ${TELEPATHY_QT4_INCLUDE_DIR}
    ${KTP_INCLUDE_DIR}
    ${QTGSTREAMER_INCLUDES}
    ${QTGSTREAMER_UI_LIBRARIES}
)
add_definitions(
    ${KDE4_DEFINITIONS}
    ${QTGSTREAMER_DEFINITIONS}
)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${QTGSTREAMER_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}")

configure_file(version.h.in ${CMAKE_CURRENT_BINARY_DIR}/src/version.h @ONLY)

add_subdirectory(libqtf)
add_subdirectory(libktpcall)
add_subdirectory(src)
