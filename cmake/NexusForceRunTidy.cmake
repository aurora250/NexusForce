set(SOURCE_LIST "${SOURCE_FILES}")

file(WRITE "${REPORT_FILE}" "clang-tidy report\n==================\n\n")

set(HAS_ERROR OFF)

foreach(SOURCE ${SOURCE_LIST})
    execute_process(
            COMMAND "${CLANG_TIDY_EXECUTABLE}"
            "--config-file=${CONFIG_FILE}"
            "--extra-arg=-w"
            "-p" "${BUILD_DIR}"
            "${SOURCE}"
            OUTPUT_VARIABLE TIDY_OUT
            ERROR_VARIABLE  TIDY_ERR
            RESULT_VARIABLE TIDY_RESULT
    )

    set(COMBINED "${TIDY_OUT}${TIDY_ERR}")

    string(REPLACE "\n" ";" COMBINED_LINES "${COMBINED}")

    set(ERROR_BLOCK "")
    set(IN_ERROR_BLOCK OFF)

    foreach(LINE ${COMBINED_LINES})
        if(LINE MATCHES "[^:]+:[0-9]+:[0-9]+: error:")
            set(IN_ERROR_BLOCK ON)
            string(APPEND ERROR_BLOCK "${LINE}
            ")
        elseif(IN_ERROR_BLOCK)
            if(LINE MATCHES "[^:]+:[0-9]+:[0-9]+: warning:|[^:]+:[0-9]+:[0-9]+: error:|[^:]+:[0-9]+:[0-9]+: note:")
                if(LINE MATCHES "[^:]+:[0-9]+:[0-9]+: error:")
                    string(APPEND ERROR_BLOCK "${LINE}\n")
                else()
                    set(IN_ERROR_BLOCK OFF)
                endif()
            else()
                string(APPEND ERROR_BLOCK "${LINE}\n")
            endif()
        endif()
    endforeach()

    if(ERROR_BLOCK)
        file(APPEND "${REPORT_FILE}" ">>> ${SOURCE}\n${ERROR_BLOCK}\n")
        message(STATUS ">>> ${SOURCE}\n${ERROR_BLOCK}")
    endif()

    if(NOT TIDY_RESULT EQUAL 0)
        set(HAS_ERROR ON)
    endif()
endforeach()

if(HAS_ERROR)
    message(FATAL_ERROR "clang-tidy found issues. See report: ${REPORT_FILE}")
else()
    file(APPEND "${REPORT_FILE}" "\nAll files passed clang-tidy checks.\n")
    message(STATUS "clang-tidy passed. Report: ${REPORT_FILE}")
endif()
