# Keep platform libraries, resources, labels and exceptional properties at the
# call site. Logic-only tests do not acquire a dependency on libui or its GUI.
function(libui_add_test target)
	add_executable(${target} ${ARGN})
	libui_apply_internal_options(${target})
	add_test(NAME ${target} COMMAND ${target})
	set_tests_properties(${target} PROPERTIES TIMEOUT 60)
endfunction()
