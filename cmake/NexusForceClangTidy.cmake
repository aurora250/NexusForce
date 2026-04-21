if(NOT NEXUSFORCE_CLANG_TIDY)
    return()
endif()

find_program(CLANG_TIDY_EXECUTABLE
        NAMES clang-tidy clang-tidy-19 clang-tidy-18 clang-tidy-17
        DOC "clang-tidy executable"
)

if(NOT CLANG_TIDY_EXECUTABLE)
    message(WARNING "clang-tidy not found. Static analysis disabled.")
    set(NEXUSFORCE_CLANG_TIDY OFF CACHE BOOL "clang-tidy disabled" FORCE)
    return()
endif()

message(STATUS "Found clang-tidy: ${CLANG_TIDY_EXECUTABLE}")

file(GLOB_RECURSE TIDY_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

list(FILTER TIDY_SOURCES EXCLUDE REGEX ".*/(build|vcpkg_installed)/.*")
list(LENGTH TIDY_SOURCES NUM_TIDY_SOURCES)
message(STATUS "Found ${NUM_TIDY_SOURCES} source files for clang-tidy")

set(TIDY_REPORT_FILE "${CMAKE_BINARY_DIR}/clang_tidy_report.txt")

set(TIDY_COMMANDS "")
foreach(SOURCE_FILE ${TIDY_SOURCES})
    list(APPEND TIDY_COMMANDS
            COMMAND ${CLANG_TIDY_EXECUTABLE}
            "--config-file=${CMAKE_CURRENT_SOURCE_DIR}/.clang-tidy"
            "--extra-arg=-w"
            "-p" "${CMAKE_BINARY_DIR}"
            "${SOURCE_FILE}"
    )
endforeach()

add_custom_target(tidy-check
        ${TIDY_COMMANDS}
        COMMENT "Running clang-tidy on ${NUM_TIDY_SOURCES} files..."
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        VERBATIM
)

add_custom_target(tidy-check-report
        COMMAND ${CMAKE_COMMAND}
        -DCLANG_TIDY_EXECUTABLE=${CLANG_TIDY_EXECUTABLE}
        -DCONFIG_FILE=${CMAKE_CURRENT_SOURCE_DIR}/.clang-tidy
        -DBUILD_DIR=${CMAKE_BINARY_DIR}
        -DREPORT_FILE=${TIDY_REPORT_FILE}
        "-DSOURCE_FILES=${TIDY_SOURCES}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/NexusForceRunTidy.cmake"
        COMMENT "Running clang-tidy and saving report to ${TIDY_REPORT_FILE}..."
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        VERBATIM
)

set_directory_properties(PROPERTIES
        ADDITIONAL_CLEAN_FILES "${TIDY_REPORT_FILE}"
)
