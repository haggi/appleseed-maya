# Copyright (c) 2013 Esteban Tovagliari

# Find Appleseed's headers and libraries.
#
#  This module defines
# APPLESEED_INCLUDE_DIRS - where to find APPLESEED includes.
# APPLESEED_LIBRARIES    - List of libraries when using APPLESEED.
# APPLESEED_FOUND        - True if APPLESEED found.

# Look for the includes.
FIND_PATH(APPLESEED_INCLUDE_DIR NAMES renderer/api/project.h)

# Look for the library.
FIND_LIBRARY(APPLESEED_LIBRARY NAMES appleseed)

# handle the QUIETLY and REQUIRED arguments and set APPLESEED_FOUND to TRUE if
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(
    APPLESEED DEFAULT_MSG
    APPLESEED_LIBRARY
    APPLESEED_INCLUDE_DIR)

# Copy the results to the output variables.
IF(APPLESEED_FOUND)
    SET(APPLESEED_LIBRARIES ${APPLESEED_LIBRARY})
    SET(APPLESEED_INCLUDE_DIRS ${APPLESEED_INCLUDE_DIR})
ELSE()
    SET(APPLESEED_LIBRARIES)
    SET(APPLESEED_INCLUDE_DIRS)
ENDIF()

MARK_AS_ADVANCED(APPLESEED_LIBRARY APPLESEED_INCLUDE_DIR)
