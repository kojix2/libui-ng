if(WIN32)
	set(unit_backend windows)
elseif(APPLE)
	set(unit_backend darwin)
else()
	set(unit_backend gtk)
endif()
if(BUILD_SHARED_LIBS)
	set(unit_linkage shared)
else()
	set(unit_linkage static)
endif()

set(unit_sources
	main.c init.c slider.c spinbox.c label.c button.c combobox.c checkbox.c
	radiobuttons.c tab.c entry.c menu.c progressbar.c datetimepicker.c grid.c
	drawmatrix.c drawimage.c attrstr.c imageview.c window.c box.c group.c
	editablecombobox.c multilineentry.c
	separator.c fontdescriptor.c fontbutton.c colorbutton.c form.c area.c image.c table.c
)
list(APPEND unit_sources table_fixture.c table_value.c table_width.c)
if(NOT BUILD_SHARED_LIBS)
	list(APPEND unit_sources control.c)
endif()
if(WIN32)
	list(APPEND unit_sources resources.rc)
	libui_add_test(windows-regression-unit regression_windows.c area_state_windows.cpp resources.rc)
	target_link_libraries(windows-regression-unit PRIVATE libui::ui cmocka::cmocka)
	set_tests_properties(windows-regression-unit PROPERTIES
		TIMEOUT 120 LABELS "contract;backend;gui;windows;${unit_linkage}")
endif()
libui_add_test(unit ${unit_sources})
target_link_libraries(unit PRIVATE libui_test_support)
if(WIN32)
	target_link_libraries(unit PRIVATE user32 comctl32)
endif()
set_tests_properties(unit PROPERTIES
	TIMEOUT 180 LABELS "contract;backend;gui;${unit_backend};${unit_linkage}")

libui_add_test(utf-unit utf.c ../../common/utf.c)
set_tests_properties(utf-unit PROPERTIES LABELS "logic;portable")
target_link_libraries(utf-unit PRIVATE cmocka::cmocka)
target_include_directories(utf-unit PRIVATE ../../common)

libui_add_test(opentype-unit opentype.c ../../common/opentype.c ../../common/attribute.c)
set_tests_properties(opentype-unit PROPERTIES LABELS "logic;contract;portable")
target_link_libraries(opentype-unit PRIVATE cmocka::cmocka)
target_compile_definitions(opentype-unit PRIVATE _UI_STATIC)

libui_add_test(allocsize-unit allocsize.c)
set_tests_properties(allocsize-unit PROPERTIES LABELS "logic;portable")
target_link_libraries(allocsize-unit PRIVATE cmocka::cmocka)

libui_add_test(shouldquit-unit shouldquit.c ../../common/shouldquit.c)
set_tests_properties(shouldquit-unit PROPERTIES LABELS "logic;portable")
target_link_libraries(shouldquit-unit PRIVATE cmocka::cmocka)
target_include_directories(shouldquit-unit PRIVATE ../.. ../../common)

libui_add_test(imagerep-unit imagerep.c ../../common/imagerep.c)
set_tests_properties(imagerep-unit PROPERTIES LABELS "logic;portable")
target_link_libraries(imagerep-unit PRIVATE cmocka::cmocka)
target_include_directories(imagerep-unit PRIVATE ../.. ../../common)

libui_add_test(gridutil-unit gridutil.c ../../common/gridutil.c)
set_tests_properties(gridutil-unit PROPERTIES LABELS "logic;portable")
target_link_libraries(gridutil-unit PRIVATE cmocka::cmocka)
target_include_directories(gridutil-unit PRIVATE ../.. ../../common)

if(UNIX AND NOT APPLE)
	target_sources(unit PRIVATE entry_gtk.c area_gtk.c)

	libui_add_test(alloc-unix alloc_backend.c ../../unix/alloc.c)
	set_tests_properties(alloc-unix PROPERTIES LABELS "backend;fault;gtk")
	target_link_libraries(alloc-unix PRIVATE cmocka::cmocka PkgConfig::GTK3)
	target_compile_definitions(alloc-unix PRIVATE _POSIX_C_SOURCE=200809L)

	libui_add_test(minimumsize-gtk minimumsize_gtk.c)
	set_tests_properties(minimumsize-gtk PROPERTIES LABELS "contract;backend;gui;gtk;${unit_linkage}")
	target_link_libraries(minimumsize-gtk PRIVATE libui::ui)

	foreach(component IN ITEMS datetimepicker slider)
		libui_add_test(${component}-gtk ${component}_gtk.c)
		set_tests_properties(${component}-gtk PROPERTIES LABELS "backend;gui;gtk;${unit_linkage}")
		target_link_libraries(${component}-gtk PRIVATE libui_test_support)
	endforeach()
endif()
if(APPLE)
	find_library(LIBUI_TEST_APPKIT_FRAMEWORK AppKit)
	if(NOT LIBUI_TEST_APPKIT_FRAMEWORK)
		message(FATAL_ERROR "AppKit is required for Darwin layout tests")
	endif()
	libui_add_test(layout-darwin layout_darwin.m)
	set_tests_properties(layout-darwin PROPERTIES LABELS "backend;gui;darwin;${unit_linkage}")
	target_link_libraries(layout-darwin PRIVATE libui::ui cmocka::cmocka
		${LIBUI_TEST_APPKIT_FRAMEWORK})

	libui_add_test(alloc-darwin alloc_backend.c alloc_darwin.m
		../../darwin/alloc.m)
	set_tests_properties(alloc-darwin PROPERTIES LABELS "backend;fault;darwin")
	target_link_libraries(alloc-darwin PRIVATE cmocka::cmocka
		${LIBUI_TEST_APPKIT_FRAMEWORK})
endif()

add_executable(imageviewrect-unit
	imageviewrect.c ../../common/imageview.c ../../common/imagerep.c
)
target_link_libraries(imageviewrect-unit PRIVATE cmocka::cmocka)
if(UNIX)
	target_link_libraries(imageviewrect-unit PRIVATE m)
endif()
target_include_directories(imageviewrect-unit PRIVATE ../.. ../../common)
libui_apply_internal_options(imageviewrect-unit)
add_test(NAME imageviewrect-unit COMMAND imageviewrect-unit)
set_tests_properties(imageviewrect-unit PROPERTIES TIMEOUT 60)
