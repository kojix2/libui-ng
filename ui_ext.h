/*
This file assumes that you have included "ui.h" beforehand.
It provides unofficial APIs.
*/

#ifndef __LIBUI_UI_EXT_H__
#define __LIBUI_UI_EXT_H__

#ifdef __cplusplus
extern "C" {
#endif

// This might not work with some kinds of controls on Windows.
// See: https://github.com/libui-ng/libui-ng/pull/266
/**
 * Sets the control tooltip.
 *
 * @param c uiControl instance.
 * @param tooltip Control tooltip.\n
 *             A valid, `NULL` terminated UTF-8 string.\n
 *             Data is copied internally. Ownership is not transferred.
 * @note Setting `NULL` resets the tooltip to the default value.
 * @memberof uiControl
 */
_UI_EXTERN void uiControlSetTooltip(uiControl *c, const char *tooltip);

#ifdef __cplusplus
}
#endif

#endif
