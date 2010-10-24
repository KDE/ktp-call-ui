add_subdirectory(libkcallprivate)

include_directories(${CMAKE_CURRENT_BINARY_DIR})

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
