# ---- Basic String ----
add_nexusforce_benchmark(NexusForceBasicStringBenchmark
    basic_string_benchmark.cpp
)

# ---- String Builder ----
add_nexusforce_benchmark(NexusForceStringBuilderBenchmark
    string_builder_benchmark.cpp
)

# ---- Format ----
add_subdirectory(format)

# ---- Regex ----
add_subdirectory(regex)
