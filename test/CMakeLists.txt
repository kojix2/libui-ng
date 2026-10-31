include(FetchContent)

find_package(cmocka 1.1.8 CONFIG QUIET)
if(NOT TARGET cmocka::cmocka)
	find_package(PkgConfig QUIET)
	if(PkgConfig_FOUND)
		pkg_check_modules(CMOCKA QUIET IMPORTED_TARGET "cmocka>=1.1.8")
		if(TARGET PkgConfig::CMOCKA)
			add_library(libui_cmocka_system INTERFACE)
			target_link_libraries(libui_cmocka_system INTERFACE PkgConfig::CMOCKA)
			add_library(cmocka::cmocka ALIAS libui_cmocka_system)
		endif()
	endif()
endif()
if(NOT TARGET cmocka::cmocka)
	if(NOT LIBUI_FETCH_TEST_DEPS)
		message(FATAL_ERROR
			"cmocka >= 1.1.8 is required for LIBUI_BUILD_TESTS; install it or set LIBUI_FETCH_TEST_DEPS=ON")
	endif()
	set(WITH_EXAMPLES OFF CACHE BOOL "" FORCE)
	set(UNIT_TESTING OFF CACHE BOOL "" FORCE)
	set(PICKY_DEVELOPER OFF CACHE BOOL "" FORCE)
	if(POLICY CMP0135)
		cmake_policy(SET CMP0135 NEW)
	endif()
	if(FETCHCONTENT_SOURCE_DIR_CMOCKA)
		set(cmocka_SOURCE_DIR "${FETCHCONTENT_SOURCE_DIR_CMOCKA}")
		set(cmocka_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/cmocka-build")
	else()
		FetchContent_Populate(cmocka
			URL https://cmocka.org/files/1.1/cmocka-1.1.8.tar.xz
			URL_HASH SHA256=58435b558766d7f4c729ba163bdf3aec38bed3bc766dab684e3526ed0aa7c780
		)
	endif()
	# Keep the test-only dependency static even when libui itself is shared.
	# libui_ui's type has already been selected, so temporarily changing this
	# directory variable affects only cmocka's add_library() call.
	set(_libui_saved_build_shared_libs "${BUILD_SHARED_LIBS}")
	set(BUILD_SHARED_LIBS OFF)
	add_subdirectory("${cmocka_SOURCE_DIR}" "${cmocka_BINARY_DIR}" EXCLUDE_FROM_ALL)
	set(BUILD_SHARED_LIBS "${_libui_saved_build_shared_libs}")
	unset(_libui_saved_build_shared_libs)
endif()

add_subdirectory(tester)

include(AddTest.cmake)
add_subdirectory(support)
add_subdirectory(unit)
add_subdirectory(qa)
