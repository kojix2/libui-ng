find_library(LIBUI_FOUNDATION_FRAMEWORK Foundation)
find_library(LIBUI_APPKIT_FRAMEWORK AppKit)
find_library(LIBUI_OBJC_LIBRARY objc)
if(NOT LIBUI_FOUNDATION_FRAMEWORK OR NOT LIBUI_APPKIT_FRAMEWORK OR NOT LIBUI_OBJC_LIBRARY)
	message(FATAL_ERROR "Foundation, AppKit, and the Objective-C runtime are required")
endif()

target_sources(libui_ui PRIVATE
	alloc.m area.m areaevents.m attrstr.m autolayout.m box.m button.m checkbox.m
	colorbutton.m combobox.m control.m datetimepicker.m debug.m draw.m drawtext.m
	editablecombo.m entry.m event.m fontbutton.m fontmatch.m fonttraits.m
	fontvariation.m form.m graphemes.m grid.m group.m image.m label.m main.m menu.m
	multilineentry.m opentype.m progressbar.m radiobuttons.m scrollview.m separator.m
	slider.m spinbox.m stddialogs.m tab.m table.m tablecolumn.m text.m undocumented.m
	util.m window.m winmoveresize.m nstextfield.m
)
target_link_libraries(libui_ui PRIVATE
	$<BUILD_INTERFACE:${LIBUI_FOUNDATION_FRAMEWORK}>
	$<BUILD_INTERFACE:${LIBUI_APPKIT_FRAMEWORK}>
	$<BUILD_INTERFACE:${LIBUI_OBJC_LIBRARY}>
	$<INSTALL_INTERFACE:libui::AppleDependencies>
)
