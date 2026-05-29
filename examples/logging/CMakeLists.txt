# =============================================================================
# NexusForce Logging Module Examples
# =============================================================================

# ---- Logging Basic ----
add_executable(NexusForceLoggingBasicExample
    logging_basic.cpp
)
if(WIN32)
    set_target_properties(NexusForceLoggingBasicExample PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )
elseif(UNIX AND NOT APPLE)
    set_target_properties(NexusForceLoggingBasicExample PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )
endif()
target_link_libraries(NexusForceLoggingBasicExample PRIVATE NexusForce::NexusForce)
nexusforce_compiler_options(NexusForceLoggingBasicExample)

# ---- Logging Hierarchy ----
add_executable(NexusForceLoggingHierarchyExample
    logging_hierarchy.cpp
)
if(WIN32)
    set_target_properties(NexusForceLoggingHierarchyExample PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )
elseif(UNIX AND NOT APPLE)
    set_target_properties(NexusForceLoggingHierarchyExample PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )
endif()
target_link_libraries(NexusForceLoggingHierarchyExample PRIVATE NexusForce::NexusForce)
nexusforce_compiler_options(NexusForceLoggingHierarchyExample)

# ---- Logging Async ----
add_executable(NexusForceLoggingAsyncExample
    logging_async.cpp
)
if(WIN32)
    set_target_properties(NexusForceLoggingAsyncExample PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )
elseif(UNIX AND NOT APPLE)
    set_target_properties(NexusForceLoggingAsyncExample PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )
endif()
target_link_libraries(NexusForceLoggingAsyncExample PRIVATE NexusForce::NexusForce)
nexusforce_compiler_options(NexusForceLoggingAsyncExample)

# ---- Logging Context ----
add_executable(NexusForceLoggingContextExample
    logging_context.cpp
)
if(WIN32)
    set_target_properties(NexusForceLoggingContextExample PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )
elseif(UNIX AND NOT APPLE)
    set_target_properties(NexusForceLoggingContextExample PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )
endif()
target_link_libraries(NexusForceLoggingContextExample PRIVATE NexusForce::NexusForce)
nexusforce_compiler_options(NexusForceLoggingContextExample)
