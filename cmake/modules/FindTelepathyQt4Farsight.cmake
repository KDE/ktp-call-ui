# Try to find the Qt4 binding of the Telepathy Farsight library
# TELEPATHY_QT4_FARSIGHT_FOUND - system has TelepathyQt4Farsight
# TELEPATHY_QT4_FARSIGHT_INCLUDE_DIR - the TelepathyQt4Farsight include directory
# TELEPATHY_QT4_FARSIGHT_LIBRARIES - Link these to use TelepathyQt4Farsight

# Copyright (c) 2009, Andre Moreira Magalhaes <andrunko@gmail.com>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

set(TELEPATHY_QT4_FARSIGHT_FIND_REQUIRED ${TelepathyQt4Farsight_FIND_REQUIRED})
if(TELEPATHY_QT4_FARSIGHT_INCLUDE_DIR AND TELEPATHY_QT4_FARSIGHT_LIBRARIES)
  # Already in cache, be silent
  set(TELEPATHY_QT4_FARSIGHT_FIND_QUIETLY TRUE)
endif(TELEPATHY_QT4_FARSIGHT_INCLUDE_DIR AND TELEPATHY_QT4_FARSIGHT_LIBRARIES)

find_package(PkgConfig)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(PC_TELEPATHY_QT4_FARSIGHT QUIET TelepathyQt4Farsight>=0.1.8)
endif(PKG_CONFIG_FOUND)

find_path(TELEPATHY_QT4_FARSIGHT_INCLUDE_DIR
          NAMES TelepathyQt4/Farsight/Channel
          HINTS
          ${PC_TELEPATHY_QT4_FARSIGHT_INCLUDEDIR}
          ${PC_TELEPATHY_QT4_FARSIGHT_INCLUDE_DIRS}
)

find_library(TELEPATHY_QT4_FARSIGHT_LIBRARIES
             NAMES telepathy-qt4-farsight
             HINTS
             ${PC_TELEPATHY_QT4_FARSIGHT_LIBDIR}
             ${PC_TELEPATHY_QT4_FARSIGHT_LIBRARY_DIRS}
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(TELEPATHY_QT4_FARSIGHT DEFAULT_MSG
                                  TELEPATHY_QT4_FARSIGHT_LIBRARIES TELEPATHY_QT4_FARSIGHT_INCLUDE_DIR)
