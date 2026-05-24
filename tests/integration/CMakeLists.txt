add_executable(NexusForceIntegrationTest
        main.cpp
        test_db.cpp
        test_dns_client.cpp
        test_http.cpp
        test_network.cpp
        test_network_socket.cpp
        test_tcp_socket.cpp
        test_ssl_socket.cpp
)

set_target_properties(NexusForceIntegrationTest PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin/Debug
        RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin/Release
        RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/bin/RelWithDebInfo
        RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_BINARY_DIR}/bin/MinSizeRel
)

target_link_libraries(NexusForceIntegrationTest PRIVATE NexusForce::NexusForce)
nexusforce_compiler_options(NexusForceIntegrationTest)

find_package(GTest CONFIG REQUIRED)
target_link_libraries(NexusForceIntegrationTest PRIVATE GTest::gtest)

enable_testing()
add_test(NAME NexusForceIntegrationTest COMMAND NexusForceIntegrationTest)
