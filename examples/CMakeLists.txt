set(libui_examples
	hello-world window button label checkbox entry multiline-entry box-layout
	form-layout grid-layout tabs group control-destroy combobox editable-combobox
	radio-buttons spinbox slider progressbar separator date-picker time-picker
	color-button font-button menu menu-checkbox open-file open-folder save-file
	message-box error-message-box timer queue-main should-quit
)

foreach(name IN LISTS libui_examples)
	add_executable(${name} "${name}.c")
	target_link_libraries(${name} PRIVATE libui::ui)
	libui_apply_internal_options(${name})
	libui_configure_application(${name})
endforeach()

foreach(name controlgallery histogram drawtext datetime)
	add_executable(${name} "${name}/main.c")
	target_link_libraries(${name} PRIVATE libui::ui)
	libui_apply_internal_options(${name})
	libui_configure_application(${name})
endforeach()

find_package(Threads REQUIRED)
add_executable(cpp-multithread cpp-multithread/main.cpp)
target_link_libraries(cpp-multithread PRIVATE libui::ui Threads::Threads)
libui_apply_internal_options(cpp-multithread)
libui_configure_application(cpp-multithread)
