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
        test_format.cpp
        test_functional.cpp
        test_http.cpp
        test_iterator.cpp
        test_logging.cpp
        test_memory.cpp
        test_memory_pool.cpp
        test_network.cpp
        test_network_ssl_tcp.cpp
        test_network_util.cpp
        test_nfrs.cpp
        test_numeric.cpp
        test_plugin.cpp
        test_random.cpp
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
        test_tui.cpp
        test_tui_component.cpp
        test_tui_dom.cpp
        test_utf.cpp
        test_util_container.cpp
        test_utility.cpp
)

target_link_libraries(NexusForceUnitTests PRIVATE NexusForce::NexusForce)
if(MSVC)
    target_compile_options(NexusForceUnitTests PRIVATE /bigobj)
else()
    target_compile_options(NexusForceUnitTests PRIVATE -Wno-deprecated-declarations)
endif()
nexusforce_compiler_options(NexusForceUnitTests)
target_link_libraries(NexusForceUnitTests PRIVATE GTest::gtest)

# Real loadable plugin used to exercise plugin_manager end to end: the module
# exports the create_plugin / destroy_plugin entry points the manager resolves
# via dynamic_library. Its path is handed to the test as a compile definition.
add_library(NexusForceTestPlugin MODULE plugin_test_module.cpp)
target_link_libraries(NexusForceTestPlugin PRIVATE NexusForce::NexusForce)
nexusforce_compiler_options(NexusForceTestPlugin)
set_target_properties(NexusForceTestPlugin PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/$<CONFIG>/plugins"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>/plugins"
        PREFIX ""
        FOLDER "tests"
)
add_dependencies(NexusForceUnitTests NexusForceTestPlugin)
target_compile_definitions(NexusForceUnitTests PRIVATE NEFORCE_TEST_PLUGIN_PATH="$<TARGET_FILE:NexusForceTestPlugin>")

enable_testing()
add_test(NAME NexusForceUnitTests COMMAND NexusForceUnitTests)

nexusforce_reflect_scan(
    TARGET NexusForceUnitTests
    HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/nfrs_test_data
)
set_target_properties(NexusForceUnitTests PROPERTIES FOLDER "tests")
