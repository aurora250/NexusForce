if(NOT NEXUSFORCE_BUILD_TESTS)
    message(STATUS "Skipping tests build (NEXUSFORCE_BUILD_TESTS=OFF)")
    return()
endif()

add_subdirectory(unit)
add_subdirectory(integration)
