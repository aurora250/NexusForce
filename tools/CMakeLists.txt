include(${CMAKE_CURRENT_SOURCE_DIR}/../cmake/NexusForceCompilerOptions.cmake)

add_executable(NFRS reflect_scanner.cpp)
add_executable(NexusForce::NFRS ALIAS NFRS)
target_link_libraries(NFRS PRIVATE NexusForce::NexusForce)
nexusforce_compiler_options(NFRS)

set_target_properties(NFRS PROPERTIES
    INSTALL_RPATH "$ORIGIN/../${CMAKE_INSTALL_LIBDIR}/Release"
)
set_target_properties(NFRS PROPERTIES FOLDER "tools")

install(TARGETS NFRS
        EXPORT NexusForceTargets
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        CONFIGURATIONS Release
)
