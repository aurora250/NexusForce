add_executable(NexusForceCryptoExample
        crypto.cpp
)

if(WIN32)
    set_target_properties(NexusForceCryptoExample PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin/Debug
            RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin/Release
            RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/bin/RelWithDebInfo
            RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_BINARY_DIR}/bin/MinSizeRel
    )
elseif(UNIX AND NOT APPLE)
    set_target_properties(NexusForceCryptoExample PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    )
endif()

target_link_libraries(NexusForceCryptoExample PRIVATE NexusForce::NexusForce)
nexusforce_compiler_options(NexusForceCryptoExample)
