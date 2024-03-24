#!/bin/bash

DIR="build"

if cmake -DGLFW_BUILD_DOCS=OFF -S . -B $DIR; then
	printf "\n"
	if cmake --build $DIR --parallel; then
		exit 0
	else
		printf ">> build failed\n"
		exit 1
	fi
else
	printf ">> configure failed\n"
	exit 1
fi
