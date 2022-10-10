cmake_minimum_required(VERSION 3.16)

# KDE Gear Version, managed by release script
set (RELEASE_SERVICE_VERSION_MAJOR "22")
set (RELEASE_SERVICE_VERSION_MINOR "08")
set (RELEASE_SERVICE_VERSION_MICRO "2")

set(KTP_CALL_UI_VERSION "${RELEASE_SERVICE_VERSION_MAJOR}.${RELEASE_SERVICE_VERSION_MINOR}.${RELEASE_SERVICE_VERSION_MICRO}")

project(ktp-call-ui VERSION ${KTP_CALL_UI_VERSION})

find_package(ECM 1.3.0 REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR})

find_package(KF5 REQUIRED COMPONENTS Config Declarative IconThemes I18n Notifications XmlGui KCMUtils)
find_package(Qt5 REQUIRED COMPONENTS Quick Qml)
find_package(KTp REQUIRED)
find_package(TelepathyQt5 REQUIRED)
find_package(TelepathyQt5Farstream REQUIRED)
find_package(Qt5GStreamer REQUIRED)
find_package(Phonon4Qt5 REQUIRED)
find_package(PkgConfig REQUIRED)

include_directories( ${QTGSTREAMER_INCLUDES} )
add_definitions( ${QTGSTREAMER_DEFINITIONS} )

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(ECMSetupVersion)
include(FeatureSummary)

kde_enable_exceptions()
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${QTGSTREAMER_FLAGS}")

configure_file(version.h.in ${CMAKE_CURRENT_BINARY_DIR}/src/version.h @ONLY)

add_subdirectory(libqtf)
add_subdirectory(libktpcall)
add_subdirectory(src)

ki18n_install(po)

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
