export GRPC_ROOT=$(realpath $(dirname $0))/../grpc
export CPPFLAGS="-I$GRPC_ROOT/include -I$GRPC_ROOT/third_party/protobuf/src"
export CXX=clang++
export LDFLAGS=-L$GRPC_ROOT/libs/opt
export GRPC_CPP_PLUGIN=$GRPC_ROOT/bins/opt/grpc_cpp_plugin
export PROTOC=$GRPC_ROOT/bins/opt/protobuf/protoc
