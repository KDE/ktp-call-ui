# - Try to find telepathy-farsight
#
# Based on FindGStreamer.cmake
# Copyright (c) 2006, Tim Beaulen <tbscope@gmail.com>
# Copyright (c) 2009, George Kiagiadakis <kiagiadakis.george@gmail.com>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

IF (TPFARSIGHT_INCLUDE_DIR AND TPFARSIGHT_LIBRARIES)
   # in cache already
   SET(TpFarsight_FIND_QUIETLY TRUE)
ELSE (TPFARSIGHT_INCLUDE_DIR AND TPFARSIGHT_LIBRARIES)
   SET(TpFarsight_FIND_QUIETLY FALSE)
ENDIF (TPFARSIGHT_INCLUDE_DIR AND TPFARSIGHT_LIBRARIES)

IF (NOT WIN32)
   # use pkg-config to get the directories and then use these values
   # in the FIND_PATH() and FIND_LIBRARY() calls
   FIND_PACKAGE(PkgConfig)
   PKG_CHECK_MODULES(PC_TPFARSIGHT QUIET telepathy-farsight>=0.0.4)
   SET(TPFARSIGHT_DEFINITIONS ${PC_TPFARSIGHT_CFLAGS_OTHER})
ENDIF (NOT WIN32)

FIND_PATH(TPFARSIGHT_INCLUDE_DIR telepathy-farsight/channel.h
   PATHS
   ${PC_TPFARSIGHT_INCLUDEDIR}
   ${PC_TPFARSIGHT_INCLUDE_DIRS}
   )

FIND_LIBRARY(TPFARSIGHT_LIBRARIES NAMES telepathy-farsight
   PATHS
   ${PC_TPFARSIGHT_LIBDIR}
   ${PC_TPFARSIGHT_LIBRARY_DIRS}
   )

IF (NOT TPFARSIGHT_INCLUDE_DIR)
   MESSAGE(STATUS "TpFarsight: WARNING: include dir not found")
ENDIF (NOT TPFARSIGHT_INCLUDE_DIR)

IF (NOT TPFARSIGHT_LIBRARIES)
   MESSAGE(STATUS "TpFarsight: WARNING: library not found")
ENDIF (NOT TPFARSIGHT_LIBRARIES)

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(TpFarsight  DEFAULT_MSG  TPFARSIGHT_LIBRARIES TPFARSIGHT_INCLUDE_DIR)

MARK_AS_ADVANCED(TPFARSIGHT_INCLUDE_DIR TPFARSIGHT_LIBRARIES)
