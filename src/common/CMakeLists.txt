# Protobuf runtime/library
find_package(Protobuf REQUIRED)

# protoc compiler (needed at build-time for code generation)
# Priority:
# 1) CMake cache / system discovery from FindProtobuf
# 2) Project-local tool: <repo>/.tools/bin/protoc
# 3) Auto-bootstrap a local protoc (Linux x86_64) into <repo>/.tools
if(NOT Protobuf_PROTOC_EXECUTABLE OR Protobuf_PROTOC_EXECUTABLE MATCHES "-NOTFOUND$")
  find_program(PROJECT_LOCAL_PROTOC
    NAMES protoc
    PATHS ${PROJECT_SOURCE_DIR}/.tools/bin
    NO_DEFAULT_PATH
  )

  if(PROJECT_LOCAL_PROTOC)
    set(Protobuf_PROTOC_EXECUTABLE "${PROJECT_LOCAL_PROTOC}" CACHE FILEPATH "Path to protoc" FORCE)
  endif()
endif()

if(NOT Protobuf_PROTOC_EXECUTABLE OR Protobuf_PROTOC_EXECUTABLE MATCHES "-NOTFOUND$")
  if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|amd64")
    set(PROTOC_DIR ${PROJECT_SOURCE_DIR}/.tools)
    set(PROTOC_BIN ${PROTOC_DIR}/bin/protoc)
    set(PROTOC_ZIP ${PROTOC_DIR}/protoc.zip)
    set(PROTOC_URL "https://github.com/protocolbuffers/protobuf/releases/download/v21.12/protoc-21.12-linux-x86_64.zip")

    file(MAKE_DIRECTORY ${PROTOC_DIR})

    message(STATUS "protoc not found; bootstrapping local protoc from ${PROTOC_URL}")
    file(DOWNLOAD ${PROTOC_URL} ${PROTOC_ZIP} STATUS DL_STATUS SHOW_PROGRESS)
    list(GET DL_STATUS 0 DL_CODE)
    if(NOT DL_CODE EQUAL 0)
      list(GET DL_STATUS 1 DL_MSG)
      message(FATAL_ERROR "Failed to download protoc: ${DL_MSG}")
    endif()

    execute_process(
      COMMAND ${CMAKE_COMMAND} -E tar xvf ${PROTOC_ZIP}
      WORKING_DIRECTORY ${PROTOC_DIR}
      RESULT_VARIABLE UNZIP_RC
    )
    if(NOT UNZIP_RC EQUAL 0)
      message(FATAL_ERROR "Failed to extract ${PROTOC_ZIP}")
    endif()

    if(EXISTS ${PROTOC_BIN})
      execute_process(COMMAND chmod +x ${PROTOC_BIN})
      set(Protobuf_PROTOC_EXECUTABLE "${PROTOC_BIN}" CACHE FILEPATH "Path to protoc" FORCE)
    endif()
  endif()
endif()

if(NOT Protobuf_PROTOC_EXECUTABLE OR Protobuf_PROTOC_EXECUTABLE MATCHES "-NOTFOUND$" OR NOT EXISTS ${Protobuf_PROTOC_EXECUTABLE})
  message(FATAL_ERROR
    "protoc not found. Install protobuf-compiler, or provide ${PROJECT_SOURCE_DIR}/.tools/bin/protoc")
endif()

set(PROTO_FILE ${CMAKE_CURRENT_SOURCE_DIR}/msg/message.proto)
set(PROTO_GEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)
file(MAKE_DIRECTORY ${PROTO_GEN_DIR})

set(PROTO_SRCS ${PROTO_GEN_DIR}/message.pb.cc)
set(PROTO_HDRS ${PROTO_GEN_DIR}/message.pb.h)

add_custom_command(
  OUTPUT ${PROTO_SRCS} ${PROTO_HDRS}
  COMMAND ${Protobuf_PROTOC_EXECUTABLE}
          --cpp_out=${PROTO_GEN_DIR}
          -I ${CMAKE_CURRENT_SOURCE_DIR}/msg
          ${PROTO_FILE}
  DEPENDS ${PROTO_FILE}
  COMMENT "Generating protobuf C++ sources from message.proto"
  VERBATIM
)

add_library(project_common STATIC
  ${PROTO_SRCS}
)

target_include_directories(project_common PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${PROTO_GEN_DIR}
)

target_link_libraries(project_common PUBLIC protobuf::libprotobuf)
