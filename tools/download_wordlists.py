#!/usr/bin/env python3
"""
download_wordlists.py — fetch open-source word lists for Ukrainian and English

Ukrainian: hunspell uk_UA dictionary (from LibreOffice / Mozilla)
English:   SCOWL (Spell Checker Oriented Word Lists) via github.com/en-wl/wordlist

Usage:
    python3 tools/download_wordlists.py
"""

import os
import sys
import urllib.request
import zipfile
import io
import pathlib

ROOT = pathlib.Path(__file__).parent.parent
DATA_DIR = ROOT / "data" / "wordlists"
DATA_DIR.mkdir(parents=True, exist_ok=True)

UK_AFF_URL = (
    "https://github.com/LibreOffice/dictionaries/raw/master/uk_UA/uk_UA.aff"
)
UK_DIC_URL = (
    "https://github.com/LibreOffice/dictionaries/raw/master/uk_UA/uk_UA.dic"
)

EN_ZIP_URL = (
    "https://github.com/en-wl/wordlist/releases/latest/download/scowl.zip"
)
# Fallback: direct SCOWL word list from a known release
EN_WORDS_URL = (
    "https://raw.githubusercontent.com/dwyl/english-words/master/words_alpha.txt"
)


def fetch(url: str, desc: str) -> bytes:
    print(f"Fetching {desc}...", file=sys.stderr)
    req = urllib.request.Request(
        url, headers={"User-Agent": "clavi/0.1 build-tool"}
    )
    with urllib.request.urlopen(req, timeout=60) as resp:
        data = resp.read()
    print(f"  {len(data) // 1024} KB downloaded", file=sys.stderr)
    return data


def parse_hunspell_dic(dic_bytes: bytes) -> list[str]:
    """Parse .dic file: first line is word count, rest are word/affix pairs."""
    words: list[str] = []
    for i, line in enumerate(dic_bytes.decode("utf-8", errors="ignore").splitlines()):
        if i == 0:
            continue  # word count header
        word = line.split("/")[0].strip()
        if word and not word.startswith("#"):
            words.append(word)
    return words


def download_ukrainian() -> None:
    out_path = DATA_DIR / "uk_words.txt"
    if out_path.exists():
        print(f"Ukrainian word list already exists: {out_path}", file=sys.stderr)
        return

    try:
        dic_data = fetch(UK_DIC_URL, "uk_UA.dic (hunspell)")
        words = parse_hunspell_dic(dic_data)
        print(f"Parsed {len(words)} Ukrainian words", file=sys.stderr)
        out_path.write_text("\n".join(words), encoding="utf-8")
        print(f"Written: {out_path}", file=sys.stderr)
    except Exception as e:
        print(f"ERROR downloading Ukrainian word list: {e}", file=sys.stderr)
        sys.exit(1)


def download_english() -> None:
    out_path = DATA_DIR / "en_words.txt"
    if out_path.exists():
        print(f"English word list already exists: {out_path}", file=sys.stderr)
        return

    try:
        # Use dwyl/english-words as a simple fallback (~370k words, alpha only)
        data = fetch(EN_WORDS_URL, "english-words (dwyl/english-words)")
        words = [
            w.strip()
            for w in data.decode("utf-8", errors="ignore").splitlines()
            if w.strip() and w.strip().isalpha()
        ]
        print(f"Parsed {len(words)} English words", file=sys.stderr)
        out_path.write_text("\n".join(words), encoding="utf-8")
        print(f"Written: {out_path}", file=sys.stderr)
    except Exception as e:
        print(f"ERROR downloading English word list: {e}", file=sys.stderr)
        sys.exit(1)


def main() -> None:
    print("=== Downloading word lists ===", file=sys.stderr)
    download_ukrainian()
    download_english()
    print("\nDone. Run build_dict.py to compile .bin files.", file=sys.stderr)
    print("\nExample commands:", file=sys.stderr)
    print(
        "  python3 tools/build_dict.py data/wordlists/uk_words.txt packs/uk/dictionary.bin",
        file=sys.stderr,
    )
    print(
        "  python3 tools/build_dict.py data/wordlists/en_words.txt packs/en/dictionary.bin",
        file=sys.stderr,
    )
    print(
        "  python3 tools/build_keymap.py data/keyboard_maps/en_qwerty.json "
        "data/keyboard_maps/uk_qwerty.json packs/uk/keyboard_map.bin",
        file=sys.stderr,
    )
    print(
        "  python3 tools/build_keymap.py data/keyboard_maps/uk_qwerty.json "
        "data/keyboard_maps/en_qwerty.json packs/en/keyboard_map.bin",
        file=sys.stderr,
    )
    print(
        "  python3 tools/build_ngram.py data/wordlists/uk_words.txt packs/uk/ngram.bin --ngram-size 3",
        file=sys.stderr,
    )


if __name__ == "__main__":
    main()
