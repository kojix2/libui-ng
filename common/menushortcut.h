#ifndef UIPRIV_MENU_SHORTCUT_H
#define UIPRIV_MENU_SHORTCUT_H

/* All backends use the same public validation and modifier normalization. */
typedef char uiprivMenuShortcutModifierBitsDistinct[
	(uiModifierPrimary &
		(uiModifierCtrl | uiModifierAlt | uiModifierShift | uiModifierSuper)) == 0
		? 1 : -1];

/* uiExtKey constants and ASCII digit/letter literals share one key argument. */
typedef char uiprivMenuShortcutKeyValuesDistinct[
	uiExtKeyNDivide < '0' ? 1 : -1];

static void uiprivValidateMenuShortcut(unsigned int key,
	uiModifiers modifiers)
{
	unsigned int allowed = uiModifierPrimary |
		uiModifierCtrl | uiModifierAlt |
		uiModifierShift;
	int plainAllowed;

	if (modifiers & ~allowed)
		uiprivUserBug("Unsupported menu shortcut modifier (Super is not allowed).");
	if (key == uiExtKeyNone) {
		if (modifiers != 0)
			uiprivUserBug("Clearing a menu shortcut requires zero modifiers.");
		return;
	}
	plainAllowed = key >= uiExtKeyF1 && key <= uiExtKeyF12;
	if (!plainAllowed &&
		key != uiExtKeyDelete &&
		!(key >= 'A' && key <= 'Z') &&
		!(key >= '0' && key <= '9') &&
		!(key >= uiExtKeyUp && key <= uiExtKeyRight))
		uiprivUserBug("Unsupported menu shortcut key.");
	if ((modifiers & uiModifierPrimary) &&
		(modifiers & uiModifierCtrl))
		uiprivUserBug("Primary and Ctrl cannot be combined in a menu shortcut.");
	if (!plainAllowed && !(modifiers & (uiModifierPrimary |
		uiModifierCtrl | uiModifierAlt)))
		uiprivUserBug("A letter, digit, Delete, or arrow shortcut requires Primary, Ctrl, or Alt.");
}

static inline uiModifiers uiprivShortcutEffectiveModifiers(
	uiModifiers modifiers)
{
#ifndef __APPLE__
	if (modifiers & uiModifierPrimary) {
		modifiers &= ~uiModifierPrimary;
		modifiers |= uiModifierCtrl;
	}
#endif
	return modifiers;
}

#endif
