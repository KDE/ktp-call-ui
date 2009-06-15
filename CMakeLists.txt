project(kcall)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules)

find_package(KDE4 REQUIRED)
find_package(TelepathyQt4 REQUIRED)
find_package(LibKNotificationItem-1 REQUIRED)

include_directories(${KDE4_INCLUDES} ${TELEPATHY_QT4_INCLUDE_DIR}
                    ${KNOTIFICATIONITEM_INCLUDE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
add_definitions(${KDE4_DEFINITIONS})

add_subdirectory(callwindowpart)

set(kcall_SRCS
    main.cpp
    kcallapplication.cpp
    mainwindow.cpp
    contactlistcontroller.cpp
    callwindow.cpp
    callhandler.cpp
    abstractclientapprover.cpp
    systrayicon.cpp
    knotifyapprover.cpp
    models/treemodel.cpp
    models/contactsmodel.cpp
    models/pendingcontacts.cpp
    models/accountitem.cpp
    models/contactitem.cpp
)

kde4_add_ui_files(kcall_SRCS mainwindow.ui)
kde4_add_executable(kcall ${kcall_SRCS})
target_link_libraries(kcall ${KDE4_KPARTS_LIBS} ${TELEPATHY_QT4_LIBRARIES} ${KNOTIFICATIONITEM_LIBRARIES})

install(TARGETS kcall ${INSTALL_TARGETS_DEFAULT_ARGS})
#install(FILES kcallui.rc DESTINATION ${DATA_INSTALL_DIR}/kcall)
install(FILES kcall.notifyrc DESTINATION ${DATA_INSTALL_DIR}/kcall)
