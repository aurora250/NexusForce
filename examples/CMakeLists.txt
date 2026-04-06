if(NOT NEXUSFORCE_BUILD_EXAMPLES)
    message(STATUS "Skipping examples build (NEXUSFORCE_BUILD_EXAMPLES=OFF)")
    return()
endif()

file(GLOB EXAMPLE_SOURCES)

foreach(EXAMPLE_SINGLE ${EXAMPLE_SOURCES})
    get_filename_component(EXAMPLE_NAME ${EXAMPLE_SINGLE} NAME_WE)
    add_executable(${EXAMPLE_NAME} ${EXAMPLE_SINGLE})
    target_link_libraries(${EXAMPLE_NAME} PRIVATE NexusForce::NexusForce)

    configure_compiler_options(${EXAMPLE_NAME})

    message(STATUS "Example target created: ${EXAMPLE_NAME}")
endforeach()

install(TARGETS ${EXAMPLE_NAME}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}/examples
        OPTIONAL
)
