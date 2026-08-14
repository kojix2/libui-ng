# `uiDrawImage()` implementation

## API contract

```c
void uiDrawImage(uiDrawContext *c, uiImage *img,
    double x, double y, double width, double height);
```

- `x`, `y`, `width`, and `height` are drawing-context logical units.
- The image is scaled to the destination rectangle.
- `img` is borrowed for this call. The function neither copies nor retains it.
- `x` and `y` must be finite. `width` and `height` must be finite and positive.
  Invalid or `NULL` arguments produce no drawing.
- As with the existing `uiImage` API, representations appended with
  `uiImageAppend()` contain premultiplied RGBA pixels.

## Representation selection

`uiImage` can contain multiple pixel representations of one logical image.
Selection for `uiDrawImage()` is based on the destination rather than only on
the logical size passed to `uiNewImage()`.

GTK and Windows calculate a target extent in device pixels from:

1. the destination `width` and `height`;
2. the current drawing transform, including scale, rotation, or shear; and
3. the widget scale factor on GTK or render-target DPI on Windows.

The transformed axis-aligned bounding-box extent is rounded up to whole
pixels. The shared private matcher then:

1. prefers representations at least as large as the target in both axes;
2. chooses the closest such representation;
3. falls back to the closest smaller representation when none is large enough;
4. uses deterministic dimension-based tie-breaking, independent of append
   order.

This destination-aware selector is used only by `uiDrawImage()`. Existing
consumers such as table image cells keep their legacy logical-size/DPI
selection behavior.

On macOS, `CGImageForProposedRect:context:hints:` receives the destination
rectangle, current graphics context, and current transform so AppKit can choose
the native representation.

## Backend rendering

### GTK

- The selected Cairo surface is scaled into the destination rectangle.
- Image surfaces use bilinear filtering.
- The `GtkWidget` associated with the `uiDrawContext` supplies the display
  scale factor.

### Windows

- The selected WIC bitmap is converted to an `ID2D1Bitmap` for the current
  render target.
- Direct2D draws it with linear interpolation.
- Existing `uiDrawClip()` state is applied around the draw.

### macOS

- AppKit selects an `NSImage` representation using the proposed destination
  and transform hints.
- Core Graphics draws the resulting `CGImage` with high interpolation quality
  and normal alpha blending.
- The implementation accounts for the flipped `NSView` coordinate system.

## Ownership relationships

- `uiDrawImage()` borrows its `uiImage` only during the call.
- `uiImageViewSetImage()` keeps an owned native copy or reference, so the
  caller may free the source image after the call.
- Table image cells remain non-owning; their source `uiImage` must stay valid
  while displayed.

## Tests and remaining platform checks

The common unit suite covers representation matching, destination geometry,
`uiImageView` ownership lifecycle, and invocation of `uiDrawImage()` through a
real `uiArea` draw callback.

Platform validation should additionally check:

- equivalent representation selection on GTK, Windows, and macOS;
- scale factors of 1 and 2 and enlarged destinations;
- opaque, transparent, and semitransparent premultiplied images;
- Windows ImageView backgrounds at 96 and 192 DPI in light and dark themes;
- normal Windows painting and `WM_PRINTCLIENT` output.

The implementation does not cache render-target-specific converted bitmaps.
Repeated calls can therefore recreate native drawing resources.
