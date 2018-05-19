#!/bin/bash

mkdir -p build/native
cd build/native
cmake ../../native/
make clean
make -j4