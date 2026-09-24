add_executable(NexusForceIntegrationTest
        main.cpp
        test_db.cpp
        test_dns_client.cpp
        test_http.cpp
        test_network.cpp
        test_network_socket.cpp
        test_ssl_socket.cpp
        test_tcp_socket.cpp
        test_tui.cpp
)

target_link_libraries(NexusForceIntegrationTest PRIVATE NexusForce::NexusForce)
nexusforce_compiler_options(NexusForceIntegrationTest)
target_link_libraries(NexusForceIntegrationTest PRIVATE GTest::gtest)

enable_testing()
add_test(NAME NexusForceIntegrationTest COMMAND NexusForceIntegrationTest)
set_target_properties(NexusForceIntegrationTest PROPERTIES FOLDER "tests")

if(NEXUSFORCE_MEMORY_POOL_GLOBAL_OVERRIDE)
    set(_nexusforce_override_test_ready ON)
    if(NEXUSFORCE_GTEST_IS_SHARED AND WIN32 AND NOT NEXUSFORCE_OVERRIDE_TEST_WITH_SHARED_GTEST)
        set(_nexusforce_override_test_ready OFF)
        message(STATUS
                "Skipping NexusForceAllocatorOverrideTest: GTest is a shared library, so the process would mix "
                "the memory pool with the C runtime heap. Point -DNEXUSFORCE_GOOGLETEST_SOURCE_DIR at a "
                "googletest source tree, or pass -DNEXUSFORCE_OVERRIDE_TEST_WITH_SHARED_GTEST=ON to build it anyway.")
    elseif(NEXUSFORCE_GTEST_IS_SHARED AND WIN32)
        message(WARNING
                "NexusForceAllocatorOverrideTest replaces operator new in the test executable while gtest.dll "
                "keeps the C runtime heap; the debug heap rejects blocks crossing that boundary. "
                "Build the tests against a static GTest (e.g. an x64-windows-static-md install) to run it.")
    endif()
    if(NOT _nexusforce_override_test_ready)
        return()
    endif()
    add_executable(NexusForceAllocatorOverrideTest
            main.cpp
            test_allocator_override.cpp
            $<TARGET_OBJECTS:NexusForceMemoryPoolOverride>
    )
    target_link_libraries(NexusForceAllocatorOverrideTest PRIVATE NexusForce::NexusForce GTest::gtest)
    target_compile_definitions(NexusForceAllocatorOverrideTest PRIVATE NEFORCE_USING_MEMORY_POOL_OVERRIDE)
    nexusforce_compiler_options(NexusForceAllocatorOverrideTest)
    add_test(NAME NexusForceAllocatorOverrideTest COMMAND NexusForceAllocatorOverrideTest)
    set_target_properties(NexusForceAllocatorOverrideTest PROPERTIES FOLDER "tests")
endif()
