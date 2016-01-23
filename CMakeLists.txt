
CMAKE_MINIMUM_REQUIRED(VERSION 2.8.0 FATAL_ERROR)

######################################################
# project

PROJECT(mayaToAppleseed)

######################################################
# cmake stuff

SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH}
    ${PROJECT_SOURCE_DIR}/mayatocommon/cmake/Modules
    ${PROJECT_SOURCE_DIR}/cmake/Modules)

#disable in source builds
IF(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
    MESSAGE(FATAL_ERROR "CMake generation for mayaToAppleseed is not allowed within the source directory!")
ENDIF()

# Set the default built type
IF(NOT CMAKE_BUILD_TYPE)
  SET(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel." FORCE)
ENDIF()

######################################################
# version

######################################################
# options

######################################################
# platform specific

IF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    # ...
ELSEIF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    ADD_DEFINITIONS( -DLINUX)
ELSEIF(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    # ...
ELSE()
    MESSAGE(FATAL_ERROR "Platform ${CMAKE_SYSTEM_NAME} not supported yet")
ENDIF()

######################################################
# compiler flags

IF(CMAKE_COMPILER_IS_GNUCXX)
    # ...
ELSE()
    #MESSAGE( FATAL_ERROR "unknown compiler is not supported yet")
ENDIF()

######################################################
# dependencies

FIND_PACKAGE(Maya REQUIRED)
FIND_PACKAGE(Appleseed REQUIRED)
FIND_PACKAGE(Boost REQUIRED)

######################################################
# optional dependencies

######################################################
# definitions

ADD_DEFINITIONS(-D_BOOL)
ADD_DEFINITIONS(-DREQUIRE_IOSTREAM)

######################################################
# includes

INCLUDE_DIRECTORIES(${MAYA_INCLUDE_DIRS}
                    ${APPLESEED_INCLUDE_DIRS}
                    ${Boost_INCLUDE_DIRS})

######################################################
# subdirs

ADD_SUBDIRECTORY(src)
ADD_SUBDIRECTORY(src/appleseedtools)
