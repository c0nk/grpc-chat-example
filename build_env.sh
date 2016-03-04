export GRPC_ROOT=third_party/grpc
export CPPFLAGS="-I$GRPC_ROOT/include -I$GRPC_ROOT/third_party/protobuf/src"
export CXX=clang++
export LDFLAGS="-L$GRPC_ROOT/libs/opt -L$GRPC_ROOT/libs/opt/protobuf"
export GRPC_CPP_PLUGIN=$GRPC_ROOT/bins/opt/grpc_cpp_plugin
export PROTOC=$GRPC_ROOT/bins/opt/protobuf/protoc

export LD_LIBRARY_PATH=$GRPC_ROOT/libs/opt:$GRPC_ROOT/libs/opt/protobuf:$DYLD_LIBRARY_PATH
export DYLD_LIBRARY_PATH=$GRPC_ROOT/libs/opt:$GRPC_ROOT/libs/opt/protobuf:$DYLD_LIBRARY_PATH
