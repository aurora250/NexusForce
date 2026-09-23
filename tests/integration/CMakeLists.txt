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

if(NEXUSFORCE_USING_MEMORY_POOL)
    add_executable(NexusForceAllocatorOverrideTest
            main.cpp
            test_allocator_override.cpp
            $<TARGET_OBJECTS:NexusForceMemoryPoolOverride>
    )
    target_link_libraries(NexusForceAllocatorOverrideTest PRIVATE NexusForce::NexusForce GTest::gtest)
    nexusforce_compiler_options(NexusForceAllocatorOverrideTest)
    add_test(NAME NexusForceAllocatorOverrideTest COMMAND NexusForceAllocatorOverrideTest)
    set_target_properties(NexusForceAllocatorOverrideTest PROPERTIES FOLDER "tests")
endif()
