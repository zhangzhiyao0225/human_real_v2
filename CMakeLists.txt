cmake_minimum_required(VERSION 3.20)
project(bitbot_efc)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE Debug)
# set(CMAKE_BUILD_TYPE Release)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native -g -O0")

include(idlgen.cmake)


include(botadaptor.cmake)
SET(BITBOT_LIB_NAME "bitbot_${BITBOT_TYPE}")
SET(BITBOT_APP_NAME "${BITBOT_TYPE}_app")
# Compile DDS idl files
find_package(fastcdr REQUIRED)
find_package(fastrtps REQUIRED) 

# Generate DDS lib for each message in idl directory
# set(EFC_DDS_SOURCES "")
# set(EFC_DDS_INCLUDES "")
# set(IDL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/idl")
# file(GLOB IDLs "${IDL_DIR}/*") 
# foreach(SUBDIR ${IDLs})
# 	Aux_source_directory(${SUBDIR} DDS_SOURCCES_SUBDIR)
#        	list(APPEND EFC_DDS_SOURCES ${DDS_SOURCCES_SUBDIR})
#         list(APPEND EFC_DDS_INCLUDES ${SUBDIR})
# endforeach()
# add_library(efc_dds_lib
#   ${EFC_DDS_SOURCES}
#   ${EFC_DDS_INCLUDES}
# )
target_link_libraries(efc_dds_lib fastrtps fastcdr)

# Bitbot EFC Library

set(BITBOT_DEPENDENCY_USE_PROXY TRUE)
set(BITBOT_DEPENDENCY_USE_LOCAL_FILE TRUE)
#set(BITBOT_DEPENDENCY_LOCAL_FILE_PATH ${CMAKE_SOURCE_DIR}/dependencies)
#include(dependencies/bitbot.cmake)
#BitbotAddDependency(
#  NAME           bitbot_kernel
#  FILE_NAME      "bitbot_kernel-main.zip"
#  FILE_PATH      ${BITBOT_DEPENDENCY_LOCAL_FILE_PATH}
#  USE_LOCAL_FILE TRUE
#)

find_package(yaml-cpp REQUIRED)
include_directories("/usr/include/eigen3")

SET(BITBOT_SOURCE_DIR ${BITBOT_SRC_ROOT_DIR}/${BITBOT_TYPE})
aux_source_directory(${BITBOT_SOURCE_DIR}/device BITBOT_EFC_DEVICE_SRC)
aux_source_directory(${BITBOT_SOURCE_DIR}/bus BITBOT_EFC_BUS_SRC)
set(BITBOT_EFC_SRC
  ${BITBOT_EFC_DEVICE_SRC}
  ${BITBOT_EFC_BUS_SRC}
  ${BITBOT_EFC_KERNEL_SRC}
  ${BITBOT_CURRENT_FRONTEND_SRC_DIR}
)

add_library(${BITBOT_LIB_NAME} ${BITBOT_EFC_SRC})
target_include_directories(${BITBOT_LIB_NAME} PUBLIC $<BUILD_INTERFACE:${THIRD_PARTY_INCLUDE}> $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include>
  ${IDL_GEN_INC_DIR}
  ${BITBOT_INCLUDES}
  ${BITBOT_KERNEL_INCLUDE_DIR}
  ${GLAZE_INCLUDE_DIR}
  ${BITBOT_CURRENT_FRONTEND_INC_DIR}
  )

target_link_libraries(${BITBOT_LIB_NAME} BitbotKernel 
  efc_dds_lib
)

# Bitbot main app
# FIXME: removed by xky
# include(FetchContent)
# FetchContent_Declare(
#   ovinf
#   GIT_REPOSITORY https://github.com/Dknt0/ovinf.git 
#   GIT_TAG main
# )
# FetchContent_MakeAvailable(ovinf)

include(thirdparties.cmake)


list(APPEND CMAKE_PREFIX_PATH "${PROJECT_SOURCE_DIR}/src/mj/src/custom_msgs/install")
list(APPEND CMAKE_PREFIX_PATH "${OPENVINO_ROOT_DIR}/cmake")

find_package(OpenVINO REQUIRED)

add_executable(${BITBOT_APP_NAME} main.cc)

target_include_directories(${BITBOT_APP_NAME} PUBLIC
  include
  ${BITBOT_INCLUDES}
  ${OVINF_INCLUDE_DIR}
  ${OPENVINO_INCLUDE_DIR}
  ${CONCURRENTQUEUE_INCLUDE_DIR}
  ${BITBOT_ROS_INC_DIR}
)

target_link_libraries(${BITBOT_APP_NAME}
  ${BITBOT_LIB_NAME}
  ovinf
  ${BITBOT_ROS_LIBS}
)

# Tests
set(BUILD_TEST ON)
if(BUILD_TEST)
  include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${BITBOT_INCLUDES}
    ${IDL_GEN_INC_DIR}
  )

  add_executable(pub_test src/test/pub_test.cc)
  target_link_libraries(pub_test efc_dds_lib)

  add_executable(sub_test src/test/sub_test.cc)
  target_link_libraries(sub_test efc_dds_lib)

  add_executable(ankle_test src/test/ankle_test.cc)
endif()
