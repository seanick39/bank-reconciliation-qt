cmake_minimum_required(VERSION ${CMAKE_MINIMUM_REQUIRED_VERSION})
project(bank-reconc-lib)

set(CMAKE_CXX_STANDARD 20) # usage of std::find_if in algorithm avbl in stdc++ 20 
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

message("win32: ${WIN32}")

include_directories(BOOST_ROOT)

if (WIN32)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS 1)
endif ()

file(GLOB_RECURSE BRLIB_HEADERS "*.h")
file(GLOB_RECURSE BRLIB_SOURCES "*.cpp")

add_library(bank-reconc-lib SHARED ${BRLIB_HEADERS} ${BRLIB_SOURCES})

target_include_directories(bank-reconc-lib PUBLIC BOOST_ROOT)