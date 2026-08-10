if(NOT NEXUSFORCE_CLANG_TIDY)
    return()
endif()

if (WIN32)
    find_program(CLANG_TIDY_EXECUTABLE
            NAMES clang-tidy
            DOC "clang-tidy executable"
    )
elseif(UNIX AND NOT APPLE)
    find_program(CLANG_TIDY_EXECUTABLE
            NAMES clang-tidy-19
            DOC "clang-tidy executable"
    )
endif()

if(NOT CLANG_TIDY_EXECUTABLE)
    message(WARNING "clang-tidy not found. Static analysis disabled.")
    set(NEXUSFORCE_CLANG_TIDY OFF CACHE BOOL "clang-tidy disabled" FORCE)
    return()
endif()

message(STATUS "Found clang-tidy: ${CLANG_TIDY_EXECUTABLE}")

file(GLOB_RECURSE TIDY_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/*.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/*.cpp"
)

list(FILTER TIDY_SOURCES EXCLUDE REGEX ".*/(build|vcpkg_installed)/.*")
list(LENGTH TIDY_SOURCES NUM_TIDY_SOURCES)
message(STATUS "Found ${NUM_TIDY_SOURCES} source files for clang-tidy")

set(TIDY_STAMP_DIR "${CMAKE_BINARY_DIR}/tidy_stamps")
file(MAKE_DIRECTORY "${TIDY_STAMP_DIR}")

set(TIDY_STAMPS "")
foreach(SOURCE_FILE ${TIDY_SOURCES})
    string(REGEX REPLACE "[^a-zA-Z0-9_]" "_" STAMP_NAME "${SOURCE_FILE}")
    set(STAMP_FILE "${TIDY_STAMP_DIR}/${STAMP_NAME}.stamp")
    list(APPEND TIDY_STAMPS "${STAMP_FILE}")

    add_custom_command(
            OUTPUT "${STAMP_FILE}"
            COMMAND ${CLANG_TIDY_EXECUTABLE}
            "--config-file=${CMAKE_CURRENT_SOURCE_DIR}/.clang-tidy"
            "--extra-arg=-w"
            "-p" "${CMAKE_BINARY_DIR}"
            "${SOURCE_FILE}"
            COMMAND ${CMAKE_COMMAND} -E touch "${STAMP_FILE}"
            COMMENT "clang-tidy: ${SOURCE_FILE}"
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            VERBATIM
    )
endforeach()

add_custom_target(tidy
        DEPENDS ${TIDY_STAMPS}
        COMMENT "Running clang-tidy on ${NUM_TIDY_SOURCES} files..."
)

set_target_properties(tidy PROPERTIES FOLDER "utils")
