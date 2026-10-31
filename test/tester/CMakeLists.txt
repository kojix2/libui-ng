set(tester_sources
	main.c menus.c spaced.c images.c
	# Controls
	entries.c multiline.c numbers.c selection.c datetime.c control_state.c
	# Layout and ownership
	properties.c layout_parenting.c layout_visibility.c layout_sizes.c
	layout_form.c layout_grid.c
	# Drawing and input
	area.c area_arc.c area_mouse.c area_scroll.c drawtests.c text.c
	# Windows and dialogs
	windows.c dialogs.c
	# Tables
	table.c table_model.c
)
if(WIN32)
	list(APPEND tester_sources resources.rc)
endif()
add_executable(tester ${tester_sources})
target_link_libraries(tester PRIVATE libui::ui)
libui_apply_internal_options(tester)
