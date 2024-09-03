cmake_minimum_required(VERSION 3.22)
set(CMAKE_C_COMPILER "/usr/bin/clang-18")
set(CMAKE_CXX_COMPILER "/usr/bin/clang++-18")
SET(CMAKE_TOOLCHAIN_FILE "/home/fanghz/lib/vcpkg/scripts/buildsystems/vcpkg.cmake")
project(Agent)
set(CMAKE_CXX_STANDARD 20)
# SET(CMAKE_BUILD_TYPE "Debug")
# SET(CMAKE_BUILD_TYPE "Release")
find_package(Threads REQUIRED)
find_package(absl CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(TBB CONFIG REQUIRED)

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
include_directories(${PROJECT_SOURCE_DIR}/src)
add_executable(match_engine main.cc dsp_channel.cc dsp_message.cc txn_recoder.cc)

# if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
#     target_compile_options(example PRIVATE -O3 -march=native -fopt-info-vec-optimized)
# elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
#     target_compile_options(example PRIVATE -O3 -march=native -Rpass=loop-vectorize -Rpass-analysis=loop-vectorize -Rpass-missed=loop-vectorize)
# endif ()

# if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
#     target_compile_options(filter_sz PRIVATE -O3 -march=native -fopt-info-vec-optimized)
# elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
#     target_compile_options(filter_sz PRIVATE -O3 -march=native)
# endif ()

target_link_libraries(match_engine ${absl_LIBRARIES})
target_link_libraries(match_engine fmt::fmt)
target_link_libraries(match_engine PkgConfig::liburing)
target_link_libraries(match_engine TBB::tbb TBB::tbbmalloc)
target_link_libraries(match_engine Folly::folly Folly::folly_deps Folly::follybenchmark Folly::folly_test_util)
target_link_libraries(match_engine xsimd)
target_link_libraries(match_engine cppzmq cppzmq-static)
