# MDI Icon Font Workflow (LVGL Converter)

This project uses an optional generated LVGL font source:

- `main/ui/fonts/mdi_standard_icons_56.c`

If the file exists, it is compiled automatically and used by light tiles.
If it does not exist, the light tile falls back to `LV_SYMBOL_POWER`.

## 1) Generate converter input

From project root:

```powershell
python tools/mdi_to_lvgl_converter.py --use-top50 --font main/ui/materialdesignicons-webfont.ttf
```

Copy the printed `Range:` string.

## 2) LVGL Online Font Converter

Use the converter with:

- Font file: `materialdesignicons-webfont.ttf`
- Range: paste the generated range string
- BPP: your target setting (usually 4 for icons)
- Size: `56` (for current tile design)
- Font name: `mdi_standard_icons_56`
- Output format: C file

Save output as:

- `main/ui/fonts/mdi_standard_icons_56.c`

## 3) Build

Rebuild the firmware. No code change is needed; the font is auto-detected by CMake.

