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
