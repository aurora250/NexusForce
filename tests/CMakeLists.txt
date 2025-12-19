if(NOT MSTL_BUILD_TESTS)
    return()
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_executable(MSTL_TEST main.cpp try.cpp try.h)

target_link_libraries(MSTL_TEST PRIVATE MSTL)

configure_compiler_options(MSTL_TEST)
