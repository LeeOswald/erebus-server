protoc -I=. --cpp_out=. erebus.proto
protoc -I=. --grpc_out=. --plugin=protoc-gen-grpc="%GRPC_ROOT%\tools\grpc\grpc_cpp_plugin.exe" erebus.proto