# Clavi iOS Keyboard Extension

Custom iOS keyboard (UIKit-based, no SwiftUI for compatibility with iOS 14+).

## Features (v0.5)

- Ukrainian ЙЦУКЕН + English QWERTY
- Translit mode (KMU 2010): type Latin → Ukrainian Cyrillic
- Language switch (UK ↔ EN)
- Clipboard history strip above keyboard

## Building

Requires Xcode 15+ and an Apple Developer account.

```bash
open ios/ClaviKeyboard/ClaviKeyboard.xcodeproj
# Set your Team in Signing & Capabilities
# Build & Run on device (simulator doesn't support keyboard extensions well)
```

## Testing on device

1. Build & install the app on your iPhone/iPad
2. Settings → General → Keyboard → Keyboards → Add New Keyboard
3. Find "Clavi" → tap it
4. In any text field: tap 🌐 to switch to Clavi

## Distribution

For beta testing: use **TestFlight**
1. Archive in Xcode (Product → Archive)
2. Upload to App Store Connect
3. Add testers in TestFlight

## Structure

```
ClaviKeyboardExtension/
├── KeyboardViewController.swift  # UIInputViewController, main IME logic
├── ClaviKeyboardView.swift       # Custom UIView keyboard rendering
├── KeyboardLayout.swift          # Layout definitions (UK/EN/symbols)
├── TranslitEngine.swift          # KMU 2010 transliteration
├── ClipboardHistory.swift        # Clipboard monitoring & history
└── Info.plist                    # Extension config (RequestsOpenAccess = true)
```
