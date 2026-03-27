#ifndef __LIBUI_UNIT_H__
#define __LIBUI_UNIT_H__

#include "../support/fixture.h"

/**
 * Unit test run functions.
 */
int initRunUnitTests(void);
#ifdef _UI_STATIC
int controlRunUnitTests(void);
#endif
int windowRunUnitTests(void);
int boxRunUnitTests(void);
int groupRunUnitTests(void);
int sliderRunUnitTests(void);
int spinboxRunUnitTests(void);
int labelRunUnitTests(void);
int buttonRunUnitTests(void);
int comboboxRunUnitTests(void);
int editableComboboxRunUnitTests(void);
int checkboxRunUnitTests(void);
int radioButtonsRunUnitTests(void);
int tabRunUnitTests(void);
int entryRunUnitTests(void);
int multilineEntryRunUnitTests(void);
int menuRunUnitTests(void);
int progressBarRunUnitTests(void);
int dateTimePickerRunUnitTests(void);
int separatorRunUnitTests(void);
int fontDescriptorRunUnitTests(void);
int fontButtonRunUnitTests(void);
int colorButtonRunUnitTests(void);
int formRunUnitTests(void);
int areaRunUnitTests(void);
int imageRunUnitTests(void);
int gridRunUnitTests(void);
int drawMatrixRunUnitTests(void);
int drawImageRunUnitTests(void);
int attrstrRunUnitTests(void);
int tableRunUnitTests(void);
int imageViewRunUnitTests(void);
int tooltipRunUnitTests(void);

#endif
