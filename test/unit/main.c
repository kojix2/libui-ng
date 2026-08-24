#include <stdio.h>
#include <string.h>

#include "unit.h"

struct unitTest {
	const char *name;
	int (*fn)(void);
};

int main(int argc, char **argv)
{
	size_t i;
	int failedTests = 0;
	int failedComponents = 0;
	struct unitTest unitTests[] = {
		{ "init", initRunUnitTests },
#ifdef _UI_STATIC
		{ "control", controlRunUnitTests },
#endif
		{ "window", windowRunUnitTests },
		{ "box", boxRunUnitTests },
		{ "group", groupRunUnitTests },
		{ "menu", menuRunUnitTests },
		{ "slider", sliderRunUnitTests },
		{ "spinbox", spinboxRunUnitTests },
		{ "label", labelRunUnitTests },
		{ "button", buttonRunUnitTests },
		{ "combobox", comboboxRunUnitTests },
		{ "editablecombobox", editableComboboxRunUnitTests },
		{ "checkbox", checkboxRunUnitTests },
		{ "radiobuttons", radioButtonsRunUnitTests },
		{ "tab", tabRunUnitTests },
		{ "entry", entryRunUnitTests },
		{ "multilineentry", multilineEntryRunUnitTests },
		{ "progressbar", progressBarRunUnitTests },
		{ "datetimepicker", dateTimePickerRunUnitTests },
		{ "separator", separatorRunUnitTests },
		{ "fontdescriptor", fontDescriptorRunUnitTests },
		{ "fontbutton", fontButtonRunUnitTests },
		{ "colorbutton", colorButtonRunUnitTests },
		{ "form", formRunUnitTests },
		{ "area", areaRunUnitTests },
		{ "image", imageRunUnitTests },
		{ "grid", gridRunUnitTests },
		{ "drawmatrix", drawMatrixRunUnitTests },
		{ "drawimage", drawImageRunUnitTests },
		{ "attrstr", attrstrRunUnitTests },
		{ "table", tableRunUnitTests },
		{ "imageview", imageViewRunUnitTests },
	};

	/* Preserve the full runner and its order; optional names select components. */
	if (argc == 2 && strcmp(argv[1], "--list") == 0) {
		for (i = 0; i < sizeof(unitTests)/sizeof(*unitTests); ++i)
			puts(unitTests[i].name);
		return 0;
	}
	for (int arg = 1; arg < argc; ++arg) {
		for (i = 0; i < sizeof(unitTests)/sizeof(*unitTests); ++i)
			if (strcmp(argv[arg], unitTests[i].name) == 0)
				break;
		if (i == sizeof(unitTests)/sizeof(*unitTests)) {
			fprintf(stderr, "Unknown component: %s (use --list)\n", argv[arg]);
			return EXIT_FAILURE;
		}
	}

	for (i = 0; i < sizeof(unitTests)/sizeof(*unitTests); ++i) {
		if (argc > 1) {
			int arg;
			for (arg = 1; arg < argc; ++arg)
				if (strcmp(argv[arg], unitTests[i].name) == 0)
					break;
			if (arg == argc)
				continue;
		}

		int fails = (unitTests[i].fn)();
		failedTests += fails;
		if (fails > 0)
			failedComponents++;
	}

	puts("[==========]");
	if (failedTests == 0)
		puts("[  PASSED  ] All test(s) in all component(s).");
	else
		printf("[  FAILED  ] %d test(s) in %d component(s), see above.\n",
			       failedTests, failedComponents);

	return failedTests;
}
