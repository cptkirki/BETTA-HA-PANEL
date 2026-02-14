#!/usr/bin/env python3
"""
Generate LVGL converter inputs for Material Design Icons (MDI).

Outputs:
- Range string for LVGL Online Font Converter
- UTF-8 C macros for direct lv_label_set_text usage

Optional:
- Validate codepoints against a TTF using fontTools (if installed)
"""

from __future__ import annotations

import argparse
from pathlib import Path
from typing import Iterable, List, Optional

TOP50_HOME_AUTOMATION_ICONS = [
    "F0335", "F06E8", "F0031", "F033E", "F0531", "F0001", "F0710", "F0AC4",
    "F033F", "F0341", "F07EE", "F0C7E", "F05D1", "F0210", "F0E37", "F0302",
    "F081C", "F081D", "F06D3", "F026B", "F040A", "F04DB", "F057E", "F044C",
    "F0190", "F02D7", "F05E3", "F0318", "F0599", "F0595", "F059A", "F0585",
    "F02FD", "F00FB", "F050F", "F007D", "F0493", "F0019", "F0041", "F0156",
    "F001D", "F001C", "F0415", "F0623", "F035C", "F029C", "F0508", "F04B3",
    "F092E", "F024B",
]


def parse_codepoint(token: str) -> int:
    t = token.strip().upper()
    if not t:
        raise ValueError("empty icon token")
    if t.startswith("U+"):
        t = t[2:]
    if t.startswith("0X"):
        t = t[2:]
    return int(t, 16)


def unique_sorted(values: Iterable[int]) -> List[int]:
    return sorted(set(values))


def utf8_escape(cp: int) -> str:
    return "".join(f"\\x{b:02X}" for b in chr(cp).encode("utf-8"))


def range_string(codepoints: List[int]) -> str:
    return ",".join(f"0x{cp:X}" for cp in codepoints)


def load_tokens_from_file(path: Path) -> List[str]:
    tokens: List[str] = []
    for line in path.read_text(encoding="utf-8").splitlines():
        line = line.split("#", 1)[0].strip()
        if not line:
            continue
        for chunk in line.split(","):
            chunk = chunk.strip()
            if chunk:
                tokens.append(chunk)
    return tokens


def validate_with_ttf(font_path: Path, codepoints: List[int]) -> Optional[List[int]]:
    try:
        import fontTools.ttLib as ttlib  # type: ignore
    except Exception:
        print("validation skipped: fontTools not installed")
        return None

    cmap = ttlib.TTFont(str(font_path)).getBestCmap()
    return [cp for cp in codepoints if cp not in cmap]


def main() -> None:
    parser = argparse.ArgumentParser(description="Generate LVGL converter range/macros from MDI codepoints.")
    parser.add_argument(
        "--icon",
        action="append",
        default=[],
        help="Single icon codepoint, e.g. F06E8 or 0xF06E8 or U+F06E8 (repeatable).",
    )
    parser.add_argument(
        "--icons-file",
        type=Path,
        help="Text file with icon codepoints (comma or newline separated).",
    )
    parser.add_argument(
        "--use-top50",
        action="store_true",
        help="Use bundled top-50 home automation icon set.",
    )
    parser.add_argument(
        "--font",
        type=Path,
        help="Optional .ttf path for codepoint validation (requires fontTools).",
    )
    args = parser.parse_args()

    tokens: List[str] = []
    if args.use_top50:
        tokens.extend(TOP50_HOME_AUTOMATION_ICONS)
    if args.icons_file is not None:
        tokens.extend(load_tokens_from_file(args.icons_file))
    tokens.extend(args.icon)

    if not tokens:
        tokens = ["F06E8"]

    cps = unique_sorted(parse_codepoint(t) for t in tokens)

    print("Range:")
    print(range_string(cps))
    print()

    print("C UTF-8 macros:")
    for cp in cps:
        print(f'#define ICON_MDI_{cp:05X} "{utf8_escape(cp)}"')
    print()

    if args.font is not None:
        missing = validate_with_ttf(args.font, cps)
        if missing is not None:
            if missing:
                print("Missing in font:")
                print(",".join(f"0x{cp:X}" for cp in missing))
            else:
                print("All requested codepoints exist in font.")


if __name__ == "__main__":
    main()
