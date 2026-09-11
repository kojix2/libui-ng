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
	add_subdirectory("${cmocka_SOURCE_DIR}" "${cmocka_BINARY_DIR}" EXCLUDE_FROM_ALL)
endif()

set(tester_sources
	drawtests.c images.c main.c menus.c page1.c page2.c page3.c page4.c page5.c
	page6.c page7.c page7a.c page7b.c page7c.c page10.c page11.c page12.c
	page13.c page14.c page15.c page16.c page17.c spaced.c
)
if(WIN32)
	list(APPEND tester_sources resources.rc)
endif()
add_executable(tester ${tester_sources})
target_link_libraries(tester PRIVATE libui::ui)
libui_apply_internal_options(tester)

add_subdirectory(unit)
add_subdirectory(qa)
