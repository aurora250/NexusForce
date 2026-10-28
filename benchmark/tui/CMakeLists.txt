# ---- Text layout (width, wrap) ----
add_nexusforce_benchmark(NexusForceTextLayoutBenchmark
    text_layout_benchmark.cpp
)

# ---- Markdown (parse, layout, stream) ----
add_nexusforce_benchmark(NexusForceMarkdownBenchmark
    markdown_benchmark.cpp
)

# ---- Frame diff (render + ANSI) ----
add_nexusforce_benchmark(NexusForceFrameBenchmark
    frame_benchmark.cpp
)

# ---- Components (scroll view, virtual list, headless pipeline) ----
add_nexusforce_benchmark(NexusForceComponentBenchmark
    component_benchmark.cpp
)

# ---- Panel/table render (layout + draw) ----
add_nexusforce_benchmark(NexusForceTuiRenderBenchmark
    render_benchmark.cpp
)
