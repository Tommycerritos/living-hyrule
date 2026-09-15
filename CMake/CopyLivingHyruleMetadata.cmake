# Keep local metadata available without byte-comparing 20,000 unchanged files
# on every Windows code build. No mirror/delete flags: extra local files stay.
if(NOT IS_DIRECTORY "${METADATA_SOURCE}" OR NOT DEFINED METADATA_DESTINATION)
    message(FATAL_ERROR "Metadata copy needs an existing source and a destination")
endif()

if(CMAKE_HOST_WIN32)
    find_program(METADATA_ROBOCOPY robocopy.exe PATHS "$ENV{SystemRoot}/System32" NO_DEFAULT_PATH REQUIRED)
    execute_process(
        COMMAND "${METADATA_ROBOCOPY}" "${METADATA_SOURCE}" "${METADATA_DESTINATION}"
            /E /COPY:DAT /DCOPY:DAT /R:1 /W:1 /NFL /NDL /NJH /NJS /NP /MT:8
        RESULT_VARIABLE copy_result OUTPUT_VARIABLE copy_output ERROR_VARIABLE copy_error
    )
    # Robocopy's 0..7 are successful comparison/copy results; 8+ are errors.
    if(NOT copy_result MATCHES "^[0-7]$")
        message(FATAL_ERROR "Metadata copy failed (${copy_result}): ${copy_output}${copy_error}")
    endif()
else()
    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E copy_directory_if_different "${METADATA_SOURCE}" "${METADATA_DESTINATION}"
        RESULT_VARIABLE copy_result
    )
    if(NOT copy_result EQUAL 0)
        message(FATAL_ERROR "Metadata copy failed (${copy_result})")
    endif()
endif()
