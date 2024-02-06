#!/bin/sh
protoc -I=. --cpp_out=./erebus erebus.proto
protoc -I=. --grpc_out=./erebus --plugin=protoc-gen-grpc=${gRPC_DIR}/grpc_cpp_plugin erebus.proto