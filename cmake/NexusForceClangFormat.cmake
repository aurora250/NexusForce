if(NOT NEXUSFORCE_FORMAT)
    return()
endif()

find_program(CLANG_FORMAT_EXECUTABLE
        NAMES clang-format clang-format-19 clang-format-18 clang-format-17 clang-format-16
        DOC "clang-format executable"
)

if(NOT CLANG_FORMAT_EXECUTABLE)
    message(WARNING "clang-format not found. Code formatting targets will not be available.")
    return()
endif()

message(STATUS "Found clang-format: ${CLANG_FORMAT_EXECUTABLE}")

file(GLOB_RECURSE NEXUSFORCE_FORMAT_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/benchmark/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/examples/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/examples/*.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/*.hpp"
)

list(FILTER NEXUSFORCE_FORMAT_SOURCES EXCLUDE REGEX ".*/build/.*")
list(FILTER NEXUSFORCE_FORMAT_SOURCES EXCLUDE REGEX ".*/vcpkg_installed/.*")

list(LENGTH NEXUSFORCE_FORMAT_SOURCES NUM_SOURCES)
message(STATUS "Found ${NUM_SOURCES} source files for formatting")

if(NUM_SOURCES EQUAL 0)
    message(STATUS "No source files found for formatting. Skipping format targets.")
    return()
endif()

set(FORMAT_COMMANDS "")
foreach(SOURCE_FILE ${NEXUSFORCE_FORMAT_SOURCES})
    list(APPEND FORMAT_COMMANDS
            COMMAND ${CLANG_FORMAT_EXECUTABLE} -style=file -i "${SOURCE_FILE}"
    )
endforeach()

add_custom_target(format
        ${FORMAT_COMMANDS}
        COMMENT "Formatting ${NUM_SOURCES} files with ${CLANG_FORMAT_EXECUTABLE}..."
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        VERBATIM
)

set(FORMAT_CHECK_COMMANDS "")
foreach(SOURCE_FILE ${NEXUSFORCE_FORMAT_SOURCES})
    list(APPEND FORMAT_CHECK_COMMANDS
            COMMAND ${CLANG_FORMAT_EXECUTABLE} -style=file --dry-run -Werror "${SOURCE_FILE}"
    )
endforeach()

add_custom_target(format-check
        COMMAND ${CMAKE_COMMAND} -E echo "Checking code format with ${CLANG_FORMAT_EXECUTABLE}..."
        ${FORMAT_CHECK_COMMANDS}
        COMMAND ${CMAKE_COMMAND} -E echo "✅ All ${NUM_SOURCES} files are properly formatted!"
        COMMENT "Checking code format with clang-format"
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        VERBATIM
)
