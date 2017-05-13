# Minimum CMake version required
cmake_minimum_required(VERSION 2.8)

option(BUILD_PEER "Build the peer" ON)
option(BUILD_MONITOR "Build the monitor" ON)
option(BUILD_SPLITTER "Build the splitter" ON)
option(BUILD_SYNCHRONIZER "Build the synchronizer" ON)
option(BUILD_INCORPORATION_TEST "Build binaries with incorporation test flag" OFF)

option(BUILD_WITH_CLANG "Build using the Clang compiler" OFF)

if(BUILD_WITH_CLANG)
  set(CMAKE_C_COMPILER clang)
  set(CMAKE_CXX_COMPILER clang++)
endif()

# Project name
project(p2psp-console C CXX)

list(APPEND BOOST_REQUIRED_COMPONENTS
  system
  filesystem
  chrono
  serialization
  timer
  thread
  log
  log_setup
  date_time
  program_options
)

if(APPLE)
  set(CMAKE_SHARED_LIBRARY_SUFFIX ".so")
  set(OPENSSL_INCLUDE_DIR "/usr/local/opt/openssl/include")
endif(APPLE)

find_package(Boost COMPONENTS
  ${BOOST_REQUIRED_COMPONENTS}
REQUIRED)

ADD_DEFINITIONS(-DBOOST_LOG_DYN_LINK)

find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)

set(P2PSP_INCLUDE_DIR ../core/src/)

include_directories(
  ${P2PSP_INCLUDE_DIR}
  ${Boost_INCLUDE_DIR}
  ${OPENSSL_INCLUDE_DIR}
)

link_directories(${Boost_LIBRARY_DIR})

#Find P2PSP Library
#FIND_LIBRARY(P2PSP_LIBRARY p2psp lib/p2psp/bin/ lib/p2psp/src/core/)
FIND_LIBRARY(P2PSP_LIBRARY p2psp ../core/bin/ ../core/src/)

# Initialize CXXFLAGS
set(CMAKE_CXX_FLAGS                "-Wall -std=c++11")
set(CMAKE_CXX_FLAGS_DEBUG          "-O0 -g")
#set(CMAKE_CXX_FLAGS_DEBUG          "-O0 -g -D__D__ -D__D_CHURN__ -D__D_LOST_CHUNKS__")
#set(CMAKE_CXX_FLAGS_MINSIZEREL     "-Os")
set(CMAKE_CXX_FLAGS_MINSIZEREL     "-Os")
#set(CMAKE_CXX_FLAGS_RELEASE        "-O4")
set(CMAKE_CXX_FLAGS_RELEASE        "-O4")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g")

# Compiler-specific C++11 activation
if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU")
    execute_process(
        COMMAND ${CMAKE_CXX_COMPILER} -dumpversion OUTPUT_VARIABLE GCC_VERSION)
    if (NOT (GCC_VERSION VERSION_GREATER 4.7 OR GCC_VERSION VERSION_EQUAL 4.7))
        message(FATAL_ERROR "${PROJECT_NAME} requires g++ 4.7 or greater")
    endif ()
elseif ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
else ()
    message(FATAL_ERROR "Your C++ compiler does not support C++11")
endif ()

# Sources

if(BUILD_PEER)
  # Executable
  add_executable(peer "src/peer.cc")
  # Defines
  if(BUILD_INCORPORATION_TEST)
    target_compile_definitions(peer PUBLIC __incorporation_test__)
  endif()

  # Libraries
  target_link_libraries(peer
    ${P2PSP_LIBRARY}
    ${CMAKE_THREAD_LIBS_INIT}
    ${Boost_LIBRARIES}
  )

  # Enable C++11 for CMake 3.1.0 or later
  set_property(TARGET peer PROPERTY CXX_STANDARD 11)
  set_property(TARGET peer PROPERTY CXX_STANDARD_REQUIRED ON)
endif()

if(BUILD_MONITOR)
  # Executable
  add_executable(monitor "src/peer.cc")

  # Defines
  #add_definitions(-D__monitor__)
  target_compile_definitions(monitor PUBLIC __monitor__)
  if(BUILD_INCORPORATION_TEST)
    target_compile_definitions(monitor PUBLIC __incorporation_test__)
  endif()

  # Libraries
  target_link_libraries(monitor
    ${P2PSP_LIBRARY}
    ${CMAKE_THREAD_LIBS_INIT}
    ${Boost_LIBRARIES}
  )

  # Enable C++11 for CMake 3.1.0 or later
  set_property(TARGET monitor PROPERTY CXX_STANDARD 11)
  set_property(TARGET monitor PROPERTY CXX_STANDARD_REQUIRED ON)
endif()

if(BUILD_SPLITTER)
  # Executable
  add_executable(splitter "src/splitter.cc")
  # Defines
  if(BUILD_INCORPORATION_TEST)
    target_compile_definitions(splitter PUBLIC __incorporation_test__)
  endif()

  # Libraries
  target_link_libraries(splitter
    ${P2PSP_LIBRARY}
    ${CMAKE_THREAD_LIBS_INIT}
    ${Boost_LIBRARIES}
  )

  # Enable C++11 for CMake 3.1.0 or later
  set_property(TARGET splitter PROPERTY CXX_STANDARD 11)
  set_property(TARGET splitter PROPERTY CXX_STANDARD_REQUIRED ON)
endif()

if(BUILD_SYNCHRONIZER)
  # Executable
  add_executable(synchronizer "src/synchronizer.cc")

  # Libraries
  target_link_libraries(synchronizer
    ${P2PSP_LIBRARY}
    ${CMAKE_THREAD_LIBS_INIT}
    ${Boost_LIBRARIES}
  )

  # Enable C++11 for CMake 3.1.0 or later
  set_property(TARGET synchronizer PROPERTY CXX_STANDARD 11)
  set_property(TARGET synchronizer PROPERTY CXX_STANDARD_REQUIRED ON)
endif()
