protoc -I=. --cpp_out=. erebus.proto
protoc -I=. --grpc_out=. --plugin=protoc-gen-grpc="%gRPC_DIR%\tools\grpc\grpc_cpp_plugin.exe" erebus.proto