cmake_minimum_required(VERSION 3.16)
project(plagiarism_detector CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── Compiler flags ──
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -O2)
endif()

# ── Source files ──
set(SOURCES
    src/tokenizer.cpp
    src/suffix_tree.cpp
    src/checker.cpp
    src/patchwork_detector.cpp
    src/reporter.cpp
    src/main.cpp
)

add_executable(plagcheck ${SOURCES})

target_include_directories(plagcheck PRIVATE include)

# Link pthreads for multi-threading
find_package(Threads REQUIRED)
target_link_libraries(plagcheck PRIVATE Threads::Threads)

# ── Tests (optional) ──
option(BUILD_TESTS "Build test executables" OFF)
if(BUILD_TESTS)
    add_subdirectory(tests)
endif()
