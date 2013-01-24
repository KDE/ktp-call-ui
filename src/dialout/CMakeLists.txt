include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})

set(ktp-dialout-ui_SRCS
    main.cpp
    dialout-widget.cpp
    main-window.cpp
)

kde4_add_ui_files(ktp-dialout-ui_SRCS dialout-widget.ui)
kde4_add_executable(ktp-dialout-ui ${ktp-dialout-ui_SRCS})
target_link_libraries(ktp-dialout-ui
    ${KDE4_KDEUI_LIBS}
    ${TELEPATHY_QT4_LIBRARIES}
    ${KTP_LIBRARIES}
    ${KTP_MODELS_LIBRARIES}
    ${KTP_WIDGETS_LIBRARIES}
)

install(TARGETS ktp-dialout-ui DESTINATION ${BIN_INSTALL_DIR})
