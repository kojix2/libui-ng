foreach(required LIBUI_ARTIFACT LIBUI_STAGE_DIR LIBUI_SOURCE_DIR)
	if(NOT DEFINED ${required})
		message(FATAL_ERROR "StageLegacyRelease.cmake requires ${required}")
	endif()
endforeach()

set(output_dir "${LIBUI_STAGE_DIR}/builddir/meson-out")
file(MAKE_DIRECTORY "${output_dir}")
set(log_dir "${LIBUI_STAGE_DIR}/builddir/meson-logs")
file(MAKE_DIRECTORY "${log_dir}")
# Keep the legacy archive entries stable. Detailed build and test output remains
# available in the CI job log; these files identify the producing build system.
file(WRITE "${log_dir}/meson-setup.txt"
	"Legacy release layout staged by CMake (${LIBUI_CONFIG}).\n")
file(WRITE "${log_dir}/meson-log.txt"
	"See the producing CI job for the complete CMake build log.\n")
if(LIBUI_EXPECT_TESTS)
	file(WRITE "${log_dir}/testlog.txt"
		"See the producing CI job for the complete CTest log.\n")
endif()

function(libui_copy_if_different source destination)
	execute_process(
		COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${source}" "${destination}"
		RESULT_VARIABLE copy_result
	)
	if(NOT copy_result EQUAL 0)
		message(FATAL_ERROR "Failed to copy ${source} to ${destination}")
	endif()
endfunction()

if(LIBUI_STATIC)
	set(output_library "${output_dir}/libui.a")
else()
	get_filename_component(artifact_name "${LIBUI_ARTIFACT}" NAME)
	set(output_library "${output_dir}/${artifact_name}")
endif()
libui_copy_if_different("${LIBUI_ARTIFACT}" "${output_library}")
if(NOT LIBUI_STATIC AND DEFINED LIBUI_LINKER_ARTIFACT AND EXISTS "${LIBUI_LINKER_ARTIFACT}")
	get_filename_component(linker_artifact_name "${LIBUI_LINKER_ARTIFACT}" NAME)
	if(NOT linker_artifact_name STREQUAL artifact_name)
		libui_copy_if_different("${LIBUI_LINKER_ARTIFACT}" "${output_dir}/${linker_artifact_name}")
	endif()
endif()
libui_copy_if_different("${LIBUI_SOURCE_DIR}/ui.h" "${LIBUI_STAGE_DIR}/ui.h")
if(NOT DEFINED LIBUI_PLATFORM_HEADER)
	message(FATAL_ERROR "The platform header is required by the legacy release contract")
endif()
libui_copy_if_different(
	"${LIBUI_SOURCE_DIR}/${LIBUI_PLATFORM_HEADER}"
	"${LIBUI_STAGE_DIR}/${LIBUI_PLATFORM_HEADER}"
)

if(DEFINED LIBUI_RUNTIME_DIR AND IS_DIRECTORY "${LIBUI_RUNTIME_DIR}")
	file(GLOB runtime_artifacts "${LIBUI_RUNTIME_DIR}/*")
	foreach(runtime_artifact IN LISTS runtime_artifacts)
		if(NOT IS_DIRECTORY "${runtime_artifact}")
			file(COPY "${runtime_artifact}" DESTINATION "${output_dir}")
		endif()
	endforeach()
endif()

if(LIBUI_MSVC AND LIBUI_STATIC AND LIBUI_CONFIG STREQUAL "Debug")
	if(NOT DEFINED LIBUI_COMPILE_PDB OR NOT EXISTS "${LIBUI_COMPILE_PDB}")
		message(FATAL_ERROR "The MSVC Debug compile PDB is required by the legacy release contract")
	endif()
	set(pdb_dir "${output_dir}/libui.a.p")
	file(MAKE_DIRECTORY "${pdb_dir}")
	file(COPY "${LIBUI_COMPILE_PDB}" DESTINATION "${pdb_dir}")
endif()

set(required_paths
	"${output_library}"
	"${LIBUI_STAGE_DIR}/ui.h"
	"${LIBUI_STAGE_DIR}/${LIBUI_PLATFORM_HEADER}"
	"${log_dir}/meson-setup.txt"
	"${log_dir}/meson-log.txt"
)
if(LIBUI_EXPECT_TESTS)
	list(APPEND required_runtime tester qa unit utf-unit imagerep-unit)
	list(APPEND required_paths "${log_dir}/testlog.txt")
endif()
if(LIBUI_EXPECT_EXAMPLES)
	list(APPEND required_runtime
		hello-world window button label checkbox entry multiline-entry box-layout
		form-layout grid-layout tabs group control-destroy combobox editable-combobox
		radio-buttons spinbox slider progressbar separator date-picker time-picker
		color-button font-button menu menu-checkbox open-file open-folder save-file
		message-box error-message-box timer queue-main should-quit controlgallery
		histogram drawtext datetime cpp-multithread
	)
endif()
foreach(runtime_name IN LISTS required_runtime)
	if(WIN32)
		string(APPEND runtime_name ".exe")
	endif()
	list(APPEND required_paths "${output_dir}/${runtime_name}")
endforeach()
foreach(required_path IN LISTS required_paths)
	if(NOT EXISTS "${required_path}")
		message(FATAL_ERROR "Legacy release entry was not staged: ${required_path}")
	endif()
endforeach()
