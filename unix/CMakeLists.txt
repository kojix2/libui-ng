find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK3 REQUIRED IMPORTED_TARGET "gtk+-3.0>=3.18")

target_sources(libui_ui PRIVATE
	alloc.c area.c attrstr.c box.c button.c cellrendererbutton.c checkbox.c child.c
	colorbutton.c combobox.c control.c datetimepicker.c debug.c draw.c drawmatrix.c
	drawpath.c drawtext.c editablecombo.c entry.c fontbutton.c fontmatch.c form.c
	future.c graphemes.c grid.c group.c image.c label.c main.c menu.c multilineentry.c
	opentype.c progressbar.c radiobuttons.c separator.c slider.c spinbox.c stddialogs.c
	tab.c table.c tablemodel.c text.c toolbar.c util.c window.c
)
target_compile_definitions(libui_ui PRIVATE _POSIX_C_SOURCE=200809L)

# GTK types are part of ui_unix.h, so GTK's usage requirements are public.
target_link_libraries(libui_ui PUBLIC PkgConfig::GTK3)

include(CheckFunctionExists)
check_function_exists(tan LIBUI_TAN_IN_LIBC)
if(NOT LIBUI_TAN_IN_LIBC)
	set(CMAKE_REQUIRED_LIBRARIES m)
	check_function_exists(tan LIBUI_TAN_IN_LIBM)
	unset(CMAKE_REQUIRED_LIBRARIES)
	if(NOT LIBUI_TAN_IN_LIBM)
		message(FATAL_ERROR "tan() was not found in libc or libm")
	endif()
	target_link_libraries(libui_ui PRIVATE m)
endif()
if(CMAKE_DL_LIBS)
	target_link_libraries(libui_ui PRIVATE ${CMAKE_DL_LIBS})
endif()
