# uiImageView (MVP)

## Purpose
- Minimal control for displaying images.
- Only property is ContentMode (Center / Fit[default]).

## Public API
- `uiNewImageView()`
- `uiImageViewSetImage(iv, image)` — Pass `NULL` to clear. Internally retains the pixel data (or equivalent native representation) of the passed `uiImage`. The original image can be freed after this call.
- `uiImageViewSetContentMode(iv, mode)` — `Center | Fit(default)`

## Content Modes (Detailed Specification)
- **Center**: Display image at 1:1 scale, centered in the view. May clip if image is larger than view, or leave margins if smaller.
- **Fit (default)**: Aspect-fit scaling (letterbox). Image is scaled to fit entirely within the view while maintaining aspect ratio. May leave margins on sides or top/bottom.

## Memory and Ownership (Important)
- This control uses "copy-owned" semantics.
  - When SetImage is called, the content of the `uiImage` is converted to native OS image format and stored internally.
  - After the call, the original `uiImage` can be safely freed.
  - When replacing an image, the old internal copy is automatically freed.
- Difference from uiTable
  - Image cells in `uiTable` are "non-owned" (no copying). The application must keep the `uiImage` alive while it's displayed.
  - `uiImageView` prioritizes safety by keeping its own internal copy.

## Default Behavior
- Images are always centered.
- Scaling follows the ContentMode setting.
- Interpolation uses high-quality filtering on all platforms:
  - macOS: Uses NSImageView's built-in scaling for Center/Fit modes
  - Linux/GTK: Uses Cairo's CAIRO_FILTER_GOOD for high-quality scaling
  - Windows: Uses D2D1 cubic interpolation for high-quality scaling
- Background is transparent or system default color.
- Color space assumes sRGB and converts to native OS format.
- HiDPI handling is delegated to the OS (drawing based on logical points).

## Sizing
- Final size is determined by the parent layout.
- Preferred size when no image is set: 64x64 (MVP).
- Minimum size constraints:
  - macOS: No minimum size constraint (fully flexible via Auto Layout)
  - Windows: 16x16 minimum size (supports icon usage)
  - Linux: 16x16 minimum size (supports icon usage)
- Images can be displayed smaller than their original size on all platforms.
- ContentMode only affects drawing behavior, not the preferred size (use stretch 0 if fixed size is sufficient).

## Notes
- All calls must be made from the UI thread.
- No getters are provided (can be added in the future if needed).
