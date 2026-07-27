add_custom_target(clean-cache
    COMMAND ${CMAKE_COMMAND}
        -D "BUILD_DIR=${CMAKE_BINARY_DIR}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/NexusForceCleanImpl.cmake"
    COMMENT "Cleaning build cache (preserving vcpkg_installed)..."
    VERBATIM
)
