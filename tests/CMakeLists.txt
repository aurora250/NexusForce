if(NOT MSTL_BUILD_TESTS)
    return()
endif()

add_executable(MSTL_TEST main.cpp try.cpp try.h)

set_target_properties(MSTL_TEST PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin/Debug
        RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin/Release
        RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/bin/RelWithDebInfo
        RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_BINARY_DIR}/bin/MinSizeRel
)

target_link_libraries(MSTL_TEST PRIVATE MSTL)
configure_compiler_options(MSTL_TEST)
