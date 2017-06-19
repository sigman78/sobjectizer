#!/bin/bash
set -ev

mkdir build && cd build
# Not using CXXFLAGS in order to avoid affecting dependencies
cmake .. \
    -DCMAKE_CXX_FLAGS="$CMAKE_CXX_FLAGS" \
    -DCMAKE_PREFIX_PATH="$HOME/deps" \
    -DCMAKE_BUILD_TYPE=Debug \
    -DBUILD_TESTS=ON
# Otherwise the job gets killed (probably because using too much memory)
make -j4
ctest -V
