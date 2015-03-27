include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})

set(ktp-dialout-ui_SRCS
    main.cpp
    dialout-widget.cpp
    main-window.cpp
    ../ktp_call_ui_debug.cpp
)

qt5_wrap_ui(ktp-dialout-ui_SRCS dialout-widget.ui)
add_executable(ktp-dialout-ui ${ktp-dialout-ui_SRCS})
target_link_libraries(ktp-dialout-ui
    KTp::CommonInternals
    KTp::Models
    KTp::Widgets
    KF5::XmlGui
    KF5::KDELibs4Support
)

install(TARGETS ktp-dialout-ui DESTINATION ${BIN_INSTALL_DIR})
