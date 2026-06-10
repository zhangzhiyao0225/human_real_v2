cmake_minimum_required(VERSION 3.20)
project(bitbot_mc)

include(options.cmake)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE Release)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")

set(IDL_COMPILER ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/Fast-DDS-Gen/scripts/fastddsgen)
MESSAGE(STATUS "IDL_COMPILER: ${IDL_COMPILER}")

# Compile DDS idl files
find_package(fastcdr REQUIRED)
find_package(fastrtps REQUIRED)

# Generate DDS lib for each message in idl directory
set(EFC_DDS_SOURCES "")
set(EFC_DDS_INCLUDES "")

set(IDL_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/idl")
set(IDL_GEN_DIR "${IDL_ROOT_DIR}/gen")
set(IDL_SRC_DIR "${IDL_ROOT_DIR}/src")
set(IDL_GEN_INC_DIR "${IDL_GEN_DIR}")

file(GLOB IDLs "${IDL_SRC_DIR}/*.idl")
foreach(IDL ${IDLs})
	get_filename_component(IDL_NAME ${IDL} NAME_WE)

  SET(CUR_IDL_SRC_DIR ${IDL_GEN_DIR}/${IDL_NAME})

  message(STATUS "Generating DDS for ${IDL}: Name: ${IDL_NAME}, Dir: ${CUR_IDL_SRC_DIR}")
  file(MAKE_DIRECTORY ${CUR_IDL_SRC_DIR})

  set(IDL_OUT_CPP "${CUR_IDL_SRC_DIR}/${IDL_NAME}.cxx")
  set(IDL_OUT_HPP "${CUR_IDL_SRC_DIR}/${IDL_NAME}.h")
  set(IDL_OUT_PUBSUB_CPP "${CUR_IDL_SRC_DIR}/${IDL_NAME}PubSubTypes.cxx")
  set(IDL_OUT_PUBSUB_HPP "${CUR_IDL_SRC_DIR}/${IDL_NAME}PubSubTypes.h")
  # if (NOT EXISTS ${IDL_OUT_CPP})
    add_custom_command(
          OUTPUT ${IDL_OUT_CPP} ${IDL_OUT_HPP} ${IDL_OUT_PUBSUB_CPP} ${IDL_OUT_PUBSUB_HPP}

          COMMAND ${IDL_COMPILER} -typeros2 ${IDL} -d ${CUR_IDL_SRC_DIR}
                  # --cpp_out ${IDL_PATH}

          DEPENDS ${IDL}

          COMMENT "Generating C++ from ${IDL}"

          VERBATIM
      )
  # endif(NOT EXISTS ${IDL_OUT_CPP})
  
  list(APPEND EFC_DDS_SOURCES ${IDL_OUT_CPP} ${IDL_OUT_HPP} ${IDL_OUT_PUBSUB_CPP} ${IDL_OUT_PUBSUB_HPP})
  list(APPEND EFC_DDS_INCLUDES ${CUR_IDL_SRC_DIR})

endforeach(IDL)
# message(STATUS "Generating DDS source ${EFC_DDS_SOURCES}")
# message(STATUS "Generating DDS include ${EFC_DDS_INCLUDES}")

add_library(${BITBOT_TYPE}_dds_lib
  ${EFC_DDS_SOURCES}
  ${EFC_DDS_INCLUDES}
)
target_link_libraries(${BITBOT_TYPE}_dds_lib fastrtps fastcdr)
