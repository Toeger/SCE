cmake_minimum_required(VERSION 3.1)
project(SCE)

set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 17)

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)
# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)

# Find the QtWidgets library
find_package(Qt5Widgets CONFIG REQUIRED)

# Set up protobuffer
if (NOT DEFINED ENV{TRAVIS})
	set(USR_PATH "/usr")
else()
	set(USR_PATH "/usr/local")
endif()

set(CMAKE_MODULE_PATH "${USR_PATH}/lib/grpc/cmake/;${USR_PATH}/lib/protobuf/cmake/;${CMAKE_MODULE_PATH}")
find_package(Protobuf REQUIRED)
include_directories(${CMAKE_CURRENT_BINARY_DIR})
PROTOBUF_GENERATE_CPP(PROTO_SRCS PROTO_HDRS interop/sce.proto)
PROTOBUF_GENERATE_PYTHON(PROTO_PY_SRCS interop/sce.proto)
if (UNIX)
	set(GRPC_PLUGIN_PATH "${USR_PATH}/bin")
else()
	message(FATAL_ERROR "Figure out where the grpc_cpp_plugin is located on your system and add it in the CMakeLists.txt")
endif()
add_custom_command(
	COMMAND ${Protobuf_PROTOC_EXECUTABLE} "--grpc_out=." "--plugin=protoc-gen-grpc=${GRPC_PLUGIN_PATH}/grpc_cpp_plugin" -I${CMAKE_CURRENT_SOURCE_DIR}/interop ${CMAKE_CURRENT_SOURCE_DIR}/interop/sce.proto
	COMMAND ${Protobuf_PROTOC_EXECUTABLE} "--python_out=${CMAKE_CURRENT_SOURCE_DIR}/testdata/interop_scripts" "--grpc_python_out=${CMAKE_CURRENT_SOURCE_DIR}/testdata/interop_scripts" "--plugin=protoc-gen-grpc_python=${GRPC_PLUGIN_PATH}/grpc_python_plugin" -I${CMAKE_CURRENT_SOURCE_DIR}/interop ${CMAKE_CURRENT_SOURCE_DIR}/interop/sce.proto
	DEPENDS interop/sce.proto
	OUTPUT sce.grpc.pb.cc  sce.grpc.pb.h #${CMAKE_CURRENT_SOURCE_DIR}/testdata/sce_pb2_grpc.py  ${CMAKE_CURRENT_SOURCE_DIR}/testdata/sce_pb2.py
	COMMENT "Generating RPC code for sce.proto.")

#extra flags
if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU" OR "${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
	#general warnings
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror")
	#disable warning for generated protobuf file
	set_source_files_properties(sce.pb.cc sce.grpc.pb.cc PROPERTIES COMPILE_FLAGS -w)
	#sanitizers
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=undefined,address")
	set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} -fsanitize=undefined,address")
	#disable annoying warnings
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wno-unused-parameter")
endif()

#defines
add_definitions(-DTEST_DATA_PATH=\"${CMAKE_SOURCE_DIR}/testdata/\")

# Source files
set(SCE_SRC
	interop/plugin.cpp
	logic/process_reader.cpp
	logic/settings.cpp
	logic/syntax_highligher.cpp
	logic/tool.cpp
	logic/tool_actions.cpp
	main.cpp
	tests/test.cpp
	tests/test_mainwindow.cpp
	tests/test_plugin.cpp
	tests/test_process_reader.cpp
	tests/test_settings.cpp
	tests/test_tool.cpp
	tests/test_tool_editor_widget.cpp
	ui/edit_window.cpp
	ui/mainwindow.cpp
	ui/tool_editor_widget.cpp
	utility/thread_call.cpp
	utility/unique_handle.cpp
)

# Create code from a list of Qt designer ui files.
set(CMAKE_AUTOUIC ON)
#disable autouic for every .cpp file that includes a ui_ file but doesn't have a .ui file of the same name
set_property(SOURCE tests/test_mainwindow.cpp PROPERTY SKIP_AUTOUIC ON)

#create the executable
add_executable(SCE ${SCE_SRC} ${PROTO_SRCS} ${PROTO_HDRS} sce.grpc.pb.cc  sce.grpc.pb.h)

# Link executable
# Debian is fine with "util grpc++", but apparently Ubuntu wants all the libraries spelled out.
# On a proper system that packages grpc $ENV{GRPC_LIBS} should be empty.
# Otherwise compile from source and use `export GRPC_LIBS=$(sh print_libs.sh)` to get a list of the compiled libs.
#message("GRPC libs: " $ENV{GRPC_LIBS})
target_link_libraries(SCE Qt5::Widgets ${PROTOBUF_LIBRARIES} util grpc++ $ENV{GRPC_LIBS} $ENV{GRPC_LIBS})

# Install the executable
#install(TARGETS SCE DESTINATION bin)
