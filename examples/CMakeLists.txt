cmake_minimum_required(VERSION 3.1.15)
project(OrbbecSDK-Samples LANGUAGES CXX C)

if(UNIX)
  set(CMAKE_CXX_FLAGS "-std=c++11 -pthread ${CMAKE_CXX_FLAGS}")
endif()

if(NOT DEFINED OrbbecSDK_FOUND)
    set(OrbbecSDK_DIR "D:/02_zz_gitlab/14-tof-guangban-test/OrbbecSDK/lib")
    message(STATUS "OrbbecSDK_DIR: ${OrbbecSDK_DIR}")
    find_package(OrbbecSDK REQUIRED)
endif()

# find opencv
set(OpenCV_DIR "E:/02-software-install/02-opencv/opencv/build/x64/vc16/lib")
find_package(OpenCV QUIET)

if(UNIX)
    set(CMAKE_SKIP_BUILD_RPATH FALSE)
    set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
    set(CMAKE_INSTALL_RPATH "$ORIGIN:$ORIGIN/../lib")
    set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
endif()

# add_subdirectory(c)
add_subdirectory(cpp)

install(DIRECTORY
  ${OrbbecSDK_LIBS_DIR}/ DESTINATION
  bin
  FILES_MATCHING PATTERN "*.so"
  PATTERN "*.so.*"
  PATTERN "*.dylib"
  PATTERN "*.*.dylib"
  PATTERN "*.dll"
  PATTERN ".lib"
  )
