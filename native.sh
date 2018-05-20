#!/bin/bash

# Hack for macOS stuff
if [ `uname -s` == "Darwin" ]; then
  echo "MACOS GAH"
  export PATH="$PATH:/usr/local/bin/"
  export CC=/Users/pwootage/bin/clang-5.0.1/bin/clang
  export CXX=/Users/pwootage/bin/clang-5.0.1/bin/clang++
fi

mkdir -p build/native
cd build/native
cmake ../../native/
make clean
make -j4