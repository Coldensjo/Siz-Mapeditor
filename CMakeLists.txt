cmake_minimum_required(VERSION 2.8)

project(rme)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    add_compile_options(-w)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    add_compile_options(/W0)
endif()

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

set(CMAKE_CXX_FLAGS                "-Wall -Wextra -Wno-unused-parameter -Wno-unused-variable -Wno-deprecated-declarations -Wno-overloaded-virtual -std=c++0x -Wno-strict-aliasing -Wno-sign-compare -Wno-unused-function -Wunused-result")
set(CMAKE_CXX_FLAGS_DEBUG          "-O0 -g -D__DEBUG__")
set(CMAKE_CXX_FLAGS_MINSIZEREL     "-Os -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE        "-O4 -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g")

find_package(OpenGL REQUIRED)

if(APPLE)
    set(CMAKE_PREFIX_PATH /usr/local/opt/libarchive)
endif()
find_package(LibArchive REQUIRED)

if(WIN32)
    set(Boost_THREADAPI win32)
endif()
find_package(Boost 1.34.0 COMPONENTS thread system REQUIRED)

find_package(wxWidgets COMPONENTS html aui gl adv core net base REQUIRED)

find_package(GLUT REQUIRED)

find_package(tomlplusplus CONFIG REQUIRED)

include(${wxWidgets_USE_FILE})
include(source/CMakeLists.txt)
add_executable(rme ${rme_H} ${rme_SRC})

include_directories(${Boost_INCLUDE_DIRS} ${LibArchive_INCLUDE_DIRS} ${OPENGL_INCLUDE_DIR} ${GLUT_INCLUDE_DIRS})
target_link_libraries(rme ${wxWidgets_LIBRARIES} ${Boost_LIBRARIES} ${LibArchive_LIBRARIES} ${OPENGL_LIBRARIES} ${GLUT_LIBRARY} tomlplusplus::tomlplusplus)
