project(kcall)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules)

find_package(KDE4 REQUIRED)
find_package(TelepathyQt4 REQUIRED)
find_package(TelepathyQt4Farsight REQUIRED)
find_package(LibKNotificationItem-1 REQUIRED)
find_package(GStreamer REQUIRED)
find_package(GObject REQUIRED)
find_package(GLIB2 REQUIRED)
find_package(TpFarsight REQUIRED)
# dbus-glib and libxml2 are not really used, but headers from other libs include stuff from those,
# so we need their INCLUDE_DIR variable
find_package(DbusGlib REQUIRED)
find_package(LibXml2 REQUIRED)

include(CheckCXXSourceCompiles)
set(CMAKE_REQUIRED_INCLUDES ${KDE4_INCLUDES})
check_cxx_source_compiles("
#include <QtCore/QtGlobal>
int main()
{
#if defined(QT_NO_GLIB)
#error \"Qt was compiled with Glib disabled\"
#endif
return 0;
}"
QT4_GLIB_SUPPORT)

if (NOT QT4_GLIB_SUPPORT)
    message(STATUS "WARNING: Qt4 is compiled without glib support. kcall will not work properly!")
endif (NOT QT4_GLIB_SUPPORT)


include_directories(${KDE4_INCLUDES} ${TELEPATHY_QT4_INCLUDE_DIR} ${TELEPATHY_QT4_FARSIGHT_INCLUDE_DIR}
                    ${KNOTIFICATIONITEM_INCLUDE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
add_definitions(${KDE4_DEFINITIONS})

add_subdirectory(libkcallprivate)
add_subdirectory(libkgstvideowidget)
add_subdirectory(libqtgstreamer)
add_subdirectory(libkgstdevices)
add_subdirectory(libqtpfarsight)
add_subdirectory(kcall_handler)
add_subdirectory(kcm)
add_subdirectory(tests)

##########

set(kcall_SRCS
    main.cpp
    kcallapplication.cpp
    mainwindow.cpp
    contactlistcontroller.cpp
    abstractclientapprover.cpp
    systrayicon.cpp
    knotifyapprover.cpp
)

kde4_add_ui_files(kcall_SRCS mainwindow.ui)
kde4_add_executable(kcall ${kcall_SRCS})
target_link_libraries(kcall ${KDE4_KUTILS_LIBS}
                            ${TELEPATHY_QT4_LIBRARIES}
                            ${TELEPATHY_QT4_FARSIGHT_LIBRARIES}
                            ${KNOTIFICATIONITEM_LIBRARIES}
                            kcallprivate)

install(TARGETS kcall ${INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES kcall.notifyrc DESTINATION ${DATA_INSTALL_DIR}/kcall)
