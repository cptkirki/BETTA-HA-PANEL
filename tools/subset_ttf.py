#!/usr/bin/env python3
"""
Create a tiny TrueType subset by stripping unused glyph outlines from the glyf table.

The output keeps original glyph indices/cmap and only rewrites:
- glyf
- loca
- head.checkSumAdjustment

This is enough for LVGL tiny_ttf/stb_truetype while shrinking icon fonts heavily.
"""

from __future__ import annotations

import argparse
import struct
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, Iterable, List, Set, Tuple


@dataclass
class TableRecord:
    tag: bytes
    checksum: int
    offset: int
    length: int


def be_u16(buf: bytes, off: int) -> int:
    return struct.unpack_from(">H", buf, off)[0]


def be_i16(buf: bytes, off: int) -> int:
    return struct.unpack_from(">h", buf, off)[0]


def be_u32(buf: bytes, off: int) -> int:
    return struct.unpack_from(">I", buf, off)[0]


def parse_tables(ttf: bytes) -> Dict[bytes, TableRecord]:
    num_tables = be_u16(ttf, 4)
    tables: Dict[bytes, TableRecord] = {}
    for i in range(num_tables):
        off = 12 + i * 16
        tag, checksum, offset, length = struct.unpack_from(">4sIII", ttf, off)
        tables[tag] = TableRecord(tag=tag, checksum=checksum, offset=offset, length=length)
    return tables


def table_bytes(ttf: bytes, rec: TableRecord) -> bytes:
    return ttf[rec.offset : rec.offset + rec.length]


def parse_loca(ttf: bytes, tables: Dict[bytes, TableRecord], num_glyphs: int) -> List[int]:
    head = table_bytes(ttf, tables[b"head"])
    loca = table_bytes(ttf, tables[b"loca"])
    index_to_loc_format = be_i16(head, 50)
    offsets: List[int] = []
    if index_to_loc_format == 0:
        for i in range(num_glyphs + 1):
            offsets.append(be_u16(loca, i * 2) * 2)
    else:
        for i in range(num_glyphs + 1):
            offsets.append(be_u32(loca, i * 4))
    return offsets


def gid_for_codepoint(ttf: bytes, tables: Dict[bytes, TableRecord], codepoint: int) -> int:
    cmap = table_bytes(ttf, tables[b"cmap"])
    num_subtables = be_u16(cmap, 2)

    best_gid = 0

    for i in range(num_subtables):
        rec_off = 4 + i * 8
        _platform_id, _encoding_id, sub_off = struct.unpack_from(">HHI", cmap, rec_off)
        fmt = be_u16(cmap, sub_off)

        if fmt == 12:
            n_groups = be_u32(cmap, sub_off + 12)
            group_off = sub_off + 16
            for _ in range(n_groups):
                start_char = be_u32(cmap, group_off)
                end_char = be_u32(cmap, group_off + 4)
                start_gid = be_u32(cmap, group_off + 8)
                if start_char <= codepoint <= end_char:
                    return start_gid + (codepoint - start_char)
                group_off += 12
            continue

        if fmt != 4 or codepoint > 0xFFFF:
            continue

        seg_count = be_u16(cmap, sub_off + 6) // 2
        end_off = sub_off + 14
        start_off = end_off + seg_count * 2 + 2
        delta_off = start_off + seg_count * 2
        range_off_off = delta_off + seg_count * 2

        for seg in range(seg_count):
            end_code = be_u16(cmap, end_off + seg * 2)
            start_code = be_u16(cmap, start_off + seg * 2)
            if codepoint < start_code or codepoint > end_code:
                continue

            id_delta = be_i16(cmap, delta_off + seg * 2)
            id_range_offset = be_u16(cmap, range_off_off + seg * 2)
            if id_range_offset == 0:
                gid = (codepoint + id_delta) & 0xFFFF
            else:
                ro_addr = range_off_off + seg * 2
                glyph_index_addr = ro_addr + id_range_offset + (codepoint - start_code) * 2
                if glyph_index_addr + 2 > len(cmap):
                    gid = 0
                else:
                    glyph_index = be_u16(cmap, glyph_index_addr)
                    gid = 0 if glyph_index == 0 else ((glyph_index + id_delta) & 0xFFFF)
            if gid != 0:
                best_gid = gid
                break

    return best_gid


def composite_refs(glyph: bytes) -> List[int]:
    if len(glyph) < 10:
        return []
    n_contours = be_i16(glyph, 0)
    if n_contours >= 0:
        return []

    refs: List[int] = []
    off = 10
    more = True

    ARG_1_AND_2_ARE_WORDS = 0x0001
    WE_HAVE_A_SCALE = 0x0008
    MORE_COMPONENTS = 0x0020
    WE_HAVE_AN_X_AND_Y_SCALE = 0x0040
    WE_HAVE_A_TWO_BY_TWO = 0x0080

    while more and (off + 4) <= len(glyph):
        flags = be_u16(glyph, off)
        gidx = be_u16(glyph, off + 2)
        refs.append(gidx)
        off += 4

        off += 4 if (flags & ARG_1_AND_2_ARE_WORDS) else 2

        if flags & WE_HAVE_A_SCALE:
            off += 2
        elif flags & WE_HAVE_AN_X_AND_Y_SCALE:
            off += 4
        elif flags & WE_HAVE_A_TWO_BY_TWO:
            off += 8

        more = (flags & MORE_COMPONENTS) != 0

    return refs


def compute_checksum(data: bytes) -> int:
    pad = (-len(data)) % 4
    if pad:
        data += b"\0" * pad
    total = 0
    for i in range(0, len(data), 4):
        total = (total + be_u32(data, i)) & 0xFFFFFFFF
    return total


def make_ttf(tables: Dict[bytes, bytes]) -> bytes:
    tags = sorted(tables.keys())
    num_tables = len(tags)

    max_pow2 = 1
    entry_selector = 0
    while (max_pow2 << 1) <= num_tables:
        max_pow2 <<= 1
        entry_selector += 1
    search_range = max_pow2 * 16
    range_shift = num_tables * 16 - search_range

    header = struct.pack(">IHHHH", 0x00010000, num_tables, search_range, entry_selector, range_shift)

    offset = 12 + num_tables * 16
    records: List[bytes] = []
    payload = bytearray()

    for tag in tags:
        data = tables[tag]
        chk = compute_checksum(data)
        records.append(struct.pack(">4sIII", tag, chk, offset, len(data)))
        payload.extend(data)
        pad = (-len(data)) % 4
        if pad:
            payload.extend(b"\0" * pad)
        offset += len(data) + pad

    return header + b"".join(records) + bytes(payload)


def set_head_checksum_adjustment(font: bytes) -> bytes:
    num_tables = be_u16(font, 4)
    head_offset = -1
    for i in range(num_tables):
        rec_off = 12 + i * 16
        tag = font[rec_off : rec_off + 4]
        if tag == b"head":
            head_offset = be_u32(font, rec_off + 8)
            break
    if head_offset < 0:
        raise RuntimeError("No 'head' table found")

    mutable = bytearray(font)
    struct.pack_into(">I", mutable, head_offset + 8, 0)

    total = compute_checksum(bytes(mutable))
    adjust = (0xB1B0AFBA - total) & 0xFFFFFFFF
    struct.pack_into(">I", mutable, head_offset + 8, adjust)
    return bytes(mutable)


def subset_ttf(input_path: Path, output_path: Path, codepoints: Iterable[int]) -> Tuple[int, int]:
    src = input_path.read_bytes()
    tables = parse_tables(src)
    required = [b"head", b"hhea", b"hmtx", b"maxp", b"loca", b"glyf", b"cmap"]
    for tag in required:
        if tag not in tables:
            raise RuntimeError(f"Missing required table: {tag.decode()}")

    maxp = table_bytes(src, tables[b"maxp"])
    num_glyphs = be_u16(maxp, 4)
    loca = parse_loca(src, tables, num_glyphs)
    glyf = table_bytes(src, tables[b"glyf"])

    keep: Set[int] = {0}  # .notdef
    for cp in codepoints:
        gid = gid_for_codepoint(src, tables, cp)
        if gid != 0:
            keep.add(gid)

    queue = list(keep)
    seen = set(queue)
    while queue:
        gid = queue.pop()
        if gid < 0 or gid >= num_glyphs:
            continue
        start = loca[gid]
        end = loca[gid + 1]
        if end <= start or end > len(glyf):
            continue
        refs = composite_refs(glyf[start:end])
        for ref in refs:
            if ref not in seen:
                seen.add(ref)
                keep.add(ref)
                queue.append(ref)

    new_glyf = bytearray()
    new_loca: List[int] = [0] * (num_glyphs + 1)

    for gid in range(num_glyphs):
        new_loca[gid] = len(new_glyf)
        if gid in keep:
            start = loca[gid]
            end = loca[gid + 1]
            if 0 <= start <= end <= len(glyf):
                new_glyf.extend(glyf[start:end])
    new_loca[num_glyphs] = len(new_glyf)

    head = bytearray(table_bytes(src, tables[b"head"]))
    index_to_loc_format = be_i16(head, 50)

    if index_to_loc_format == 0:
        loca_data = bytearray()
        for off in new_loca:
            if (off % 2) != 0:
                raise RuntimeError("loca short format requires even offsets")
            loca_data.extend(struct.pack(">H", off // 2))
    else:
        loca_data = bytearray()
        for off in new_loca:
            loca_data.extend(struct.pack(">I", off))

    # Keep only the tables required by stb_truetype/tiny_ttf (+ a couple of small metadata tables).
    keep_tables = {
        b"cmap",
        b"glyf",
        b"head",
        b"hhea",
        b"hmtx",
        b"loca",
        b"maxp",
        b"OS/2",
        b"name",
    }

    new_tables: Dict[bytes, bytes] = {}
    for tag, rec in tables.items():
        if tag not in keep_tables:
            continue
        if tag == b"glyf":
            new_tables[tag] = bytes(new_glyf)
        elif tag == b"loca":
            new_tables[tag] = bytes(loca_data)
        else:
            new_tables[tag] = table_bytes(src, rec)

    rebuilt = make_ttf(new_tables)
    rebuilt = set_head_checksum_adjustment(rebuilt)
    output_path.write_bytes(rebuilt)
    return len(src), len(rebuilt)


def parse_codepoint(value: str) -> int:
    value = value.strip().lower()
    if value.startswith("u+"):
        value = value[2:]
    if value.startswith("0x"):
        value = value[2:]
    return int(value, 16)


def main() -> None:
    parser = argparse.ArgumentParser(description="Subset a TTF by keeping only selected glyph outlines.")
    parser.add_argument("input", type=Path, help="Input .ttf path")
    parser.add_argument("output", type=Path, help="Output subset .ttf path")
    parser.add_argument(
        "--codepoint",
        action="append",
        dest="codepoints",
        default=[],
        help="Codepoint to preserve (repeatable), e.g. --codepoint U+F06E8",
    )
    args = parser.parse_args()

    cp_args = args.codepoints if args.codepoints else ["0xF06E8"]
    cps = [parse_codepoint(cp) for cp in cp_args]
    src_size, dst_size = subset_ttf(args.input, args.output, cps)
    print(f"subset written: {args.output}")
    print(f"codepoints: {[hex(cp) for cp in cps]}")
    print(f"size: {src_size} -> {dst_size} bytes ({(100.0 * dst_size / src_size):.1f}%)")


if __name__ == "__main__":
    main()
