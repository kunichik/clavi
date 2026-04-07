#!/usr/bin/env python3
"""
build_ngram.py — compile a word list into an n-gram language model (ngram.bin)

Usage:
    python3 tools/build_ngram.py <wordlist.txt> <output.bin> [--ngram-size N]

    N defaults to 3 (trigrams). Can also produce 4 or 5-grams.

Format (ngram.bin):
    4-byte magic: NGRM
    4-byte LE u32: ngram count
    1-byte: ngram size (3, 4, or 5)
    Per entry (sorted alphabetically by n-gram bytes):
        [N bytes UTF-8 n-gram] [4-byte LE float: log-probability]
"""

import sys
import math
import struct
import collections
import argparse


def extract_byte_ngrams(word: str, n: int) -> list[bytes]:
    """Extract all byte-level n-grams of exactly n bytes from UTF-8 encoded word."""
    encoded = word.encode("utf-8")
    return [encoded[i : i + n] for i in range(len(encoded) - n + 1)]


def build_ngram_model(
    words: list[str], n: int, min_count: int = 2
) -> dict[bytes, float]:
    """Build byte-level n-gram model. Each n-gram is exactly n bytes."""
    counts: dict[bytes, int] = collections.Counter()
    for word in words:
        for gram in extract_byte_ngrams(word.lower(), n):
            counts[gram] += 1

    # Filter low-frequency n-grams
    counts = {g: c for g, c in counts.items() if c >= min_count}

    total = sum(counts.values())
    if total == 0:
        return {}

    log_probs: dict[bytes, float] = {}
    for gram, count in counts.items():
        log_probs[gram] = math.log(count / total)

    return log_probs


def write_bin(model: dict[bytes, float], ngram_size: int, output_path: str) -> None:
    # Sort by raw bytes (matches C++ std::string lexicographic comparison)
    entries = sorted(model.items(), key=lambda x: x[0])

    with open(output_path, "wb") as f:
        f.write(b"NGRM")
        f.write(struct.pack("<I", len(entries)))
        f.write(struct.pack("B", ngram_size))

        for gram_bytes, log_prob in entries:
            assert len(gram_bytes) == ngram_size, (
                f"n-gram byte length {len(gram_bytes)} != {ngram_size}"
            )
            f.write(gram_bytes)
            f.write(struct.pack("<f", log_prob))


def main() -> None:
    parser = argparse.ArgumentParser(description="Build n-gram binary model")
    parser.add_argument("wordlist", help="Input word list (.txt)")
    parser.add_argument("output", help="Output binary (.bin)")
    parser.add_argument(
        "--ngram-size", type=int, default=3, choices=[3, 4, 5],
        help="N-gram character size (default: 3)"
    )
    parser.add_argument(
        "--min-count", type=int, default=2,
        help="Minimum n-gram frequency to include (default: 2)"
    )
    args = parser.parse_args()

    words: list[str] = []
    with open(args.wordlist, "r", encoding="utf-8") as f:
        for line in f:
            word = line.strip()
            if word and not word.startswith("#"):
                words.append(word)

    print(f"Loaded {len(words)} words from {args.wordlist}", file=sys.stderr)

    model = build_ngram_model(words, args.ngram_size, args.min_count)
    print(
        f"Built {args.ngram_size}-gram model: {len(model)} unique grams",
        file=sys.stderr,
    )

    write_bin(model, args.ngram_size, args.output)

    import os
    size_kb = os.path.getsize(args.output) / 1024
    print(f"Written {args.output}: {size_kb:.1f} KB", file=sys.stderr)


if __name__ == "__main__":
    main()
