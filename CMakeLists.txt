cmake_minimum_required (VERSION 3.2)

############################################################################

#debug setting
set(DEBUG_MODE ON)
#set(CMAKE_VERBOSE_MAKEFILE on)

project("micro_core")

set(VERSION_MAJOR 0)
set(VERSION_MINOR 2)
set(VERSION_PATCH 0)
set(VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")

#set(CMAKE_CXX_STANDARD 11)

include(cmake/compiler_option.cmake)
include(cmake/find_external_lib.cmake)

SET(HOME_PATH ${CMAKE_SOURCE_DIR})

if ($ENV{MICRO_CORE_OUTPUT})
    SET(EXECUTABLE_OUTPUT_PATH $ENV{MICRO_CORE_OUTPUT}/output)
    SET(LIBRARY_OUTPUT_PATH $ENV{MICRO_CORE_OUTPUT}/output)
else ()
    SET(EXECUTABLE_OUTPUT_PATH ${HOME_PATH}/output)
    SET(LIBRARY_OUTPUT_PATH ${HOME_PATH}/output)
endif ()

# dcloud executable
add_subdirectory("src")

#add_dependencies(micro_core)
