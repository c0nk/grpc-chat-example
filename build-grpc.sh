#!/bin/sh
apt-get install build-essential autoconf libtool python-dev python-enum34 python-futures python-setuptools python-tox
apt-get install libedit-dev

$GRPC_ROOT/tools/run_tests/build_python.sh 2.7

pushd $GRPC_ROOT/third_party/protobuf/python
python setup.py build
popd
