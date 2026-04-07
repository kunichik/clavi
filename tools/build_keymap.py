#!/usr/bin/env python3
"""
build_keymap.py — compile keyboard layout JSON files into keyboard_map.bin

Usage:
    python3 tools/build_keymap.py <source_layout.json> <target_layout.json> <output.bin>

The JSON format is a flat object mapping key names to Unicode characters:
    {"q": "й", "w": "ц", ...}

The output contains both directions (source->target AND target->source),
sorted by source codepoint for binary search.

Format (keyboard_map.bin):
    4-byte magic: KMAP
    4-byte LE u32: entry count
    N * 8-byte: [4-byte LE u32 source codepoint][4-byte LE u32 target codepoint]
    Sorted by source codepoint ascending.
"""

import sys
import json
import struct


def codepoint(ch: str) -> int:
    """Return the Unicode codepoint of the first character in the string."""
    if not ch:
        return 0
    return ord(ch[0])


def load_layout(path: str) -> dict[str, str]:
    with open(path, "r", encoding="utf-8") as f:
        return json.load(f)


def build_pairs(
    source_layout: dict[str, str],
    target_layout: dict[str, str],
) -> list[tuple[int, int]]:
    """
    For each key name present in both layouts, emit:
      source_char -> target_char
    Returns list of (source_cp, target_cp) sorted by source_cp.
    """
    pairs: list[tuple[int, int]] = []
    for key, src_char in source_layout.items():
        if key in target_layout:
            tgt_char = target_layout[key]
            src_cp = codepoint(src_char)
            tgt_cp = codepoint(tgt_char)
            if src_cp and tgt_cp and src_cp != tgt_cp:
                pairs.append((src_cp, tgt_cp))

    # Deduplicate by source
    seen: set[int] = set()
    deduped: list[tuple[int, int]] = []
    for pair in pairs:
        if pair[0] not in seen:
            seen.add(pair[0])
            deduped.append(pair)

    return sorted(deduped, key=lambda p: p[0])


def write_bin(pairs: list[tuple[int, int]], output_path: str) -> None:
    with open(output_path, "wb") as f:
        f.write(b"KMAP")
        f.write(struct.pack("<I", len(pairs)))
        for src_cp, tgt_cp in pairs:
            f.write(struct.pack("<I", src_cp))
            f.write(struct.pack("<I", tgt_cp))


def main() -> None:
    if len(sys.argv) != 4:
        print(
            f"Usage: {sys.argv[0]} <source_layout.json> <target_layout.json> <output.bin>",
            file=sys.stderr,
        )
        sys.exit(1)

    source_path = sys.argv[1]
    target_path = sys.argv[2]
    output_path = sys.argv[3]

    source_layout = load_layout(source_path)
    target_layout = load_layout(target_path)

    print(
        f"Source layout: {len(source_layout)} keys from {source_path}",
        file=sys.stderr,
    )
    print(
        f"Target layout: {len(target_layout)} keys from {target_path}",
        file=sys.stderr,
    )

    pairs = build_pairs(source_layout, target_layout)
    write_bin(pairs, output_path)

    print(
        f"Written {output_path}: {len(pairs)} key pairs "
        f"({len(pairs) * 8} bytes)",
        file=sys.stderr,
    )


if __name__ == "__main__":
    main()
