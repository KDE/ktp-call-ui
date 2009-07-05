project(kcall)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules)

find_package(KDE4 REQUIRED)
find_package(TelepathyQt4 REQUIRED)
find_package(LibKNotificationItem-1 REQUIRED)

macro_optional_find_package(GStreamer)
macro_log_feature(GSTREAMER_FOUND "GStreamer" "gstreamer 0.10 is required for kcall"
                                    "http://gstreamer.freedesktop.org/modules/" FALSE "0.10")

macro_optional_find_package(GObject)
macro_optional_find_package(GLIB2)
macro_log_feature(GOBJECT_FOUND "GObject" "GObject is required for kcall"
                                "http://www.gtk.org/download/" FALSE)

macro_optional_find_package(TpFarsight)
macro_log_feature(TPFARSIGHT_FOUND "telepathy-farsight" "telepathy-farsight is required for kcall"
                        "http://telepathy.freedesktop.org/releases/telepathy-farsight/" FALSE "0.0.4")


# dbus-glib and libxml2 are not really used, but headers from other libs include stuff from those,
# so we need their INCLUDE_DIR variable
macro_optional_find_package(DbusGlib)
macro_log_feature(DBUSGLIB_FOUND "dbus-glib" "dbus-glib is required for kcall"
                                "http://dbus.freedesktop.org/releases/dbus-glib/" FALSE)

macro_optional_find_package(LibXml2)
macro_log_feature(LIBXML2_FOUND "LibXml2" "LibXml2 is required for kcall"
                                    "http://xmlsoft.org/downloads.html" FALSE)

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

if (GSTREAMER_FOUND AND GOBJECT_FOUND AND TPFARSIGHT_FOUND AND DBUSGLIB_FOUND AND LIBXML2_FOUND)
   set(BUILD_KCALL_HANDLER TRUE)
else (GSTREAMER_FOUND AND GOBJECT_FOUND AND TPFARSIGHT_FOUND AND DBUSGLIB_FOUND AND LIBXML2_FOUND)
   set(BUILD_KCALL_HANDLER FALSE)
endif (GSTREAMER_FOUND AND GOBJECT_FOUND AND TPFARSIGHT_FOUND AND DBUSGLIB_FOUND AND LIBXML2_FOUND)


include_directories(${KDE4_INCLUDES} ${TELEPATHY_QT4_INCLUDE_DIR}
                    ${KNOTIFICATIONITEM_INCLUDE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
add_definitions(${KDE4_DEFINITIONS})


add_subdirectory(libkcallprivate)

if ( BUILD_KCALL_HANDLER )
    add_subdirectory(libkgstdevices)
    add_subdirectory(kcall_handler)
endif ( BUILD_KCALL_HANDLER )

set(kcall_SRCS
    main.cpp
    kcallapplication.cpp
    mainwindow.cpp
    contactlistcontroller.cpp
    abstractclientapprover.cpp
    systrayicon.cpp
    knotifyapprover.cpp
)

kde4_add_kcfg_files(kcall_SRCS kcall_handler/kcallhandlersettings.kcfgc)
kde4_add_ui_files(kcall_SRCS mainwindow.ui)
kde4_add_executable(kcall ${kcall_SRCS})
target_link_libraries(kcall ${KDE4_KDEUI_LIBS}
                            ${TELEPATHY_QT4_LIBRARIES}
                            ${KNOTIFICATIONITEM_LIBRARIES}
                            kcallprivate)

install(TARGETS kcall ${INSTALL_TARGETS_DEFAULT_ARGS})
#install(FILES kcallui.rc DESTINATION ${DATA_INSTALL_DIR}/kcall)
install(FILES kcall.notifyrc DESTINATION ${DATA_INSTALL_DIR}/kcall)
