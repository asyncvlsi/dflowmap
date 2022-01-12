#!/usr/bin/env bash
if [ ! -d build ]; then
	mkdir build
	cd build
	cmake ..
else 
	cd build
fi
cmake --build . -j 8
