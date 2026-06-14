include(${CMAKE_CURRENT_SOURCE_DIR}/../cmake/NexusForceCompilerOptions.cmake)

# NexusForceReflectScanner
add_executable(NFRS reflect_scanner.cpp)
target_link_libraries(NFRS PRIVATE NexusForce::NexusForce)
nexusforce_compiler_options(NFRS)

install(TARGETS NFRS
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
