# - Try to find farsight2
#
# Based on FindGStreamer.cmake
# Copyright (c) 2006, Tim Beaulen <tbscope@gmail.com>
# Copyright (c) 2009, George Kiagiadakis <kiagiadakis.george@gmail.com>
# Copyright (c) 2010, Collabora Ltd. <info@collabora.co.uk>
#   @author George Kiagiadakis <george.kiagiadakis@collabora.co.uk>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

IF (FARSIGHT2_INCLUDE_DIR AND FARSIGHT2_LIBRARIES)
   # in cache already
   SET(Farsight2_FIND_QUIETLY TRUE)
ELSE (FARSIGHT2_INCLUDE_DIR AND FARSIGHT2_LIBRARIES)
   SET(Farsight2_FIND_QUIETLY FALSE)
ENDIF (FARSIGHT2_INCLUDE_DIR AND FARSIGHT2_LIBRARIES)

IF (NOT WIN32)
   # use pkg-config to get the directories and then use these values
   # in the FIND_PATH() and FIND_LIBRARY() calls
   FIND_PACKAGE(PkgConfig)
   PKG_CHECK_MODULES(PC_FARSIGHT2 QUIET farsight2-0.10)
   SET(FARSIGHT2_DEFINITIONS ${PC_FARSIGHT2_CFLAGS_OTHER})
ENDIF (NOT WIN32)

FIND_PATH(FARSIGHT2_INCLUDE_DIR gstreamer-0.10/gst/farsight/fs-codec.h
   PATHS
   ${PC_FARSIGHT2_INCLUDEDIR}
   ${PC_FARSIGHT2_INCLUDE_DIRS}
   )

FIND_LIBRARY(FARSIGHT2_LIBRARIES NAMES gstfarsight-0.10
   PATHS
   ${PC_FARSIGHT2_LIBDIR}
   ${PC_FARSIGHT2_LIBRARY_DIRS}
   )

IF (NOT FARSIGHT2_INCLUDE_DIR)
   MESSAGE(STATUS "Farsight2: WARNING: include dir not found")
ENDIF (NOT FARSIGHT2_INCLUDE_DIR)

IF (NOT FARSIGHT2_LIBRARIES)
   MESSAGE(STATUS "Farsight2: WARNING: library not found")
ENDIF (NOT FARSIGHT2_LIBRARIES)

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(TpFarsight  DEFAULT_MSG  FARSIGHT2_LIBRARIES FARSIGHT2_INCLUDE_DIR)

MARK_AS_ADVANCED(FARSIGHT2_INCLUDE_DIR FARSIGHT2_LIBRARIES)
