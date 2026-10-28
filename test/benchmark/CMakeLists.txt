add_executable(imagebuffer-bench imagebuffer.c)
target_link_libraries(imagebuffer-bench PRIVATE libui::ui)
libui_apply_internal_options(imagebuffer-bench)
