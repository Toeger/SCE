#!/bin/sh -e
COMPILERS="CXX=g++
CXX=clang++"
BUILDS="BUILD_TYPE=DEBUG SANITIZER=-fsanitize=undefined RUNNER=../gdb.sh
BUILD_TYPE=DEBUG SANITIZER=-fsanitize=undefined,address
BUILD_TYPE=DEBUG SANITIZER=-fsanitize=thread
BUILD_TYPE=RELEASE"

COMPILERCOLOR="\033[0;34m" #blue
BUILDCOLOR="\033[1;34m" #light blue
NOCOLOR="\033[0m"

echo "$BUILDS" | while IFS= read -r BUILD; do
	echo "$COMPILERS" | while IFS= read -r COMPILER; do
		printf "\n${COMPILERCOLOR}Compiler: $COMPILER\n${BUILDCOLOR}Build: $BUILD${NOCOLOR}\n"
		export $COMPILER
		export $BUILD
		mkdir -p testbuild && cd testbuild
		cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE -G Ninja ..
		time ninja
		time $RUNNER ./SCE test
		cd .. && rm -R testbuild
	done
done
