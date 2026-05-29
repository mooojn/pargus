cmake_minimum_required(VERSION 3.16)

project(Pargus C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

add_executable(Pargus
    main.c
    common/string_utils.c
    common/timing.c
    config/args.c
    io/reader.c
    io/writer.c
)

target_include_directories(Pargus PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

find_package(OpenMP)
if(OpenMP_C_FOUND)
    target_link_libraries(Pargus PRIVATE OpenMP::OpenMP_C)
    target_compile_definitions(Pargus PRIVATE PARGUS_HAS_OPENMP=1)
endif()

find_package(Threads REQUIRED)
target_link_libraries(Pargus PRIVATE Threads::Threads)

if(MSVC)
    target_compile_options(Pargus PRIVATE /W4)
else()
    target_compile_options(Pargus PRIVATE -Wall -Wextra -Wpedantic)
endif()

