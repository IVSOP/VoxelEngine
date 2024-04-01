#!/usr/bin/bash

DIR="profiling"

if cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -S . -B $DIR; then
	printf "\n"
	if cmake --build $DIR --parallel; then
		printf "\n"
		# gdb -ex=r --args $@
	else
		printf ">> build failed\n"
	fi
else
	printf ">> configure failed\n"
fi

valgrind --tool=callgrind --instr-atstart=no profiling/VoxelEngine
gprof2dot --format=callgrind --output=out.dot callgrind.out.* && \
rm callgrind.out.* && \
dot -Tpng out.dot -o graph.png && \
rm out.dot
