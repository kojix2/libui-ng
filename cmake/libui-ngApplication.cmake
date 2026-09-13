# Optional conveniences for executable targets that use libui.

set_property(GLOBAL PROPERTY _LIBUI_APPLICATION_SUPPORT_DIR
	"${CMAKE_CURRENT_LIST_DIR}/windows")

function(libui_configure_application target)
	if(NOT TARGET "${target}")
		message(FATAL_ERROR
			"libui_configure_application() requires an existing target: ${target}")
	endif()
	if(NOT TARGET libui::ui)
		message(FATAL_ERROR
			"libui_configure_application() requires the libui::ui target")
	endif()
	get_target_property(_libui_application_type "${target}" TYPE)
	if(NOT _libui_application_type STREQUAL "EXECUTABLE")
		message(FATAL_ERROR
			"libui_configure_application() requires an executable target: ${target}")
	endif()
	get_target_property(_libui_application_configured "${target}"
		LIBUI_APPLICATION_CONFIGURED)
	if(_libui_application_configured)
		return()
	endif()
	set_property(TARGET "${target}" PROPERTY LIBUI_APPLICATION_CONFIGURED TRUE)

	# libui APIs accept UTF-8.  This also makes UTF-8 source files independent
	# of the Windows system code page used by MSVC.
	if(MSVC)
		target_compile_options(${target} PRIVATE /utf-8)
	endif()

	if(NOT WIN32)
		return()
	endif()

	get_target_property(_libui_aliased_target libui::ui ALIASED_TARGET)
	if(_libui_aliased_target)
		set(_libui_library_target "${_libui_aliased_target}")
	else()
		set(_libui_library_target libui::ui)
	endif()
	get_target_property(_libui_library_type
		"${_libui_library_target}" TYPE)

	if(_libui_library_type STREQUAL "SHARED_LIBRARY")
		# Windows does not have an RPATH equivalent.  Put the runtime dependency
		# where the loader can find it when the application is run from its build
		# directory.
		get_target_property(_libui_runtime_output "${_libui_library_target}"
			RUNTIME_OUTPUT_DIRECTORY)
		get_target_property(_libui_application_output "${target}"
			RUNTIME_OUTPUT_DIRECTORY)
		if(NOT (_libui_runtime_output AND _libui_application_output AND
				_libui_runtime_output STREQUAL _libui_application_output))
			add_custom_command(TARGET ${target} POST_BUILD
				COMMAND ${CMAKE_COMMAND} -E copy_if_different
					$<TARGET_FILE:libui::ui> $<TARGET_FILE_DIR:${target}>
				VERBATIM
			)
		endif()
	elseif(_libui_library_type STREQUAL "STATIC_LIBRARY")
		# A static libui build has no DLL manifest, so the final executable must
		# request Common Controls v6 itself.
		enable_language(RC)
		get_property(_libui_application_support_dir GLOBAL PROPERTY
			_LIBUI_APPLICATION_SUPPORT_DIR)
		set(_libui_manifest_dir
			"${CMAKE_CURRENT_BINARY_DIR}/libui-manifests/${target}")
		file(MAKE_DIRECTORY "${_libui_manifest_dir}")
		string(REGEX REPLACE "[^A-Za-z0-9_.-]" "."
			LIBUI_APPLICATION_IDENTITY "${PROJECT_NAME}.${target}")
		set(LIBUI_APPLICATION_MANIFEST
			"${_libui_manifest_dir}/application.manifest")
		configure_file(
			"${_libui_application_support_dir}/application.manifest.in"
			"${LIBUI_APPLICATION_MANIFEST}"
			@ONLY
		)
		configure_file(
			"${_libui_application_support_dir}/application.rc.in"
			"${_libui_manifest_dir}/application.rc"
			@ONLY
		)
		target_sources(${target} PRIVATE
			"${_libui_manifest_dir}/application.rc")
		if(MSVC AND COMMAND target_link_options)
			# application.rc supplies manifest resource ID 1.  Disable the MSVC
			# linker's second manifest with the same resource ID.
			target_link_options(${target} PRIVATE /MANIFEST:NO)
		endif()
	endif()
endfunction()
