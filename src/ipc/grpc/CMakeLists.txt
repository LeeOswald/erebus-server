add_subdirectory(grpc-common)
add_library(erebus::grpc_common ALIAS erebus-grpc-common)

add_subdirectory(grpc-server)
add_library(erebus::grpc_server ALIAS erebus-grpc-server)