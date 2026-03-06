cmake_minimum_required(VERSION 3.16)
project(QtRtspTcpDemo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

option(BUILD_CLIENT "Build Qt client" ON)
option(BUILD_SERVER "Build std:: server" ON)
option(BUILD_MEDIA_CORE "Build standalone media core/test" ON)
option(BUILD_TESTS "Build C++ tests" ON)

add_subdirectory(src/common)

if(BUILD_MEDIA_CORE)
  add_subdirectory(src/media_core)
endif()

if(BUILD_SERVER)
  add_subdirectory(src/server)
endif()

if(BUILD_CLIENT)
  add_subdirectory(src/client)
endif()

if(BUILD_TESTS)
  add_subdirectory(tests)
endif()
