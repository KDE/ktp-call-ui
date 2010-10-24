project(kcall)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules)

find_package(KDE4 REQUIRED)
find_package(TelepathyQt4 REQUIRED)
find_package(TelepathyQt4Farsight REQUIRED)
find_package(QtGstreamer REQUIRED)
find_package(TpFarsight REQUIRED)
find_package(Farsight2 REQUIRED)

include_directories(${KDE4_INCLUDES} ${TELEPATHY_QT4_INCLUDE_DIR}
                    ${CMAKE_CURRENT_BINARY_DIR})
add_definitions(${KDE4_DEFINITIONS})

add_subdirectory(libkcallprivate)
add_subdirectory(kcall_handler)
add_subdirectory(libtelepathy-kde-call)

##########

set(kcall_SRCS
    main.cpp
    kcallapplication.cpp
    mainwindow.cpp
    contactlistcontroller.cpp
    systrayicon.cpp
)

kde4_add_ui_files(kcall_SRCS mainwindow.ui)
kde4_add_executable(kcall ${kcall_SRCS})
target_link_libraries(kcall ${KDE4_KUTILS_LIBS}
                            ${TELEPATHY_QT4_LIBRARIES}
                            kcallprivate)

install(TARGETS kcall ${INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES kcallui.rc DESTINATION ${DATA_INSTALL_DIR}/kcall)
