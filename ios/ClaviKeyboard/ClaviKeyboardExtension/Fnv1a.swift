// Fnv1a.swift — FNV-1a 64-bit hash (no external dependencies)
// Same algorithm used in build_spell_dict.py and SpellEngine.kt

import Foundation

enum Fnv1a {
    static func hash64(_ s: String) -> UInt64 {
        var h: UInt64 = 14695981039346656037
        for byte in s.utf8 {
            h ^= UInt64(byte)
            h &*= 1099511628211
        }
        return h == 0 ? 1 : h  // 0 is sentinel for empty hash slot
    }
}
