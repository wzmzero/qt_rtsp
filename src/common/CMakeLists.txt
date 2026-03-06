find_package(Protobuf REQUIRED)

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
