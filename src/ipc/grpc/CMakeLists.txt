add_subdirectory(grpc-common-lib)
add_library(erebus::grpc_common_lib ALIAS erebus-grpc-common-lib)

add_subdirectory(grpc-server-lib)
add_library(erebus::grpc_server_lib ALIAS erebus-grpc-server-lib)

add_subdirectory(grpc-client-lib)
add_library(erebus::grpc_client_lib ALIAS erebus-grpc-client-lib)