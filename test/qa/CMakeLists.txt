set(qa_sources
	qa.c main.c button.c checkbox.c entry.c label.c radiobuttons.c separator.c
	spinbox.c window.c
)
if(WIN32)
	list(APPEND qa_sources resources.rc)
endif()
add_executable(qa ${qa_sources})
target_link_libraries(qa PRIVATE libui::ui)
libui_apply_internal_options(qa)
