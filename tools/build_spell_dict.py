#!/usr/bin/env python3
"""
build_spell_dict.py — compile word lists into spell/en.bin and spell/uk.bin

Binary format "SPLV" (before gzip):
    4 bytes  magic: "SPLV"
    u32 LE   word_count
    u32 LE   hash_table_size   (next power-of-2 >= ceil(word_count / 0.7))
    word list: word_count × [u8 len][UTF-8 bytes]   (sorted lexicographically → frequency proxy)
    hash table: hash_table_size × u64               (FNV-1a 64-bit, open-addressing, 0 = empty)

Hash function — FNV-1a 64-bit (same in Python, Kotlin, Swift; no external deps):
    h = 14695981039346656037
    for each byte b: h = ((h ^ b) * 1099511628211) & 0xFFFFFFFFFFFFFFFF
    sentinel: 0 is "empty slot", so 0 → 1

Outputs:
    android/app/src/main/assets/spell/en.bin
    android/app/src/main/assets/spell/uk.bin
    ios/ClaviKeyboard/ClaviKeyboardExtension/Resources/spell/en.bin
    ios/ClaviKeyboard/ClaviKeyboardExtension/Resources/spell/uk.bin
"""

import gzip
import math
import os
import re
import struct
import sys

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

ANDROID_OUTPUTS = [
    "android/app/src/main/assets/spell",
]
# iOS stores files uncompressed (no gzip) — App Store thinning handles compression
IOS_OUTPUTS = [
    "ios/ClaviKeyboard/ClaviKeyboardExtension/Resources/spell",
]

MAX_WORDS = 30_000
MIN_LEN = 2
MAX_LEN = 15


def fnv1a64(word: str) -> int:
    h = 14695981039346656037
    for b in word.encode("utf-8"):
        h = ((h ^ b) * 1099511628211) & 0xFFFFFFFFFFFFFFFF
    return h or 1  # 0 is reserved sentinel


def next_power_of_2(n: int) -> int:
    p = 1
    while p < n:
        p <<= 1
    return p


def build_hash_table(words: list[str]) -> list[int]:
    count = len(words)
    table_size = next_power_of_2(math.ceil(count / 0.7))
    table = [0] * table_size
    mask = table_size - 1
    for word in words:
        h = fnv1a64(word)
        idx = h & mask
        while table[idx] != 0:
            idx = (idx + 1) & mask
        table[idx] = h
    return table


def load_words_en(path: str) -> list[str]:
    """Load English words: lowercase a-z only, 2-15 chars, top 50K."""
    pat = re.compile(r"^[a-z]{2,15}$")
    words = []
    with open(path, encoding="utf-8", errors="ignore") as f:
        for line in f:
            w = line.strip().lower()
            if pat.match(w):
                words.append(w)
    # Deduplicate while preserving order
    seen = set()
    unique = []
    for w in words:
        if w not in seen:
            seen.add(w)
            unique.append(w)
    # Sort (short words first, then alpha) — position = frequency proxy
    unique.sort(key=lambda w: (len(w), w))
    return unique[:MAX_WORDS]


def load_words_uk(path: str) -> list[str]:
    """Load Ukrainian words: lowercase Cyrillic only, 2-15 chars, top 50K."""
    # Ukrainian lowercase Cyrillic: а-я plus і ї є ґ
    pat = re.compile(r"^[а-яіїєґ]{2,15}$")
    words = []
    with open(path, encoding="utf-8", errors="ignore") as f:
        for line in f:
            w = line.strip().lower()
            # Skip lines with special markers (e.g. "+cs=...")
            if w.startswith("+") or w.startswith("-"):
                continue
            if pat.match(w):
                words.append(w)
    seen = set()
    unique = []
    for w in words:
        if w not in seen:
            seen.add(w)
            unique.append(w)
    unique.sort(key=lambda w: (len(w), w))
    return unique[:MAX_WORDS]


def encode_word_list(words: list[str]) -> bytes:
    parts = []
    for w in words:
        encoded = w.encode("utf-8")
        parts.append(struct.pack("B", len(encoded)))
        parts.append(encoded)
    return b"".join(parts)


def build_binary(words: list[str]) -> bytes:
    table = build_hash_table(words)
    word_count = len(words)
    table_size = len(table)

    header = b"SPLV"
    header += struct.pack("<I", word_count)
    header += struct.pack("<I", table_size)

    word_bytes = encode_word_list(words)
    table_bytes = struct.pack(f"<{table_size}Q", *table)

    return header + word_bytes + table_bytes


def write_output(lang: str, data: bytes):
    for out_dir in ANDROID_OUTPUTS:
        full_dir = os.path.join(REPO_ROOT, out_dir)
        os.makedirs(full_dir, exist_ok=True)
        out_path = os.path.join(full_dir, f"{lang}.bin")
        with gzip.open(out_path, "wb", compresslevel=9) as f:
            f.write(data)
        size_kb = os.path.getsize(out_path) / 1024
        print(f"  wrote {out_path}  ({size_kb:.1f} KB gzip'd)")
    for out_dir in IOS_OUTPUTS:
        full_dir = os.path.join(REPO_ROOT, out_dir)
        os.makedirs(full_dir, exist_ok=True)
        out_path = os.path.join(full_dir, f"{lang}.bin")
        with open(out_path, "wb") as f:
            f.write(data)
        size_kb = os.path.getsize(out_path) / 1024
        print(f"  wrote {out_path}  ({size_kb:.1f} KB raw)")


def main():
    en_src = os.path.join(REPO_ROOT, "data/wordlists/en_words.txt")
    uk_src = os.path.join(REPO_ROOT, "data/wordlists/uk_words.txt")

    print("Loading English words...")
    en_words = load_words_en(en_src)
    print(f"  {len(en_words)} words selected")
    en_bin = build_binary(en_words)
    write_output("en", en_bin)

    print("Loading Ukrainian words...")
    uk_words = load_words_uk(uk_src)
    print(f"  {len(uk_words)} words selected")
    uk_bin = build_binary(uk_words)
    write_output("uk", uk_bin)

    print("Done.")


if __name__ == "__main__":
    main()
