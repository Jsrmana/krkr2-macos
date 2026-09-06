# shrinkCopy integration

Upstream: https://github.com/wtnbgo/shrinkCopy
Revision: 007fbba3b3531b476d6b97e7ee80553868f31b2d
Author: miahmie. The original readme.txt and manual.tjs are retained.
The upstream readme states that the plugin follows the KiriKiri engine license.

Changes:
- Register shrinkCopy and shrinkCopyFast as Layer methods in the shrinkCopy.dll internal module.
- Replace the obsolete TJS_INTF_METHOD declaration with this engine's calling convention.
- Use uint32_t for four-byte pixels; unsigned long is eight bytes on arm64 macOS.
- Use uint64_t averaging accumulators to avoid overflow for large reductions.
- Allocate horizontal plus vertical averaging entries (w+h), rather than w*h; the original calculation underallocates for a one-pixel output dimension. Avoid pointer arithmetic on failed allocations.
- Split registration from the implementation so pixel-level tests can exercise it.

Validation: RGBA averaging, one-pixel output, negative pitch, destination clipping,
enlargement rejection, fast reduction with remainder edges, and loading/calling the
registered module through the TJS dispatch interface. All existing CTest tests pass.
