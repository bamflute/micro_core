############################################################################
#
# Copyright (c) 2019-2020 Lnjoying Tech All rights reserved.
#
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php
#
############################################################################

message(STATUS "begin to cmake.........")


############################################################################

SET(HOME_PATH ${CMAKE_SOURCE_DIR})
SET(SRC_PATH ${CMAKE_SOURCE_DIR}/src)


############################################################################

#if (LINUX)
SET(EXECUTABLE_OUTPUT_PATH ${HOME_PATH}/output/)
SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${HOME_PATH}/output)
SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${HOME_PATH}/output)
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${HOME_PATH}/output)
#endif()


#include directory
INCLUDE_DIRECTORIES(
${SRC_PATH}
${SRC_PATH}/3rd/http_parser
${SRC_PATH}/3rd/libuv-v1.x
)

set(SRC_FOLDER_LIST bus common io logger message module timer thread 3rd/http_parser)

set(SRC_LIST)

foreach(f ${SRC_FOLDER_LIST})
    message(STATUS ${f})
    AUX_SOURCE_DIRECTORY(${SRC_PATH}/${f} tmp)
    list(APPEND SRC_LIST ${tmp})
    INCLUDE_DIRECTORIES(  ${SRC_PATH}/${f})
endforeach()

#source directory
AUX_SOURCE_DIRECTORY(${SRC_PATH} DIR_SRCS)
############################################################################
message(STATUS "MICRO_CORE_3RD_LIB_ROOT_PATH ${MICRO_CORE_3RD_LIB_ROOT_PATH}" )
link_directories(${MICRO_CORE_3RD_LIB_ROOT_PATH}/libuv)
MESSAGE(STATUS "end link directory")
############################################################################

add_library(micro_core STATIC ${DIR_SRCS} ${SRC_LIST})
############################################################################

#SET(LIBRARIES)

MESSAGE(STATUS "end to cmake.........")
