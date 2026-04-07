#!/usr/bin/env python3
"""
build_dict.py — compile a word list into dictionary.bin

Usage:
    python3 tools/build_dict.py <wordlist.txt> <output.bin>

Format (dictionary.bin):
    4-byte magic: CLAV
    4-byte LE u32: entry count
    N * 8-byte LE u64: open-addressing hash table (xxh3_64 hashes)
    Load factor <= 0.7, table size = next power of 2 >= ceil(count / 0.7)
    Empty slot = 0x0000000000000000
"""

import sys
import math
import struct
import xxhash


def next_power_of_2(n: int) -> int:
    if n <= 1:
        return 1
    p = 1
    while p < n:
        p <<= 1
    return p


def hash_word(word: str) -> int:
    h = xxhash.xxh3_64(word.encode("utf-8")).intdigest()
    return h if h != 0 else 1  # 0 is sentinel for empty slot


def build_table(words: list[str]) -> list[int]:
    count = len(words)
    table_size = next_power_of_2(math.ceil(count / 0.7))
    table = [0] * table_size
    mask = table_size - 1

    for word in words:
        h = hash_word(word.lower())
        idx = h & mask
        while table[idx] != 0:
            idx = (idx + 1) & mask
        table[idx] = h

    return table


def main() -> None:
    if len(sys.argv) != 3:
        print(f"Usage: {sys.argv[0]} <wordlist.txt> <output.bin>", file=sys.stderr)
        sys.exit(1)

    wordlist_path = sys.argv[1]
    output_path = sys.argv[2]

    # Load words
    words: list[str] = []
    with open(wordlist_path, "r", encoding="utf-8") as f:
        for line in f:
            word = line.strip()
            if word and not word.startswith("#"):
                words.append(word)

    print(f"Loaded {len(words)} words from {wordlist_path}", file=sys.stderr)

    # Deduplicate (lowercase)
    seen: set[str] = set()
    unique: list[str] = []
    for w in words:
        lw = w.lower()
        if lw not in seen:
            seen.add(lw)
            unique.append(lw)

    print(f"Unique (lowercased): {len(unique)} words", file=sys.stderr)

    table = build_table(unique)

    with open(output_path, "wb") as f:
        f.write(b"CLAV")
        f.write(struct.pack("<I", len(unique)))
        for slot in table:
            f.write(struct.pack("<Q", slot))

    print(
        f"Written {output_path}: {len(unique)} words, table_size={len(table)} slots "
        f"({len(table) * 8 / 1024:.1f} KB)",
        file=sys.stderr,
    )


if __name__ == "__main__":
    main()
