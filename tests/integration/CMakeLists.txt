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

target_link_libraries(NexusForceIntegrationTest PRIVATE NexusForce::NexusForce)
nexusforce_compiler_options(NexusForceIntegrationTest)
target_link_libraries(NexusForceIntegrationTest PRIVATE GTest::gtest)

enable_testing()
add_test(NAME NexusForceIntegrationTest COMMAND NexusForceIntegrationTest)
