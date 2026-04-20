if(NOT NEXUSFORCE_BUILD_EXAMPLES)
    message(STATUS "Skipping examples build (NEXUSFORCE_BUILD_EXAMPLES=OFF)")
    return()
endif()

add_executable(NexusForceExample1_Server
        example1_server.cpp
)

set_target_properties(NexusForceExample1_Server PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin/Debug
        RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin/Release
        RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/bin/RelWithDebInfo
        RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_BINARY_DIR}/bin/MinSizeRel
)

target_link_libraries(NexusForceExample1_Server PRIVATE NexusForce::NexusForce)
nexusforce_compiler_options(NexusForceExample1_Server)


add_executable(NexusForceExample1_Client
        example1_client.cpp
)

set_target_properties(NexusForceExample1_Client PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin/Debug
        RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin/Release
        RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/bin/RelWithDebInfo
        RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_BINARY_DIR}/bin/MinSizeRel
)

target_link_libraries(NexusForceExample1_Client PRIVATE NexusForce::NexusForce)
nexusforce_compiler_options(NexusForceExample1_Client)
