if(NOT NEXUSFORCE_BUILD_TESTS)
    message(STATUS "Skipping tests build (NEXUSFORCE_BUILD_TESTS=OFF)")
    return()
endif()

find_package(GTest CONFIG QUIET)
if(NOT TARGET GTest::gtest)
    message(WARNING "GTest not found — skipping test build. Install GTest or enable the 'gtest' vcpkg feature.")
    return()
endif()

message(STATUS "GTest found — building tests")

add_subdirectory(unit)
add_subdirectory(integration)
