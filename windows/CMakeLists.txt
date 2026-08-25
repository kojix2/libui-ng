if(MINGW AND BUILD_SHARED_LIBS)
	message(FATAL_ERROR "MinGW and UCRT builds currently support static libui only")
endif()

target_sources(libui_ui PRIVATE
	alloc.cpp area.cpp areadraw.cpp areaevents.cpp areascroll.cpp areautil.cpp
	attrstr.cpp box.cpp button.cpp checkbox.cpp colorbutton.cpp colordialog.cpp
	combobox.cpp container.cpp control.cpp d2dscratch.cpp datetimepicker.cpp debug.cpp
	draw.cpp drawmatrix.cpp drawpath.cpp drawtext.cpp dwrite.cpp editablecombo.cpp
	entry.cpp events.cpp fontbutton.cpp fontdialog.cpp fontmatch.cpp form.cpp
	graphemes.cpp grid.cpp group.cpp image.cpp init.cpp label.cpp main.cpp menu.cpp
	multilineentry.cpp opentype.cpp parent.cpp progressbar.cpp radiobuttons.cpp
	separator.cpp sizing.cpp slider.cpp spinbox.cpp stddialogs.cpp tab.cpp table.cpp
	tabledispinfo.cpp tabledraw.cpp tableediting.cpp tablemetrics.cpp tabpage.cpp
	text.cpp toolbar.cpp utf16.cpp utilwin.cpp window.cpp winpublic.cpp winutil.cpp
)
if(BUILD_SHARED_LIBS)
	target_sources(libui_ui PRIVATE resources.rc)
endif()
target_link_libraries(libui_ui PRIVATE
	user32 kernel32 gdi32 comctl32 uxtheme d2d1 dwrite ole32 uuid windowscodecs
)
if(MINGW AND NOT BUILD_SHARED_LIBS)
	# These are final-link requirements of the static archive, including for
	# installed consumers written in C.
	target_link_options(libui_ui INTERFACE -static -static-libgcc -static-libstdc++)
endif()
