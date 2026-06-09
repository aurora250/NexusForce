if(NOT NEXUSFORCE_BUILD_EXAMPLES)
    message(STATUS "Skipping examples build (NEXUSFORCE_BUILD_EXAMPLES=OFF)")
    return()
endif()

# Shared macro — all example subdirectories use this to eliminate boilerplate
macro(add_nexusforce_example NAME)
    add_executable(${NAME} ${ARGN})
    target_link_libraries(${NAME} PRIVATE NexusForce::NexusForce)
    nexusforce_compiler_options(${NAME})
endmacro()

add_subdirectory(db)
add_subdirectory(logging)
add_subdirectory(network)
