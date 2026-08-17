cmocka_deps = dependency('cmocka', version: '>=1.1.8',
	fallback : ['cmocka', 'cmocka_dep'], default_options: ['default_library=static'])

libui_unit_sources = [
	'main.c',
	'init.c',
	'slider.c',
        'spinbox.c',
        'label.c',
        'button.c',
        'combobox.c',
        'checkbox.c',
        'radiobuttons.c',
        'tab.c',
        'entry.c',
        'menu.c',
        'progressbar.c',
        'datetimepicker.c',
	    'grid.c',
        'drawmatrix.c',
        'attrstr.c',
        'window.c',
]
libui_unit_sources += libui_test_internal_sources

if libui_OS == 'windows'
	libui_unit_manifest = 'unit.manifest'
	if libui_mode == 'static'
		libui_unit_manifest = 'unit.static.manifest'
	endif
	libui_unit_sources += [
		windows.compile_resources('resources.rc',
			args: libui_manifest_args,
			depend_files: [libui_unit_manifest]),
	]
endif

unit = executable('unit', libui_unit_sources,
	dependencies: [libui_binary_deps, cmocka_deps],
	link_with: libui_libui,
	win_subsystem: 'console',
	install: false)

test('Unit Tests', unit)

utf_unit = executable('utf-unit', [
	'utf.c',
	'../../common/utf.c',
],
	dependencies: cmocka_deps,
	install: false)

test('UTF Unit Tests', utf_unit)
