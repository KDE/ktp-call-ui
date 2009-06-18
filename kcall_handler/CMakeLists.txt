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


if (BUILD_KCALL_HANDLER)
   include_directories(${GSTREAMER_INCLUDE_DIR} ${GLIB2_INCLUDE_DIR} ${DBUSGLIB_INCLUDE_DIR}
                       ${LIBXML2_INCLUDE_DIR} ${TPFARSIGHT_INCLUDE_DIR} ${CMAKE_CURRENT_BINARY_DIR})

    set(kcall_handler_SRCS
        main.cpp
        callwindow.cpp
        callhandler.cpp
        channelhandler.cpp
        abstractmediahandler.cpp
        farsight/mediadevices.cpp
        farsight/mediahandler.cpp
    )

    kde4_add_executable(kcall_handler ${kcall_handler_SRCS})
    target_link_libraries(kcall_handler ${KDE4_KDEUI_LIBS}
                                        ${TELEPATHY_QT4_LIBRARIES}
                                        ${TELEPATHY_QT4_FARSIGHT_LIBRARIES}
                                        ${GSTREAMER_LIBRARIES}
                                        ${GOBJECT_LIBRARIES}
                                        ${TPFARSIGHT_LIBRARIES})

    install(TARGETS kcall_handler ${INSTALL_TARGETS_DEFAULT_ARGS})
    install(FILES callwindowui.rc DESTINATION ${DATA_INSTALL_DIR}/kcall_handler)

    configure_file(org.freedesktop.Telepathy.Client.kcall_handler.service.in
                   org.freedesktop.Telepathy.Client.kcall_handler.service)
    install(FILES ${CMAKE_BINARY_DIR}/org.freedesktop.Telepathy.Client.kcall_handler.service
            DESTINATION ${DBUS_SERVICES_INSTALL_DIR})

    # FIXME any better macro for ${XDG_DATA_DIRS} ?
    install(FILES kcall_handler.client DESTINATION ${CMAKE_INSTALL_PREFIX}/share/telepathy/clients/)

endif (BUILD_KCALL_HANDLER)
