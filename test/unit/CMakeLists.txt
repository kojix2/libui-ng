set(unit_sources
	main.c init.c slider.c spinbox.c label.c button.c combobox.c checkbox.c
	radiobuttons.c tab.c entry.c menu.c progressbar.c datetimepicker.c grid.c
	drawmatrix.c attrstr.c window.c box.c group.c editablecombobox.c multilineentry.c
	separator.c fontdescriptor.c fontbutton.c colorbutton.c form.c area.c image.c table.c
)
if(NOT BUILD_SHARED_LIBS)
	list(APPEND unit_sources control.c)
endif()
if(WIN32)
	list(APPEND unit_sources resources.rc)
endif()
add_executable(unit ${unit_sources})
target_link_libraries(unit PRIVATE libui::ui cmocka::cmocka)
libui_apply_internal_options(unit)
add_test(NAME unit COMMAND unit)
set_tests_properties(unit PROPERTIES TIMEOUT 180)

add_executable(utf-unit utf.c ../../common/utf.c)
target_link_libraries(utf-unit PRIVATE cmocka::cmocka)
target_include_directories(utf-unit PRIVATE ../../common)
libui_apply_internal_options(utf-unit)
add_test(NAME utf-unit COMMAND utf-unit)
set_tests_properties(utf-unit PROPERTIES TIMEOUT 60)

add_executable(imagerep-unit imagerep.c ../../common/imagerep.c)
target_link_libraries(imagerep-unit PRIVATE cmocka::cmocka)
target_include_directories(imagerep-unit PRIVATE ../.. ../../common)
libui_apply_internal_options(imagerep-unit)
add_test(NAME imagerep-unit COMMAND imagerep-unit)
set_tests_properties(imagerep-unit PROPERTIES TIMEOUT 60)
