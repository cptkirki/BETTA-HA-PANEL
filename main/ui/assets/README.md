# Splash House Image (Optional)

You can replace the boot splash center emblem with a static image.

## Files

Place these files in this folder:

- `splash_house_image.h`
- `splash_house_image.c`

The symbol name must be:

- `splash_house_image`

Example declaration in `splash_house_image.h`:

```c
#pragma once
#include "lvgl.h"
LV_IMAGE_DECLARE(splash_house_image);
```

## Important for performance/stability

- Use LVGL image converter output as C array (not runtime JPG decode on target).
- Keep the image at near-final display size (avoid runtime scaling/rotation).
- Prefer ARGB8888 or RGB565A8 output from converter.

This avoids expensive software image transform paths on ESP32-P4 during boot splash.
