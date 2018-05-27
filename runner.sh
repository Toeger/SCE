GDB="gdb -q --return-child-result -ex 'set confirm off' -ex 'set pagination off' -ex 'handle SIG40 nostop' -ex run -ex bt -ex quit --args"
COMPILERS="CC=gcc CXX=g++
CC=clang CXX=clang++"
BUILDS='RUNNER="${GDB}" BUILD_TYPE=DEBUG SANITIZER=-fsanitize=undefined
BUILD_TYPE=DEBUG SANITIZER=-fsanitize=undefined,address
BUILD_TYPE=RELEASE'

COMPILERCOLOR="\033[0;34m" #blue
BUILDCOLOR="\033[1;34m" #light blue
NOCOLOR="\033[0m"

echo "$COMPILERS" | while IFS= read -r COMPILER; do
	export $COMPILER
	printf "${COMPILERCOLOR}Compiler: $COMPILER${NOCOLOR}\n"
	echo "$BUILDS" | while IFS= read -r BUILD; do
		printf "${BUILDCOLOR}Build: $BUILD${NOCOLOR}\n"
		export $BUILD
		mkdir build && cd build
		cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE -G Ninja ..
		ninja -j3
		$RUNNER ./SCE test
		cd ..
		rm -R build
	done
done
