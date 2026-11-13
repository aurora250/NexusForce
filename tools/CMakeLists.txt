include(${CMAKE_CURRENT_SOURCE_DIR}/../cmake/NexusForceCompilerOptions.cmake)

add_executable(NexusRsvp reflect_scanner.cpp)
add_executable(NexusForce::NexusRsvp ALIAS NexusRsvp)
target_link_libraries(NexusRsvp PRIVATE NexusForce::NexusForce)
nexusforce_compiler_options(NexusRsvp)

set_target_properties(NexusRsvp PROPERTIES
    INSTALL_RPATH "$ORIGIN/../${CMAKE_INSTALL_LIBDIR}"
)
set_target_properties(NexusRsvp PROPERTIES FOLDER "tools")

install(TARGETS NexusRsvp
        EXPORT NexusForceTargets
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        CONFIGURATIONS Release
)
