if(NOT NEXUSFORCE_BUILD_BENCHMARKS)
    message(STATUS "Skipping benchmarks build (NEXUSFORCE_BUILD_BENCHMARKS=OFF)")
    return()
endif()

find_package(benchmark CONFIG QUIET)
if(NOT TARGET benchmark::benchmark)
    message(WARNING "Google Benchmark not found — skipping benchmark build.")
    return()
endif()

message(STATUS "Google Benchmark found — building benchmarks")

macro(add_nexusforce_benchmark NAME)
    add_executable(${NAME} ${ARGN})
    target_link_libraries(${NAME} PRIVATE NexusForce::NexusForce benchmark::benchmark)
    nexusforce_compiler_options(${NAME})
endmacro()
