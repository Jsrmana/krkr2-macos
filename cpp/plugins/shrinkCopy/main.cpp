#define NCB_MODULE_NAME TJS_W("shrinkCopy.dll")
#include "ShrinkCopy.h"

NCB_ATTACH_FUNCTION(shrinkCopy, Layer, ShrinkCopy::layerShrinkCopy);
NCB_ATTACH_FUNCTION(shrinkCopyFast, Layer, LimitedShrink::layerShrinkCopy);
