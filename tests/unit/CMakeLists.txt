add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/_nfrs_test_gen.cpp
    COMMAND $<TARGET_FILE:NFRS> ${CMAKE_CURRENT_SOURCE_DIR}/nfrs_test_data
            -o ${CMAKE_CURRENT_BINARY_DIR}/_nfrs_test_gen.cpp
    DEPENDS NFRS ${CMAKE_CURRENT_SOURCE_DIR}/nfrs_test_data/test_classes.hpp
    COMMENT "Running NFRS to generate reflection registration code..."
)
set(NEXUSFORCE_NFRS_GEN_SRC ${CMAKE_CURRENT_BINARY_DIR}/_nfrs_test_gen.cpp)

add_executable(NexusForceUnitTests
        main.cpp
        test_algorithm.cpp
        test_associative_container.cpp
        test_async.cpp
        test_complex_config.cpp
        test_compress.cpp
        test_db.cpp
        test_dns.cpp
        test_encrypt.cpp
        test_exception.cpp
        test_file.cpp
        test_functional.cpp
        test_http.cpp
        test_logging.cpp
        test_memory.cpp
        test_network.cpp
        test_network_ssl_tcp.cpp
        test_network_util.cpp
        test_nfrs.cpp
        test_numeric.cpp
        test_ranges.cpp
        test_reflect.cpp
        test_simd.cpp
        test_sequence_container.cpp
        test_serialize.cpp
        test_simple_config.cpp
        test_string.cpp
        test_string_utils.cpp
        test_system.cpp
        test_time.cpp
        test_util_container.cpp
        test_utility.cpp
        ${NEXUSFORCE_NFRS_GEN_SRC}
)

target_link_libraries(NexusForceUnitTests PRIVATE NexusForce::NexusForce)
if(MSVC)
    target_compile_options(NexusForceUnitTests PRIVATE /bigobj)
else()
    target_compile_options(NexusForceUnitTests PRIVATE -Wno-deprecated-declarations)
endif()
nexusforce_compiler_options(NexusForceUnitTests)
target_link_libraries(NexusForceUnitTests PRIVATE GTest::gtest)

enable_testing()
add_test(NAME NexusForceUnitTests COMMAND NexusForceUnitTests)
