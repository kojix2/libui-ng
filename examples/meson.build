# 24 march 2019

libui_example_sources = []
libui_example_link_args = []
libui_example_cpp_extra_args = []
if libui_OS == 'windows'
	libui_example_manifest = 'example.manifest'
	if libui_mode == 'static'
		libui_example_manifest = 'example.static.manifest'
	endif
	libui_example_sources += [
		windows.compile_resources('resources.rc',
			args: libui_manifest_args,
			depend_files: [libui_example_manifest]),
	]
	# because Microsoft's toolchain is dumb
	if libui_MSVC
		libui_example_link_args += ['/ENTRY:mainCRTStartup']
	endif
elif libui_OS == 'darwin'
	# explicitly use the C++11-compatible standard library
	# see issue #302 for more details
	libui_example_cpp_extra_args += ['--stdlib=libc++']
endif

libui_examples = {
	'hello-world': {
		'sources':		['hello-world.c'],
	},
	'window': {
		'sources':		['window.c'],
	},
	'button': {
		'sources':		['button.c'],
	},
	'label': {
		'sources':		['label.c'],
	},
	'checkbox': {
		'sources':		['checkbox.c'],
	},
	'entry': {
		'sources':		['entry.c'],
	},
	'multiline-entry': {
		'sources':		['multiline-entry.c'],
	},
	'box-layout': {
		'sources':		['box-layout.c'],
	},
	'form-layout': {
		'sources':		['form-layout.c'],
	},
	'grid-layout': {
		'sources':		['grid-layout.c'],
	},
	'tabs': {
		'sources':		['tabs.c'],
	},
	'group': {
		'sources':		['group.c'],
	},
	'control-destroy': {
		'sources':		['control-destroy.c'],
	},
	'combobox': {
		'sources':		['combobox.c'],
	},
	'editable-combobox': {
		'sources':		['editable-combobox.c'],
	},
	'radio-buttons': {
		'sources':		['radio-buttons.c'],
	},
	'spinbox': {
		'sources':		['spinbox.c'],
	},
	'slider': {
		'sources':		['slider.c'],
	},
	'progressbar': {
		'sources':		['progressbar.c'],
	},
	'separator': {
		'sources':		['separator.c'],
	},
	'date-picker': {
		'sources':		['date-picker.c'],
	},
	'time-picker': {
		'sources':		['time-picker.c'],
	},
	'color-button': {
		'sources':		['color-button.c'],
	},
	'font-button': {
		'sources':		['font-button.c'],
	},
	'menu': {
		'sources':		['menu.c'],
	},
	'menu-checkbox': {
		'sources':		['menu-checkbox.c'],
	},
	'open-file': {
		'sources':		['open-file.c'],
	},
	'open-folder': {
		'sources':		['open-folder.c'],
	},
	'save-file': {
		'sources':		['save-file.c'],
	},
	'message-box': {
		'sources':		['message-box.c'],
	},
	'error-message-box': {
		'sources':		['error-message-box.c'],
	},
	'timer': {
		'sources':		['timer.c'],
	},
	'queue-main': {
		'sources':		['queue-main.c'],
	},
	'should-quit': {
		'sources':		['should-quit.c'],
	},
	'controlgallery': {
		'sources':		['controlgallery/main.c'],
	},
	'histogram': {
		'sources':		['histogram/main.c'],
	},
	'cpp-multithread': {
		'sources':		['cpp-multithread/main.cpp'],
		'deps':		[
			dependency('threads',
				required: true),
		],
		'cpp_args':	libui_example_cpp_extra_args,
		'link_args':	libui_example_cpp_extra_args,
	},
	'drawtext': {
		'sources':		['drawtext/main.c'],
	},
	'datetime': {
		'sources':		['datetime/main.c'],
	},
}
foreach name, args : libui_examples
	executable(name, args['sources'] + libui_example_sources,
		dependencies: args.get('deps', []) + libui_binary_deps,
		link_with: libui_libui,
		cpp_args: args.get('cpp_args', []),
		link_args: args.get('link_args', []) + libui_example_link_args,
		include_directories: include_directories('..'),
		win_subsystem: 'console',
		install: false)
endforeach
