if(NOT DEFINED LIBUI_SOURCE_DIR OR NOT DEFINED LIBUI_TEMPLATE OR NOT DEFINED LIBUI_OUTPUT)
	message(FATAL_ERROR "GenerateVersion.cmake requires source, template, and output paths")
endif()

set(VCS_TAG unknown)
find_package(Git QUIET)
if(GIT_FOUND AND EXISTS "${LIBUI_SOURCE_DIR}/.git")
	execute_process(
		COMMAND "${GIT_EXECUTABLE}" describe --tags --always --abbrev=8 --match=commit-*
		WORKING_DIRECTORY "${LIBUI_SOURCE_DIR}"
		RESULT_VARIABLE git_result
		OUTPUT_VARIABLE git_version
		OUTPUT_STRIP_TRAILING_WHITESPACE
		ERROR_QUIET
	)
	if(git_result EQUAL 0 AND NOT git_version STREQUAL "")
		set(VCS_TAG "${git_version}")
	endif()
endif()

get_filename_component(output_dir "${LIBUI_OUTPUT}" DIRECTORY)
file(MAKE_DIRECTORY "${output_dir}")
set(tmp "${LIBUI_OUTPUT}.tmp")
configure_file("${LIBUI_TEMPLATE}" "${tmp}" @ONLY)
execute_process(COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${tmp}" "${LIBUI_OUTPUT}")
file(REMOVE "${tmp}")
