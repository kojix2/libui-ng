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
	'drawimage.c',
	'attrstr.c',
	'imageview.c',
	'window.c',
	'box.c',
	'group.c',
	'editablecombobox.c',
	'multilineentry.c',
	'separator.c',
	'fontbutton.c',
	'colorbutton.c',
	'form.c',
	'area.c',
	'image.c',
	'table.c',
	'toolbar.c',
]
libui_unit_sources += libui_test_internal_sources

if libui_mode == 'static'
	# These tests exercise intentionally hidden common-layer helpers directly.
	libui_unit_sources += ['control.c']
endif

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

libui_unit_deps = [libui_binary_deps, cmocka_deps]
if libui_OS != 'windows' and libui_OS != 'darwin' and libui_mode != 'static'
	# Unix-only uiArea tests emit a native GTK event through the public handle.
	libui_unit_deps += libui_deps
endif

unit = executable('unit', libui_unit_sources,
	dependencies: libui_unit_deps,
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

imagerep_unit = executable('imagerep-unit', [
	'imagerep.c',
	'../../common/imagerep.c',
],
	dependencies: cmocka_deps,
	install: false)

test('Image Representation Unit Tests', imagerep_unit)

imageviewrect_unit = executable('imageviewrect-unit', [
	'imageviewrect.c',
	'../../common/imageview.c',
	'../../common/imagerep.c',
],
	dependencies: [cmocka_deps, libui_binary_deps],
	install: false)

test('Image View Rectangle Unit Tests', imageviewrect_unit)
