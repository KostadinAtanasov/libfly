#!/bin/sh

cd build

target_dir=ReleaseTests
fly_dir=../../src/
cmake_flags="-DCMAKE_BUILD_TYPE=ReleaseTests -DBUILD_TESTS:Bool=true"

if [ ! -d $target_dir ]; then
	mkdir $target_dir
fi

if [ $1 ]; then
	if [ $1 = "clean" ]; then
		rm $target_dir -r
		exit $?
	fi
fi

cd $target_dir && cmake $cmake_flags $fly_dir && make "$@"
