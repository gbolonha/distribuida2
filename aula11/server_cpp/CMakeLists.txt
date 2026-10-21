cmake_minimum_required(VERSION 3.15)
project(FileProcessorServer CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(gRPC CONFIG REQUIRED)
find_package(Protobuf REQUIRED)

set(PROTO_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../proto")
set(PROTO_FILE "${PROTO_DIR}/file_processor.proto")

set(PROTO_SRC "${CMAKE_CURRENT_BINARY_DIR}/file_processor.pb.cc")
set(PROTO_HDR "${CMAKE_CURRENT_BINARY_DIR}/file_processor.pb.h")
set(GRPC_SRC "${CMAKE_CURRENT_BINARY_DIR}/file_processor.grpc.pb.cc")
set(GRPC_HDR "${CMAKE_CURRENT_BINARY_DIR}/file_processor.grpc.pb.h")

add_custom_command(
  OUTPUT ${PROTO_SRC} ${PROTO_HDR} ${GRPC_SRC} ${GRPC_HDR}
  COMMAND protobuf::protoc
  ARGS --grpc_out="${CMAKE_CURRENT_BINARY_DIR}"
       --cpp_out="${CMAKE_CURRENT_BINARY_DIR}"
       -I "${PROTO_DIR}"
       --plugin=protoc-gen-grpc=$<TARGET_FILE:gRPC::grpc_cpp_plugin>
       "${PROTO_FILE}"
  DEPENDS "${PROTO_FILE}"
)

include_directories("${CMAKE_CURRENT_BINARY_DIR}")

add_executable(server
  server.cc
  ${PROTO_SRC}
  ${GRPC_SRC}
)

target_link_libraries(server
  gRPC::grpc++
  gRPC::grpc
  protobuf::libprotobuf
)