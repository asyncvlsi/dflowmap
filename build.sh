#!/usr/bin/env bash
./configure
if [ ! -d build ]; then
	mkdir build
fi
cd build
cmake -DCMAKE_INSTALL_PREFIX=$ACT_HOME ..
make -j
