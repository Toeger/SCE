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
find_package(Protobuf REQUIRED)
include_directories(${CMAKE_CURRENT_BINARY_DIR})
PROTOBUF_GENERATE_CPP(PROTO_SRCS PROTO_HDRS interop/sce.proto)

# Warnings
if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU" OR "${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
    #general warnings
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror")
    #disable warning for generated protobuf file
    set_source_files_properties(sce.pb.cc PROPERTIES COMPILE_FLAGS -Wno-unused-parameter)
endif()

# Sanitizers
if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU" OR "${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=undefined,address")
    set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} -fsanitize=undefined,address")
endif()

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
#set(CMAKE_AUTOUIC ON) # use this if you have CMake 3.x instead of the following
# UI files
qt5_wrap_ui(SCE_SRC
    ui/mainwindow.ui
    ui/tool_editor_widget.ui
)

# Tell CMake to create the executable
add_executable(SCE ${SCE_SRC} ${PROTO_SRCS} ${PROTO_HDRS}) # don't know if ${PROTO_HDRS} belongs here

# Use the Widgets module from Qt 5.
target_link_libraries(SCE Qt5::Widgets ${PROTOBUF_LIBRARIES} util)

# Install the executable
#install(TARGETS SCE DESTINATION bin)
