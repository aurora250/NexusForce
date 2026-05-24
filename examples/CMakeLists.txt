if(NOT NEXUSFORCE_BUILD_EXAMPLES)
    message(STATUS "Skipping examples build (NEXUSFORCE_BUILD_EXAMPLES=OFF)")
    return()
endif()

add_subdirectory(network)
