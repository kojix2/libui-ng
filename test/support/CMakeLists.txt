# Test-only fixture: never installed or linked into libui itself.
add_library(libui_test_support STATIC fixture.c)
target_link_libraries(libui_test_support PUBLIC libui::ui cmocka::cmocka)
libui_apply_internal_options(libui_test_support)
