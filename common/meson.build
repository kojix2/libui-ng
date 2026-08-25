# 23 march 2019

libui_version_source = vcs_tag(
	input: 'version.c.in',
	output: 'version.c',
	fallback: 'unknown',
	command: ['git', 'describe', '--tags', '--always', '--dirty',
		'--abbrev=8', '--match=commit-*'],
)

libui_sources += [
	'common/attribute.c',
	'common/attrlist.c',
	'common/attrstr.c',
	'common/areaevents.c',
	'common/control.c',
	'common/debug.c',
	'common/imagerep.c',
	'common/matrix.c',
	'common/opentype.c',
	'common/shouldquit.c',
	'common/table.c',
	'common/tablemodel.c',
	'common/tablevalue.c',
	'common/toolbar.c',
	'common/userbugs.c',
	'common/utf.c',
	libui_version_source,
]
