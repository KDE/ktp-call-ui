# This is *A LOT* of dependencies to find one by one,
# since gnome people like to include as many headers as possible
# in their public headers instead of doing forward declarations.
find_package(PkgConfig REQUIRED)
pkg_check_modules(TP_FARSTREAM REQUIRED telepathy-farstream)
pkg_check_modules(TP_QT4_YELL_FARSTREAM REQUIRED TelepathyQt4YellFarstream)

include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${QTGSTREAMER_INCLUDES}
    ${TP_FARSTREAM_INCLUDE_DIRS}
    ${TP_QT4_YELL_FARSTREAM_INCLUDE_DIRS}
)

add_definitions(
    ${QTGSTREAMER_DEFINITIONS}
    ${KDE4_ENABLE_EXCEPTIONS}

    # These prevent gstreamer from including libxml2 headers
    -DGST_DISABLE_XML
    -DGST_DISABLE_LOADSAVE
)

kde4_add_library(qtf STATIC qtf.cpp)

target_link_libraries(qtf
    ${QTGSTREAMER_LIBRARIES}
    ${TP_FARSTREAM_LDFLAGS}
    ${TP_QT4_YELL_FARSTREAM_STATIC_LDFLAGS}
)
