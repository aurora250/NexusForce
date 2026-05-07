add_executable(NexusForceUnitTests
        main.cpp
        test.cpp
        test_algorithm.cpp
        test_compress.cpp
        test_container.cpp
        test_encrypt.cpp
        test_file.cpp
        test_functional.cpp
        test_memory.cpp
        test_numeric.cpp
        test_string.cpp
        test_system.cpp
        test_time.cpp
        test_utility.cpp
)

set_target_properties(NexusForceUnitTests PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin/Debug
        RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin/Release
        RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/bin/RelWithDebInfo
        RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_BINARY_DIR}/bin/MinSizeRel
)

target_link_libraries(NexusForceUnitTests PRIVATE NexusForce::NexusForce)
if(MSVC)
    target_compile_options(NexusForceUnitTests PRIVATE /bigobj)
endif()
nexusforce_compiler_options(NexusForceUnitTests)

find_package(GTest CONFIG REQUIRED)
target_link_libraries(NexusForceUnitTests PRIVATE GTest::gtest)

enable_testing()
add_test(NAME NexusForceUnitTests COMMAND NexusForceUnitTests)
