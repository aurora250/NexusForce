if(NOT NEXUSFORCE_BUILD_DOCS)
    message(STATUS "Skipping documentation build (NEXUSFORCE_BUILD_DOCS=OFF)")
    return()
endif()

find_package(Doxygen QUIET)

if(NEXUSFORCE_BUILD_DOCS AND NOT DOXYGEN_FOUND)
     message(FATAL_ERROR "Doxygen not found. Please install Doxygen to build documentation.")
endif()

find_program(DOT_EXECUTABLE dot
        HINTS
        "${CONFIG_DOT_EXECUTABLE}/bin"
        "C:/Program Files/Graphviz/bin"
        "C:/Program Files (x86)/Graphviz/bin"
        "/usr/bin"
        "/usr/local/bin"
        DOC "Graphviz dot executable"
)

if(DOT_EXECUTABLE)
    set(HAVE_DOT "YES")
    message(STATUS "Found dot: ${DOT_EXECUTABLE}")
else()
    set(HAVE_DOT "NO")
    message(STATUS "dot not found - diagrams will be disabled")
endif()


set(DOXYGEN_OUTPUT_DIR "${CMAKE_BINARY_DIR}/doxygen_output")
set(DOXYGEN_INPUT_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/../include")

configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in
        ${CMAKE_BINARY_DIR}/Doxyfile
        @ONLY
)

add_custom_target(doc
        COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_BINARY_DIR}/Doxyfile
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Generating API documentation with Doxygen"
        VERBATIM
)

add_custom_target(doc-clean
        COMMAND ${CMAKE_COMMAND} -E remove_directory ${DOXYGEN_OUTPUT_DIR}
        COMMENT "Cleaning documentation output"
)

message(STATUS "Doc Configuration:")
message(STATUS "Output directory:  ${DOXYGEN_OUTPUT_DIR}")
message(STATUS "Input directories: ${DOXYGEN_INPUT_DIRS}")
message(STATUS "")
message(STATUS "To generate documentation:")
message(STATUS "  make doc                 # Generate documentation")
message(STATUS "  make doc-clean           # Clean documentation output")
message(STATUS "")
message(STATUS "Documentation will be available at: ${DOXYGEN_OUTPUT_DIR}/html/index.html")
