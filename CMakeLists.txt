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
		C_STANDARD 99
		C_STANDARD_REQUIRED ON
		C_EXTENSIONS OFF
		CXX_STANDARD 14
		CXX_STANDARD_REQUIRED ON
		CXX_EXTENSIONS OFF
	)
	# Keep the repository's release artifacts together, but do not override a
	# parent project's output layout when libui is used through add_subdirectory
	# or FetchContent.
	if(LIBUI_TOP_LEVEL)
		set_target_properties(${target} PROPERTIES
			RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/artifacts/$<CONFIG>"
		)
	endif()
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
		$<$<CXX_COMPILER_ID:MSVC>:/utf-8>
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
	if(LIBUI_TOP_LEVEL AND BUILD_SHARED_LIBS AND
			NOT "${target}" STREQUAL "libui_ui")
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

include(cmake/libui-ngApplication.cmake)

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
	COMPILE_PDB_NAME ui
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
if(LIBUI_TOP_LEVEL AND BUILD_SHARED_LIBS AND NOT WIN32)
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
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT Runtime
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT Runtime
		NAMELINK_COMPONENT Development
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT Development
	INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
install(FILES ui.h
	DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
	COMPONENT Development
)
if(WIN32)
	install(FILES ui_windows.h
		DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
		COMPONENT Development
	)
elseif(APPLE)
	install(FILES ui_darwin.h
		DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
		COMPONENT Development
	)
else()
	install(FILES ui_unix.h
		DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
		COMPONENT Development
	)
endif()
install(FILES LICENSE.md
	DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/doc/libui-ng
	COMPONENT Development
)

set(LIBUI_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/libui-ng")
configure_package_config_file(
	cmake/libui-ngConfig.cmake.in
	"${CMAKE_CURRENT_BINARY_DIR}/libui-ngConfig.cmake"
	INSTALL_DESTINATION "${LIBUI_INSTALL_CMAKEDIR}"
)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/libui-ngConfig.cmake"
	cmake/libui-ngApplication.cmake
	DESTINATION "${LIBUI_INSTALL_CMAKEDIR}"
	COMPONENT Development
)
if(WIN32)
	install(FILES
		cmake/windows/application.rc.in
		cmake/windows/application.manifest.in
		DESTINATION "${LIBUI_INSTALL_CMAKEDIR}/windows"
		COMPONENT Development
	)
endif()
install(EXPORT libui-ngTargets
	FILE libui-ngTargets.cmake
	NAMESPACE libui::
	DESTINATION "${LIBUI_INSTALL_CMAKEDIR}"
	COMPONENT Development
)

if(MSVC)
	install(FILES "${CMAKE_CURRENT_BINARY_DIR}/pdb/$<CONFIG>/libui.pdb"
		DESTINATION ${CMAKE_INSTALL_LIBDIR}
		COMPONENT DebugSymbols
		OPTIONAL
	)
	if(BUILD_SHARED_LIBS)
		install(FILES "$<TARGET_PDB_FILE:libui_ui>"
			DESTINATION ${CMAKE_INSTALL_BINDIR}
			COMPONENT DebugSymbols
			OPTIONAL
		)
	endif()
endif()

if(LIBUI_TOP_LEVEL)
	set(CPACK_GENERATOR ZIP)
	set(CPACK_PACKAGE_NAME libui-ng)
	set(CPACK_PACKAGE_VENDOR libui-ng)
	set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Simple and portable GUI library")
	set(CPACK_PACKAGE_VERSION unversioned)
	set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)
	set(CPACK_VERBATIM_VARIABLES ON)
	include(CPack)
endif()
