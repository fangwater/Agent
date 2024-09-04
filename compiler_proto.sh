#!/bin/bash

PROTO_DIR="./proto"
OUT_DIR="./proto_gen"

# 创建输出目录
mkdir -p ${OUT_DIR}

# 编译 proto 目录下的所有 .proto 文件
for proto_file in ${PROTO_DIR}/*.proto; do
    protoc -I=${PROTO_DIR} --cpp_out=${OUT_DIR} --grpc_out=${OUT_DIR} --plugin=protoc-gen-grpc=`which grpc_cpp_plugin` ${proto_file}
    if [ $? -ne 0 ]; then
        echo "Failed to compile ${proto_file}."
        exit 1
    fi
done

echo "All proto files compiled successfully."
