add_executable(NexusForceTests
        main.cpp
        test_file.cpp
        test_base.cpp
        test_db.cpp
        test_network.cpp
        test_system.cpp
        test_reflect.cpp
        test.cpp
        test.h
)

set_target_properties(NexusForceTests PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin/Debug
        RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin/Release
        RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/bin/RelWithDebInfo
        RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_BINARY_DIR}/bin/MinSizeRel
)

target_link_libraries(NexusForceTests PRIVATE NexusForce::NexusForce)
nexusforce_compiler_options(NexusForceTests)

find_package(GTest CONFIG REQUIRED)
target_link_libraries(NexusForceTests PRIVATE GTest::gtest GTest::gtest_main)

enable_testing()
add_test(NAME NexusForceTests COMMAND NexusForceTests)
