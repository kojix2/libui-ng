cmake_minimum_required(VERSION 3.15)

if(POLICY CMP0091)
	cmake_policy(SET CMP0091 NEW)
endif()

project(libui LANGUAGES C)

if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
	set(LIBUI_TOP_LEVEL ON)
else()
	set(LIBUI_TOP_LEVEL OFF)
endif()

if(LIBUI_TOP_LEVEL)
	option(BUILD_SHARED_LIBS "Build shared libraries" ON)
elseif(NOT DEFINED BUILD_SHARED_LIBS)
	set(BUILD_SHARED_LIBS ON)
endif()

option(LIBUI_BUILD_TESTS "Build libui-ng tests" ${LIBUI_TOP_LEVEL})
option(LIBUI_BUILD_EXAMPLES "Build libui-ng examples" ${LIBUI_TOP_LEVEL})
option(LIBUI_FETCH_TEST_DEPS "Fetch missing test dependencies" ${LIBUI_TOP_LEVEL})

if(WIN32 OR LIBUI_BUILD_EXAMPLES)
	enable_language(CXX)
endif()
if(WIN32)
	enable_language(RC)
endif()
if(APPLE)
	# Objective-C became a first-class CMake language in 3.16.  Keep the
	# project-wide minimum at 3.15 for non-Apple consumers.
	if(CMAKE_VERSION VERSION_LESS 3.16)
		message(FATAL_ERROR "macOS builds require CMake 3.16 or newer")
	endif()
	enable_language(OBJC)
endif()

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

function(libui_apply_internal_options target)
	set_target_properties(${target} PROPERTIES
		RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/artifacts/$<CONFIG>"
		C_STANDARD 99
		C_STANDARD_REQUIRED ON
		C_EXTENSIONS OFF
		CXX_STANDARD 14
		CXX_STANDARD_REQUIRED ON
		CXX_EXTENSIONS OFF
	)
	target_compile_options(${target} PRIVATE
		$<$<COMPILE_LANG_AND_ID:C,GNU,Clang,AppleClang>:-Wall>
		$<$<COMPILE_LANG_AND_ID:C,GNU,Clang,AppleClang>:-Wextra>
		$<$<COMPILE_LANG_AND_ID:C,GNU,Clang,AppleClang>:-Wpedantic>
		$<$<COMPILE_LANG_AND_ID:C,GNU,Clang,AppleClang>:-Wno-unused-parameter>
		$<$<COMPILE_LANG_AND_ID:C,GNU,Clang,AppleClang>:-Wno-switch>
		$<$<COMPILE_LANG_AND_ID:CXX,GNU,Clang,AppleClang>:-Wall>
		$<$<COMPILE_LANG_AND_ID:CXX,GNU,Clang,AppleClang>:-Wextra>
		$<$<COMPILE_LANG_AND_ID:CXX,GNU,Clang,AppleClang>:-Wpedantic>
		$<$<COMPILE_LANG_AND_ID:CXX,GNU,Clang,AppleClang>:-Wno-unused-parameter>
		$<$<COMPILE_LANG_AND_ID:CXX,GNU,Clang,AppleClang>:-Wno-switch>
		$<$<COMPILE_LANG_AND_ID:OBJC,GNU,Clang,AppleClang>:-Wall>
		$<$<COMPILE_LANG_AND_ID:OBJC,GNU,Clang,AppleClang>:-Wextra>
		$<$<COMPILE_LANG_AND_ID:OBJC,GNU,Clang,AppleClang>:-Wpedantic>
		$<$<COMPILE_LANG_AND_ID:OBJC,GNU,Clang,AppleClang>:-Wno-unused-parameter>
		$<$<COMPILE_LANG_AND_ID:OBJC,GNU,Clang,AppleClang>:-Wno-switch>
		$<$<CXX_COMPILER_ID:MSVC>:/W4>
		$<$<CXX_COMPILER_ID:MSVC>:/wd4100>
		$<$<CXX_COMPILER_ID:MSVC>:/bigobj>
		$<$<AND:$<CXX_COMPILER_ID:MSVC>,$<CONFIG:Debug>>:/RTC1>
	)
	if(COMMAND target_link_options)
		target_link_options(${target} PRIVATE
			$<$<CXX_COMPILER_ID:MSVC>:/LARGEADDRESSAWARE>
			$<$<CXX_COMPILER_ID:MSVC>:/INCREMENTAL:NO>
			$<$<CXX_COMPILER_ID:MSVC>:/MANIFEST:NO>
		)
		if(MINGW)
			target_link_options(${target} PRIVATE -static -static-libgcc -static-libstdc++)
		endif()
	endif()
	if(BUILD_SHARED_LIBS AND NOT "${target}" STREQUAL "libui_ui")
		if(APPLE)
			set(_libui_executable_rpath "@executable_path/")
		elseif(UNIX)
			set(_libui_executable_rpath "$ORIGIN")
		endif()
		if(DEFINED _libui_executable_rpath)
			# Runtime artifacts are staged next to libui.  Use that same
			# relocatable layout in the build tree so copied release artifacts
			# never retain an absolute path to the CI build directory.
			set_target_properties(${target} PROPERTIES
				BUILD_WITH_INSTALL_RPATH TRUE
				INSTALL_RPATH "${_libui_executable_rpath}"
			)
			unset(_libui_executable_rpath)
		endif()
	endif()
endfunction()

add_library(libui_ui)
add_library(libui::ui ALIAS libui_ui)

set_target_properties(libui_ui PROPERTIES
	OUTPUT_NAME ui
	PREFIX lib
	EXPORT_NAME ui
	POSITION_INDEPENDENT_CODE ON
	C_STANDARD 99
	C_STANDARD_REQUIRED ON
	C_EXTENSIONS OFF
	CXX_STANDARD 14
	CXX_STANDARD_REQUIRED ON
	CXX_EXTENSIONS OFF
	C_VISIBILITY_PRESET hidden
	CXX_VISIBILITY_PRESET hidden
	OBJC_VISIBILITY_PRESET hidden
	COMPILE_PDB_NAME libui
	COMPILE_PDB_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/pdb/$<CONFIG>"
)

target_include_directories(libui_ui PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
	$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_compile_definitions(libui_ui PRIVATE libui_EXPORTS)
if(NOT BUILD_SHARED_LIBS)
	target_compile_definitions(libui_ui PUBLIC _UI_STATIC)
endif()
libui_apply_internal_options(libui_ui)
if(BUILD_SHARED_LIBS AND NOT WIN32)
	# Executables use a relative RPATH and share one runtime output directory.
	# Copy the library once, before any dependent executable is linked.
	add_custom_command(TARGET libui_ui POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E make_directory
			"${CMAKE_BINARY_DIR}/artifacts/$<CONFIG>"
		COMMAND ${CMAKE_COMMAND} -E copy_if_different
			$<TARGET_FILE:libui_ui> "${CMAKE_BINARY_DIR}/artifacts/$<CONFIG>"
		VERBATIM
	)
endif()

set(LIBUI_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(LIBUI_VERSION_SOURCE "${LIBUI_GENERATED_DIR}/version.c")
add_custom_target(libui_generate_version ALL
	COMMAND ${CMAKE_COMMAND}
		-DLIBUI_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}
		-DLIBUI_TEMPLATE=${CMAKE_CURRENT_SOURCE_DIR}/common/version.c.in
		-DLIBUI_OUTPUT=${LIBUI_VERSION_SOURCE}
		-P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/GenerateVersion.cmake
	BYPRODUCTS ${LIBUI_VERSION_SOURCE}
	VERBATIM
)
set_source_files_properties(${LIBUI_VERSION_SOURCE} PROPERTIES GENERATED TRUE)
target_sources(libui_ui PRIVATE ${LIBUI_VERSION_SOURCE})
add_dependencies(libui_ui libui_generate_version)

add_subdirectory(common)
if(WIN32)
	add_subdirectory(windows)
elseif(APPLE)
	add_subdirectory(darwin)
elseif(UNIX)
	add_subdirectory(unix)
else()
	message(FATAL_ERROR "Unsupported platform")
endif()

if(LIBUI_BUILD_TESTS)
	enable_testing()
	add_subdirectory(test)
endif()
if(LIBUI_BUILD_EXAMPLES)
	add_subdirectory(examples)
endif()

install(TARGETS libui_ui
	EXPORT libui-ngTargets
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
	INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
install(FILES ui.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
if(WIN32)
	install(FILES ui_windows.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
elseif(APPLE)
	install(FILES ui_darwin.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
else()
	install(FILES ui_unix.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
endif()

set(LIBUI_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/libui-ng")
configure_package_config_file(
	cmake/libui-ngConfig.cmake.in
	"${CMAKE_CURRENT_BINARY_DIR}/libui-ngConfig.cmake"
	INSTALL_DESTINATION "${LIBUI_INSTALL_CMAKEDIR}"
)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/libui-ngConfig.cmake"
	DESTINATION "${LIBUI_INSTALL_CMAKEDIR}"
)
install(EXPORT libui-ngTargets
	FILE libui-ngTargets.cmake
	NAMESPACE libui::
	DESTINATION "${LIBUI_INSTALL_CMAKEDIR}"
)

set(LIBUI_LEGACY_STAGE_DIR "${CMAKE_CURRENT_BINARY_DIR}/legacy-stage" CACHE PATH
	"Directory used by the stage-legacy target")
if(WIN32)
	set(LIBUI_PLATFORM_HEADER ui_windows.h)
elseif(APPLE)
	set(LIBUI_PLATFORM_HEADER ui_darwin.h)
else()
	set(LIBUI_PLATFORM_HEADER ui_unix.h)
endif()
set(LIBUI_LEGACY_RUNTIME_TARGETS)
if(LIBUI_BUILD_TESTS)
	list(APPEND LIBUI_LEGACY_RUNTIME_TARGETS tester qa unit utf-unit imagerep-unit)
endif()
if(LIBUI_BUILD_EXAMPLES)
	list(APPEND LIBUI_LEGACY_RUNTIME_TARGETS
		hello-world window button label checkbox entry multiline-entry box-layout
		form-layout grid-layout tabs group control-destroy combobox editable-combobox
		radio-buttons spinbox slider progressbar separator date-picker time-picker
		color-button font-button menu menu-checkbox open-file open-folder save-file
		message-box error-message-box timer queue-main should-quit controlgallery
		histogram drawtext datetime cpp-multithread
	)
endif()
if(MSVC)
	set(LIBUI_COMPILE_PDB_PATH
		"${CMAKE_CURRENT_BINARY_DIR}/pdb/$<CONFIG>/libui.pdb")
else()
	set(LIBUI_COMPILE_PDB_PATH "")
endif()
add_custom_target(stage-legacy
	COMMAND ${CMAKE_COMMAND}
		-DLIBUI_ARTIFACT=$<TARGET_FILE:libui_ui>
		-DLIBUI_LINKER_ARTIFACT=$<TARGET_LINKER_FILE:libui_ui>
		-DLIBUI_STAGE_DIR=${LIBUI_LEGACY_STAGE_DIR}
		-DLIBUI_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}
		-DLIBUI_RUNTIME_DIR=${CMAKE_BINARY_DIR}/artifacts/$<CONFIG>
		-DLIBUI_CONFIG=$<CONFIG>
		-DLIBUI_STATIC=$<NOT:$<BOOL:${BUILD_SHARED_LIBS}>>
		-DLIBUI_MSVC=${MSVC}
		-DLIBUI_PLATFORM_HEADER=${LIBUI_PLATFORM_HEADER}
		-DLIBUI_EXPECT_TESTS=${LIBUI_BUILD_TESTS}
		-DLIBUI_EXPECT_EXAMPLES=${LIBUI_BUILD_EXAMPLES}
		-DLIBUI_COMPILE_PDB=${LIBUI_COMPILE_PDB_PATH}
		-P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/StageLegacyRelease.cmake
	DEPENDS libui_ui ${LIBUI_LEGACY_RUNTIME_TARGETS}
	VERBATIM
)
