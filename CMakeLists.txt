cmake_minimum_required(VERSION 3.1)
project(SCE)

set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 17)

# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)

# Find the QtWidgets library
find_package(Qt5Widgets CONFIG REQUIRED)

# Boost
find_package(Boost 1.54 COMPONENTS system REQUIRED)
INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIR})

# Set up protobuffer
if (NOT DEFINED ENV{TRAVIS})
	set(USR_PATH "/usr")
else()
	set(USR_PATH "/usr/local")
endif()

set(CMAKE_MODULE_PATH "${USR_PATH}/lib/grpc/cmake/;${USR_PATH}/lib/protobuf/cmake/;${CMAKE_MODULE_PATH}")
find_package(Protobuf REQUIRED)
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
	COMMENT "Generating RPC code for sce.proto."
)

#extra flags
if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU" OR "${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
	#general warnings
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror")
	#special warnings for gcc
	if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unknown-pragmas -Wodr -Wshadow -Wredundant-decls -Wcast-align -Wmissing-declarations -Wmissing-include-dirs -Wredundant-decls -Wformat=2 -Wmissing-format-attribute -Wformat-nonliteral -Wlogical-op -Warray-bounds=2 -Wimplicit-fallthrough=5 -Walloc-zero -Wduplicated-branches -Wduplicated-cond -Wplacement-new=2 -Wrestrict -Wold-style-cast -Wzero-as-null-pointer-constant -Wformat=2 -Wformat-overflow=2 -Wformat-truncation=2 -Wformat-signedness -Wsuggest-attribute=noreturn -Wsuggest-attribute=format -Wsuggest-final-methods")
	endif()
	#special warnings for clang
	if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Weverything -Wno-c++98-compat -Wno-c++98-compat-pedantic -Wno-newline-eof -Wno-padded -Wno-sign-conversion -Wno-exit-time-destructors -Wno-global-constructors -Wno-weak-vtables -Wno-c99-extensions -Wno-shorten-64-to-32 -Wno-range-loop-analysis -Wno-switch-enum -Wno-old-style-cast -Wodr -Wshadow -Wredundant-decls -Wcast-align -Wmissing-declarations -Wmissing-include-dirs -Wredundant-decls -Wformat=2 -Wmissing-format-attribute -Wformat-nonliteral -Warray-bounds -Wimplicit-fallthrough -Wduplicate-enum -Wold-style-cast -Wformat=2")
	endif()
	#disable annoying warnings
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-parameter -Wno-missing-braces")
	#stop clang from complaining about unused flags. TODO: figure out why those flags are considered unused.
	if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-command-line-argument")
	endif()
	#extra debug flags
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer -ggdb -O0")
	#environment flags
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} $ENV{CXX_FLAGS_DEBUG}")
	set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} $ENV{CXX_LFLAGS_DEBUG}")
	#release flags
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DNDEBUG")
endif()

#defines
add_definitions(-DTEST_DATA_PATH=\"${CMAKE_SOURCE_DIR}/testdata/\")

# Source files
set(SCE_SRC
	interop/notification_server.cpp
	interop/rpc_server.cpp
	logic/process_reader.cpp
	logic/settings.cpp
	logic/syntax_highligher.cpp
	logic/tool.cpp
	logic/tool_actions.cpp
	main.cpp
	tests/test.cpp
	tests/test_mainwindow.cpp
	tests/test_notification_server.cpp
	tests/test_process_reader.cpp
	tests/test_rpc_server.cpp
	tests/test_settings.cpp
	tests/test_tool.cpp
	tests/test_tool_editor_widget.cpp
	ui/edit_window.cpp
	ui/mainwindow.cpp
	ui/tool_editor_widget.cpp
	utility/color.cpp
	utility/pipe.cpp
	utility/raii.cpp
	utility/thread_call.cpp
	utility/thread_safe.cpp
	utility/unique_handle.cpp
)

# Create code from a list of Qt designer ui files.
set(CMAKE_AUTOUIC ON)
#disable autouic for every .cpp file that includes a ui_ file but doesn't have a .ui file of the same name
set_property(SOURCE tests/test_mainwindow.cpp PROPERTY SKIP_AUTOUIC ON)

# Find includes in corresponding build directories
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -isystem ${CMAKE_CURRENT_SOURCE_DIR} -isystem ${CMAKE_CURRENT_BINARY_DIR}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -isystem SCE_autogen/include")
set(CMAKE_INCLUDE_CURRENT_DIR ON) #this line is only for qt creator's ClangCodeModel plugin

#disable warnings for generated protobuf and moc files
set_source_files_properties(sce.pb.cc sce.grpc.pb.cc SCE_autogen/mocs_compilation.cpp PROPERTIES COMPILE_FLAGS -w)

#create the executable
add_executable(${PROJECT_NAME} ${SCE_SRC} ${PROTO_SRCS} ${PROTO_HDRS} sce.grpc.pb.cc  sce.grpc.pb.h)

# Manuall list all .ui files so that Qt Creator lists them in the projects view
set(UI_FILES
	ui/mainwindow.ui
	ui/tool_editor_widget.ui
)
target_sources(${PROJECT_NAME} PUBLIC ${UI_FILES})

# Other files one would want to edit that are not directly part of the SCE build
set(MISC_FILES
	.clang-format
	.gitignore
	.travis.yml
	CMakeLists.txt
	README.md
	docs/description.md
	docs/next_steps.md
	feature_plans.md
	gdb.sh
	list_libs.sh
	lsan.supp
	run_all_test_configurations.sh
)
target_sources(${PROJECT_NAME} PUBLIC ${MISC_FILES})

# Link executable
# Debian is fine with "util grpc++", but apparently Ubuntu wants all the libraries spelled out.
# On a proper system that packages grpc $ENV{GRPC_LIBS} should be empty.
# Otherwise compile from source and use `export GRPC_LIBS=$(sh print_libs.sh)` to get a list of the compiled libs.
#message("GRPC libs: " $ENV{GRPC_LIBS})
target_link_libraries(${PROJECT_NAME}
	Qt5::Widgets
	${Boost_LIBRARIES}
	${PROTOBUF_LIBRARIES}
	util
	grpc++
	$ENV{GRPC_LIBS} $ENV{GRPC_LIBS} #appears twice to avoid library order issues
)

# Install the executable
#install(TARGETS SCE DESTINATION bin)
