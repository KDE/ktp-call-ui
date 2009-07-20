include_directories(${GSTREAMER_INCLUDE_DIR} ${GLIB2_INCLUDE_DIR} ${DBUSGLIB_INCLUDE_DIR}
                    ${LIBXML2_INCLUDE_DIR} ${TPFARSIGHT_INCLUDE_DIR}
                    ${TELEPATHY_QT4_FARSIGHT_INCLUDE_DIR} ${CMAKE_CURRENT_BINARY_DIR})

kde4_add_library(qtpfarsight STATIC qtfchannel.cpp)
