cmake_minimum_required(VERSION 3.22)
set(CMAKE_C_COMPILER "/usr/bin/clang-18")
set(CMAKE_CXX_COMPILER "/usr/bin/clang++-18")
SET(CMAKE_TOOLCHAIN_FILE "/home/fanghz/lib/vcpkg/scripts/buildsystems/vcpkg.cmake")
project(Agent)
set(CMAKE_CXX_STANDARD 20)
SET(CMAKE_BUILD_TYPE "Debug")
# SET(CMAKE_BUILD_TYPE "Release")
find_package(Threads REQUIRED)
find_package(absl CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(TBB CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)

set(absl_LIBRARIES
absl::algorithm
absl::base
absl::debugging
absl::flat_hash_map
absl::memory
absl::meta
absl::numeric
absl::str_format
absl::strings
absl::synchronization
absl::time
absl::flags
absl::strings
absl::log
absl::flags_internal
absl::log_initialize
absl::config
absl::base
absl::flags_parse
absl::utility)

set(PROTO_DIR ${CMAKE_SOURCE_DIR}/proto_gen)
include_directories(${CMAKE_SOURCE_DIR}/proto_gen)
file(GLOB PROTO_SOURCES ${PROTO_DIR}/*.cc)

file(GLOB SOURCES ${CMAKE_SOURCE_DIR}/*.cc)

add_executable(match_engine ${SOURCES} ${PROTO_SOURCES})

target_link_libraries(match_engine ${absl_LIBRARIES})
target_link_libraries(match_engine fmt::fmt)
target_link_libraries(match_engine gRPC::gpr gRPC::grpc gRPC::grpc++ gRPC::grpc++_alts)
target_link_libraries(match_engine TBB::tbb TBB::tbbmalloc)
target_link_libraries(match_engine spdlog::spdlog_header_only)
