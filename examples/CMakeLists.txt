if(NOT NEXUSFORCE_BUILD_EXAMPLES)
    message(STATUS "Skipping examples build (NEXUSFORCE_BUILD_EXAMPLES=OFF)")
    return()
endif()

macro(add_nexusforce_example NAME)
    add_executable(${NAME} ${ARGN})
    target_link_libraries(${NAME} PRIVATE NexusForce::NexusForce)
    nexusforce_compiler_options(${NAME})
    set_target_properties(${NAME} PROPERTIES FOLDER "examples")
endmacro()

add_subdirectory(db)
add_subdirectory(logging)
add_subdirectory(network)
add_subdirectory(tui)

add_subdirectory(integration)
