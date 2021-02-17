# Copyright 2018 VMware
# Copyright 2018 Ted Yin
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

cmake_minimum_required(VERSION 3.9)
project(hotstuff)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/salticidae/cmake/Modules/")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules)

# BLS extra:
set(sodium_USE_STATIC_LIBS TRUE)
find_package(sodium)
if (SODIUM_FOUND)
    message(STATUS "Found libsodium")
    message(STATUS "Sodium include dir = ${sodium_INCLUDE_DIR}")
    set(BLSALLOC_SODIUM "1" CACHE STRING "")
    set(SODIUM_STATIC "1" CACHE STRING "")
    include_directories(${sodium_INCLUDE_DIR})
endif()

set(STBIN TRUE)
find_package(gmp)
if (GMP_FOUND)
    message(STATUS "Found libgmp")
    include_directories(${GMP_INCLUDE_DIR})
    set(ARITH "gmp" CACHE STRING "")
else()
    set(ARITH "easy" CACHE STRING "")
endif()

set(TIMER "CYCLE" CACHE STRING "")
set(CHECK "off" CACHE STRING "")
set(VERBS "off" CACHE STRING "")
set(ALLOC "AUTO" CACHE STRING "")
set(SHLIB "OFF" CACHE STRING "")
set(MULTI "PTHREAD" CACHE STRING "")
set(DOCUM "off" CACHE STRING "")

set(FP_PRIME 381 CACHE INTEGER "")

IF (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(DSEED "UDEV" CACHE STRING "")
    set(FP_QNRES "off" CACHE STRING "")
ELSEIF (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    set(SEED "WCGR" CACHE STRING "")
    set(FP_QNRES "on" CACHE STRING "")
ELSE()
    set(DSEED "UDEV" CACHE STRING "")
    set(FP_QNRES "on" CACHE STRING "")
ENDIF()
set(STBIN "OFF" CACHE STRING "")

set(FP_METHD "INTEG;INTEG;INTEG;MONTY;LOWER;SLIDE" CACHE STRING "")
set(COMP "-O3 -funroll-loops -fomit-frame-pointer" CACHE STRING "")
set(FP_PMERS "off" CACHE STRING "")
set(FPX_METHD "INTEG;INTEG;LAZYR" CACHE STRING "")
set(EP_PLAIN "off" CACHE STRING "")
set(EP_SUPER "off" CACHE STRING "")
# Disable relic tests and benchmarks
set(TESTS 0 CACHE INTEGER "")
set(BENCH 0 CACHE INTEGER "")
set(QUIET 1 CACHE INTEGER "")

set(PP_EXT "LAZYR" CACHE STRING "")
set(PP_METHD "LAZYR;OATEP" CACHE STRING "")

add_subdirectory(bls/src)

include(FetchContent)

FetchContent_Declare(
        relic
        GIT_REPOSITORY https://github.com/relic-toolkit/relic.git
        GIT_TAG        ed485dccb23f9f5615eac5c71b3e37bd1bc8fb3e
)
FetchContent_MakeAvailable(relic)

include_directories(
        ${INCLUDE_DIRECTORIES}
        salticidae/include
        ${relic_SOURCE_DIR}/include
        ${relic_BINARY_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../contrib/catch
        ${GMP_INCLUDES}
)

# ---------------------------

add_subdirectory(salticidae)

find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)

include(ExternalProject)
include_directories(secp256k1/include)
ExternalProject_Add(libsecp256k1
    SOURCE_DIR secp256k1
    CONFIGURE_COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/secp256k1/autogen.sh
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/secp256k1/configure --disable-shared --with-pic --with-bignum=no --enable-module-recovery
    BUILD_COMMAND make
    INSTALL_COMMAND ""
    BUILD_IN_SOURCE 1)

add_library(secp256k1 STATIC IMPORTED)
set_target_properties(
    secp256k1
    PROPERTIES IMPORTED_LOCATION
    ${CMAKE_CURRENT_SOURCE_DIR}/secp256k1/.libs/libsecp256k1.a)
add_dependencies(secp256k1 libsecp256k1)

# add libraries

include_directories(./)
include_directories(include)
add_library(hotstuff
    OBJECT
    src/util.cpp
    src/client.cpp
    src/crypto.cpp
    src/entity.cpp
    src/consensus.cpp
    src/hotstuff.cpp
)

add_library(hotstuff_static STATIC $<TARGET_OBJECTS:hotstuff>)
set_target_properties(hotstuff_static PROPERTIES OUTPUT_NAME "hotstuff")
target_link_libraries(hotstuff_static PRIVATE salticidae_static secp256k1 crypto ${CMAKE_THREAD_LIBS_INIT} ${GMP_LIBRARIES} ${GMPXX_LIBRARIES} blstmp relic_s pthread sodium)

add_subdirectory(test)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "Release")
endif()

option(HOTSTUFF_DEBUG_LOG "enable debug log" OFF)
option(HOTSTUFF_NORMAL_LOG "enable normal log" ON)
option(HOTSTUFF_PROTO_LOG "enable protocol log" OFF)
option(HOTSTUFF_MSG_STAT "eanble message statistics" ON)
option(HOTSTUFF_BLK_PROFILE "enable block profiling" OFF)
option(HOTSTUFF_TWO_STEP "use two-step HotStuff (instead of three-step HS)" OFF)
option(BUILD_EXAMPLES "build examples" ON)

configure_file(src/config.h.in include/hotstuff/config.h @ONLY)

# build examples
if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# build tools
add_executable(hotstuff-keygen
    src/hotstuff_keygen.cpp)
target_link_libraries(hotstuff-keygen hotstuff_static blstmp relic_s relic_s pthread sodium)

add_executable(hotstuff-tls-keygen
    src/hotstuff_tls_keygen.cpp)
target_link_libraries(hotstuff-tls-keygen hotstuff_static blstmp relic_s pthread sodium)

find_package(Doxygen)
if (DOXYGEN_FOUND)
    add_custom_target(doc
        ${DOXYGEN_EXECUTABLE} doc/doxygen.conf WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
endif(DOXYGEN_FOUND)

#set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -no-pie -pg")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -W -Wall -Wextra -pedantic -Wsuggest-override")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -W -Wall -Wextra -pedantic -Wsuggest-override")

macro(remove_cxx_flag flag)
  string(REPLACE "${flag}" "" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
endmacro()

remove_cxx_flag("-DNDEBUG")
