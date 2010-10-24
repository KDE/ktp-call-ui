include_directories(${CMAKE_CURRENT_BINARY_DIR})


set(libkcallprivate_SRCS
    treemodel.cpp
    accountmanager.cpp
    pendingcontacts.cpp
    accountitem.cpp
    contactitem.cpp
    groupmembersmodel.cpp
    abstractclientapprover.cpp
)

kde4_add_library(kcallprivate STATIC ${libkcallprivate_SRCS})
target_link_libraries(kcallprivate ${KDE4_KDEUI_LIBS} ${TELEPATHY_QT4_LIBRARIES})
