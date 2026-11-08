find_package(OpenSSL QUIET)

if(OpenSSL_FOUND)
    add_nexusforce_benchmark(NexusForceAes256Benchmark aes256_benchmark.cpp)
    target_link_libraries(NexusForceAes256Benchmark PRIVATE OpenSSL::Crypto)
else()
    message(STATUS "OpenSSL not found — skipping AES-256 benchmark.")
endif()
