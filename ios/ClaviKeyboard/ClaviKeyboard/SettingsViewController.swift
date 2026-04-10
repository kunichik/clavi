import UIKit

/**
 * Clavi Settings — main app UI.
 *
 * Writes shared preferences to UserDefaults(suiteName: "group.com.clavi.keyboard")
 * so the keyboard extension can read them.
 *
 * Sections:
 *   1. Setup   — link to iOS Settings to enable the keyboard extension
 *   2. Diacritics Language — off or one of: pt de no fr es sv fi pl
 *   3. Default Language    — Ukrainian (UK) or English (EN)
 */
final class SettingsViewController: UIViewController {

    // MARK: - Palette (matches Android #263238 / #4FC3F7)

    private enum Color {
        static let background  = UIColor(red: 0.149, green: 0.196, blue: 0.220, alpha: 1) // #263238
        static let surface     = UIColor(red: 0.184, green: 0.235, blue: 0.259, alpha: 1) // slightly lighter card
        static let accent      = UIColor(red: 0.310, green: 0.761, blue: 0.969, alpha: 1) // #4FC3F7
        static let text        = UIColor.white
        static let secondaryText = UIColor(white: 1, alpha: 0.6)
        static let separator   = UIColor(white: 1, alpha: 0.12)
    }

    // MARK: - Data

    private struct DiacriticsOption {
        let displayName: String
        let localeCode: String?   // nil = Off
    }

    private let diacriticsOptions: [DiacriticsOption] = [
        DiacriticsOption(displayName: "Off",             localeCode: nil),
        DiacriticsOption(displayName: "Portuguese (pt)", localeCode: "pt"),
        DiacriticsOption(displayName: "German (de)",     localeCode: "de"),
        DiacriticsOption(displayName: "Norwegian (no)",  localeCode: "no"),
        DiacriticsOption(displayName: "French (fr)",     localeCode: "fr"),
        DiacriticsOption(displayName: "Spanish (es)",    localeCode: "es"),
        DiacriticsOption(displayName: "Swedish (sv)",    localeCode: "sv"),
        DiacriticsOption(displayName: "Finnish (fi)",    localeCode: "fi"),
        DiacriticsOption(displayName: "Polish (pl)",     localeCode: "pl"),
    ]

    private var selectedDiacriticsIndex: Int = 0  // default: Off
    private var defaultLanguageIsUK: Bool = true  // default: Ukrainian

    private struct TranslationLang {
        let displayName: String
        let code: String?   // nil = Off
    }
    private let translationLangs: [TranslationLang] = [
        TranslationLang(displayName: "Off",               code: nil),
        TranslationLang(displayName: "English (en)",      code: "en"),
        TranslationLang(displayName: "Ukrainian (uk)",    code: "uk"),
        TranslationLang(displayName: "German (de)",       code: "de"),
        TranslationLang(displayName: "French (fr)",       code: "fr"),
        TranslationLang(displayName: "Spanish (es)",      code: "es"),
        TranslationLang(displayName: "Portuguese (pt)",   code: "pt"),
        TranslationLang(displayName: "Norwegian (no)",    code: "no"),
        TranslationLang(displayName: "Chinese (zh)",      code: "zh"),
        TranslationLang(displayName: "Japanese (ja)",     code: "ja"),
    ]
    private var selectedTranslSrcIndex: Int = 0   // default: Off
    private var selectedTranslTgtIndex: Int = 2   // default: Ukrainian

    private let defaults: UserDefaults = {
        UserDefaults(suiteName: "group.com.clavi.keyboard") ?? .standard
    }()

    // MARK: - Subviews

    private let scrollView  = UIScrollView()
    private let contentStack = UIStackView()

    // Section 1 — Setup
    private let setupCard       = UIView()
    private let setupBodyLabel  = UILabel()
    private let settingsButton  = UIButton(type: .system)

    // Section 2 — Diacritics Language
    private let diacrCard       = UIView()
    private let diacrPicker     = UIPickerView()

    // Section 3 — Default Language
    private let langCard        = UIView()
    private let langSegment     = UISegmentedControl(items: ["Ukrainian", "English"])

    // Section 4 — Translation
    private let translCard      = UIView()
    private let translSrcPicker = UIPickerView()
    private let translTgtPicker = UIPickerView()

    // MARK: - Lifecycle

    override func viewDidLoad() {
        super.viewDidLoad()
        title = "Clavi Settings"
        view.backgroundColor = Color.background
        navigationController?.navigationBar.barTintColor = Color.background
        navigationController?.navigationBar.titleTextAttributes = [.foregroundColor: Color.text]
        navigationController?.navigationBar.largeTitleTextAttributes = [.foregroundColor: Color.text]
        navigationController?.navigationBar.prefersLargeTitles = true
        navigationController?.navigationBar.tintColor = Color.accent

        loadPreferences()
        setupScrollView()
        setupSetupSection()
        setupDiacriticsSection()
        setupLanguageSection()
        setupTranslationSection()
        applyUI()
    }

    // MARK: - Preferences

    private func loadPreferences() {
        let savedLocale = defaults.string(forKey: "diacritics_locale")
        if let locale = savedLocale,
           let idx = diacriticsOptions.firstIndex(where: { $0.localeCode == locale }) {
            selectedDiacriticsIndex = idx
        } else {
            selectedDiacriticsIndex = 0  // Off
        }

        let savedLang = defaults.string(forKey: "default_language") ?? "UK"
        defaultLanguageIsUK = (savedLang == "UK")

        let savedSrc = defaults.string(forKey: "translation_source_lang")
        let savedTgt = defaults.string(forKey: "translation_target_lang")
        selectedTranslSrcIndex = translationLangs.firstIndex(where: { $0.code == savedSrc }) ?? 0
        selectedTranslTgtIndex = translationLangs.firstIndex(where: { $0.code == savedTgt }) ?? 2
    }

    private func savePreferences() {
        let option = diacriticsOptions[selectedDiacriticsIndex]
        if let locale = option.localeCode {
            defaults.set(locale, forKey: "diacritics_locale")
        } else {
            defaults.removeObject(forKey: "diacritics_locale")
        }

        defaults.set(defaultLanguageIsUK ? "UK" : "EN", forKey: "default_language")

        let srcLang = translationLangs[selectedTranslSrcIndex]
        let tgtLang = translationLangs[selectedTranslTgtIndex]
        if let src = srcLang.code { defaults.set(src, forKey: "translation_source_lang") }
        else { defaults.removeObject(forKey: "translation_source_lang") }
        if let tgt = tgtLang.code { defaults.set(tgt, forKey: "translation_target_lang") }
        else { defaults.removeObject(forKey: "translation_target_lang") }

        defaults.synchronize()
    }

    // MARK: - Layout

    private func setupScrollView() {
        scrollView.translatesAutoresizingMaskIntoConstraints = false
        scrollView.alwaysBounceVertical = true
        view.addSubview(scrollView)

        contentStack.axis = .vertical
        contentStack.spacing = 20
        contentStack.translatesAutoresizingMaskIntoConstraints = false
        scrollView.addSubview(contentStack)

        NSLayoutConstraint.activate([
            scrollView.leadingAnchor.constraint(equalTo: view.leadingAnchor),
            scrollView.trailingAnchor.constraint(equalTo: view.trailingAnchor),
            scrollView.topAnchor.constraint(equalTo: view.safeAreaLayoutGuide.topAnchor),
            scrollView.bottomAnchor.constraint(equalTo: view.bottomAnchor),

            contentStack.leadingAnchor.constraint(equalTo: scrollView.leadingAnchor, constant: 20),
            contentStack.trailingAnchor.constraint(equalTo: scrollView.trailingAnchor, constant: -20),
            contentStack.topAnchor.constraint(equalTo: scrollView.topAnchor, constant: 24),
            contentStack.bottomAnchor.constraint(equalTo: scrollView.bottomAnchor, constant: -40),
            contentStack.widthAnchor.constraint(equalTo: scrollView.widthAnchor, constant: -40),
        ])
    }

    private func setupSetupSection() {
        contentStack.addArrangedSubview(sectionHeader("SETUP"))
        contentStack.addArrangedSubview(setupCard)
        setupCard.backgroundColor = Color.surface
        setupCard.layer.cornerRadius = 12

        // Body label
        setupBodyLabel.numberOfLines = 0
        setupBodyLabel.font = .systemFont(ofSize: 15)
        setupBodyLabel.textColor = Color.secondaryText
        setupBodyLabel.text =
            "To use Clavi as your keyboard:\n\n" +
            "1. Tap the button below to open iOS Settings.\n" +
            "2. Go to General → Keyboard → Keyboards.\n" +
            "3. Tap \"Add New Keyboard…\" and select Clavi.\n" +
            "4. Enable \"Allow Full Access\" so the keyboard\n" +
            "   can read clipboard and shared settings."
        setupBodyLabel.translatesAutoresizingMaskIntoConstraints = false
        setupCard.addSubview(setupBodyLabel)

        // Button
        settingsButton.setTitle("Open iOS Settings", for: .normal)
        settingsButton.setTitleColor(Color.accent, for: .normal)
        settingsButton.titleLabel?.font = .systemFont(ofSize: 16, weight: .semibold)
        settingsButton.backgroundColor = Color.accent.withAlphaComponent(0.15)
        settingsButton.layer.cornerRadius = 10
        settingsButton.translatesAutoresizingMaskIntoConstraints = false
        settingsButton.addTarget(self, action: #selector(openIOSSettings), for: .touchUpInside)
        setupCard.addSubview(settingsButton)

        NSLayoutConstraint.activate([
            setupBodyLabel.leadingAnchor.constraint(equalTo: setupCard.leadingAnchor, constant: 16),
            setupBodyLabel.trailingAnchor.constraint(equalTo: setupCard.trailingAnchor, constant: -16),
            setupBodyLabel.topAnchor.constraint(equalTo: setupCard.topAnchor, constant: 16),

            settingsButton.leadingAnchor.constraint(equalTo: setupCard.leadingAnchor, constant: 16),
            settingsButton.trailingAnchor.constraint(equalTo: setupCard.trailingAnchor, constant: -16),
            settingsButton.topAnchor.constraint(equalTo: setupBodyLabel.bottomAnchor, constant: 16),
            settingsButton.heightAnchor.constraint(equalToConstant: 46),
            settingsButton.bottomAnchor.constraint(equalTo: setupCard.bottomAnchor, constant: -16),
        ])
    }

    private func setupDiacriticsSection() {
        contentStack.addArrangedSubview(sectionHeader("DIACRITICS LANGUAGE"))
        contentStack.addArrangedSubview(diacrCard)
        diacrCard.backgroundColor = Color.surface
        diacrCard.layer.cornerRadius = 12

        let hintLabel = UILabel()
        hintLabel.numberOfLines = 0
        hintLabel.font = .systemFont(ofSize: 13)
        hintLabel.textColor = Color.secondaryText
        hintLabel.text = "Shows accent variants above the keyboard (e.g. ã â á) so you can tap instead of long-pressing. Select \"Off\" to disable."
        hintLabel.translatesAutoresizingMaskIntoConstraints = false
        diacrCard.addSubview(hintLabel)

        diacrPicker.dataSource = self
        diacrPicker.delegate = self
        diacrPicker.translatesAutoresizingMaskIntoConstraints = false
        // Style the picker for dark background
        diacrPicker.setValue(Color.text, forKeyPath: "textColor")
        diacrCard.addSubview(diacrPicker)

        // Separator between hint and picker
        let sep = separatorView()
        diacrCard.addSubview(sep)

        NSLayoutConstraint.activate([
            hintLabel.leadingAnchor.constraint(equalTo: diacrCard.leadingAnchor, constant: 16),
            hintLabel.trailingAnchor.constraint(equalTo: diacrCard.trailingAnchor, constant: -16),
            hintLabel.topAnchor.constraint(equalTo: diacrCard.topAnchor, constant: 14),

            sep.leadingAnchor.constraint(equalTo: diacrCard.leadingAnchor, constant: 16),
            sep.trailingAnchor.constraint(equalTo: diacrCard.trailingAnchor),
            sep.topAnchor.constraint(equalTo: hintLabel.bottomAnchor, constant: 12),
            sep.heightAnchor.constraint(equalToConstant: 1),

            diacrPicker.leadingAnchor.constraint(equalTo: diacrCard.leadingAnchor),
            diacrPicker.trailingAnchor.constraint(equalTo: diacrCard.trailingAnchor),
            diacrPicker.topAnchor.constraint(equalTo: sep.bottomAnchor),
            diacrPicker.heightAnchor.constraint(equalToConstant: 160),
            diacrPicker.bottomAnchor.constraint(equalTo: diacrCard.bottomAnchor),
        ])
    }

    private func setupLanguageSection() {
        contentStack.addArrangedSubview(sectionHeader("DEFAULT LANGUAGE"))
        contentStack.addArrangedSubview(langCard)
        langCard.backgroundColor = Color.surface
        langCard.layer.cornerRadius = 12

        let hintLabel = UILabel()
        hintLabel.numberOfLines = 0
        hintLabel.font = .systemFont(ofSize: 13)
        hintLabel.textColor = Color.secondaryText
        hintLabel.text = "The layout the keyboard opens with each time."
        hintLabel.translatesAutoresizingMaskIntoConstraints = false
        langCard.addSubview(hintLabel)

        let sep = separatorView()
        langCard.addSubview(sep)

        // Style the segmented control for dark appearance
        langSegment.selectedSegmentIndex = defaultLanguageIsUK ? 0 : 1
        langSegment.backgroundColor = Color.background
        langSegment.selectedSegmentTintColor = Color.accent
        langSegment.setTitleTextAttributes([.foregroundColor: Color.text], for: .normal)
        langSegment.setTitleTextAttributes([.foregroundColor: UIColor.black], for: .selected)
        langSegment.translatesAutoresizingMaskIntoConstraints = false
        langSegment.addTarget(self, action: #selector(languageSegmentChanged), for: .valueChanged)
        langCard.addSubview(langSegment)

        NSLayoutConstraint.activate([
            hintLabel.leadingAnchor.constraint(equalTo: langCard.leadingAnchor, constant: 16),
            hintLabel.trailingAnchor.constraint(equalTo: langCard.trailingAnchor, constant: -16),
            hintLabel.topAnchor.constraint(equalTo: langCard.topAnchor, constant: 14),

            sep.leadingAnchor.constraint(equalTo: langCard.leadingAnchor, constant: 16),
            sep.trailingAnchor.constraint(equalTo: langCard.trailingAnchor),
            sep.topAnchor.constraint(equalTo: hintLabel.bottomAnchor, constant: 12),
            sep.heightAnchor.constraint(equalToConstant: 1),

            langSegment.leadingAnchor.constraint(equalTo: langCard.leadingAnchor, constant: 16),
            langSegment.trailingAnchor.constraint(equalTo: langCard.trailingAnchor, constant: -16),
            langSegment.topAnchor.constraint(equalTo: sep.bottomAnchor, constant: 16),
            langSegment.bottomAnchor.constraint(equalTo: langCard.bottomAnchor, constant: -16),
            langSegment.heightAnchor.constraint(equalToConstant: 44),
        ])
    }

    private func setupTranslationSection() {
        contentStack.addArrangedSubview(sectionHeader("TRANSLATION"))
        contentStack.addArrangedSubview(translCard)
        translCard.backgroundColor = Color.surface
        translCard.layer.cornerRadius = 12

        let hintLabel = UILabel()
        hintLabel.numberOfLines = 0
        hintLabel.font = .systemFont(ofSize: 13)
        hintLabel.textColor = Color.secondaryText
        hintLabel.text = "After each space, a blue strip shows the translation — tap to replace your text. " +
                         "First use downloads ~30MB model (WiFi only). Select \"Off\" to disable."
        hintLabel.translatesAutoresizingMaskIntoConstraints = false
        translCard.addSubview(hintLabel)

        let srcLabel = UILabel()
        srcLabel.text = "Source language"
        srcLabel.font = .systemFont(ofSize: 13, weight: .medium)
        srcLabel.textColor = Color.text
        srcLabel.translatesAutoresizingMaskIntoConstraints = false
        translCard.addSubview(srcLabel)

        translSrcPicker.dataSource = self
        translSrcPicker.delegate   = self
        translSrcPicker.tag = 10  // source picker tag
        translSrcPicker.translatesAutoresizingMaskIntoConstraints = false
        translSrcPicker.setValue(Color.text, forKeyPath: "textColor")
        translCard.addSubview(translSrcPicker)

        let sep1 = separatorView()
        translCard.addSubview(sep1)

        let tgtLabel = UILabel()
        tgtLabel.text = "Target language"
        tgtLabel.font = .systemFont(ofSize: 13, weight: .medium)
        tgtLabel.textColor = Color.text
        tgtLabel.translatesAutoresizingMaskIntoConstraints = false
        translCard.addSubview(tgtLabel)

        translTgtPicker.dataSource = self
        translTgtPicker.delegate   = self
        translTgtPicker.tag = 11  // target picker tag
        translTgtPicker.translatesAutoresizingMaskIntoConstraints = false
        translTgtPicker.setValue(Color.text, forKeyPath: "textColor")
        translCard.addSubview(translTgtPicker)

        NSLayoutConstraint.activate([
            hintLabel.leadingAnchor.constraint(equalTo: translCard.leadingAnchor, constant: 16),
            hintLabel.trailingAnchor.constraint(equalTo: translCard.trailingAnchor, constant: -16),
            hintLabel.topAnchor.constraint(equalTo: translCard.topAnchor, constant: 14),

            srcLabel.leadingAnchor.constraint(equalTo: translCard.leadingAnchor, constant: 16),
            srcLabel.topAnchor.constraint(equalTo: hintLabel.bottomAnchor, constant: 12),

            translSrcPicker.leadingAnchor.constraint(equalTo: translCard.leadingAnchor),
            translSrcPicker.trailingAnchor.constraint(equalTo: translCard.trailingAnchor),
            translSrcPicker.topAnchor.constraint(equalTo: srcLabel.bottomAnchor, constant: 4),
            translSrcPicker.heightAnchor.constraint(equalToConstant: 120),

            sep1.leadingAnchor.constraint(equalTo: translCard.leadingAnchor, constant: 16),
            sep1.trailingAnchor.constraint(equalTo: translCard.trailingAnchor),
            sep1.topAnchor.constraint(equalTo: translSrcPicker.bottomAnchor),
            sep1.heightAnchor.constraint(equalToConstant: 1),

            tgtLabel.leadingAnchor.constraint(equalTo: translCard.leadingAnchor, constant: 16),
            tgtLabel.topAnchor.constraint(equalTo: sep1.bottomAnchor, constant: 12),

            translTgtPicker.leadingAnchor.constraint(equalTo: translCard.leadingAnchor),
            translTgtPicker.trailingAnchor.constraint(equalTo: translCard.trailingAnchor),
            translTgtPicker.topAnchor.constraint(equalTo: tgtLabel.bottomAnchor, constant: 4),
            translTgtPicker.heightAnchor.constraint(equalToConstant: 120),
            translTgtPicker.bottomAnchor.constraint(equalTo: translCard.bottomAnchor, constant: -8),
        ])
    }

    // MARK: - Apply loaded values to UI

    private func applyUI() {
        diacrPicker.selectRow(selectedDiacriticsIndex, inComponent: 0, animated: false)
        langSegment.selectedSegmentIndex = defaultLanguageIsUK ? 0 : 1
        translSrcPicker.selectRow(selectedTranslSrcIndex, inComponent: 0, animated: false)
        translTgtPicker.selectRow(selectedTranslTgtIndex, inComponent: 0, animated: false)
    }

    // MARK: - Helpers

    private func sectionHeader(_ text: String) -> UILabel {
        let label = UILabel()
        label.text = text
        label.font = .systemFont(ofSize: 12, weight: .semibold)
        label.textColor = Color.accent
        label.letterSpacing(1.2)
        return label
    }

    private func separatorView() -> UIView {
        let v = UIView()
        v.backgroundColor = Color.separator
        v.translatesAutoresizingMaskIntoConstraints = false
        return v
    }

    // MARK: - Actions

    @objc private func openIOSSettings() {
        guard let url = URL(string: "app-settings:") else { return }
        if UIApplication.shared.canOpenURL(url) {
            UIApplication.shared.open(url)
        }
    }

    @objc private func languageSegmentChanged() {
        defaultLanguageIsUK = (langSegment.selectedSegmentIndex == 0)
        savePreferences()
    }
}

// MARK: - UIPickerViewDataSource

extension SettingsViewController: UIPickerViewDataSource {

    func numberOfComponents(in pickerView: UIPickerView) -> Int { 1 }

    func pickerView(_ pickerView: UIPickerView, numberOfRowsInComponent component: Int) -> Int {
        switch pickerView.tag {
        case 10, 11: return translationLangs.count
        default:     return diacriticsOptions.count
        }
    }
}

// MARK: - UIPickerViewDelegate

extension SettingsViewController: UIPickerViewDelegate {

    func pickerView(_ pickerView: UIPickerView,
                    attributedTitleForRow row: Int,
                    forComponent component: Int) -> NSAttributedString? {
        let title: String
        switch pickerView.tag {
        case 10, 11: title = translationLangs[row].displayName
        default:     title = diacriticsOptions[row].displayName
        }
        return NSAttributedString(string: title, attributes: [.foregroundColor: Color.text])
    }

    func pickerView(_ pickerView: UIPickerView,
                    didSelectRow row: Int,
                    inComponent component: Int) {
        switch pickerView.tag {
        case 10: selectedTranslSrcIndex = row
        case 11: selectedTranslTgtIndex = row
        default: selectedDiacriticsIndex = row
        }
        savePreferences()
    }
}

// MARK: - UILabel helper

private extension UILabel {
    /// Applies letter spacing via attributed text.
    func letterSpacing(_ spacing: CGFloat) {
        guard let t = text else { return }
        let attrs: [NSAttributedString.Key: Any] = [
            .kern: spacing,
            .foregroundColor: textColor as Any,
            .font: font as Any,
        ]
        attributedText = NSAttributedString(string: t, attributes: attrs)
    }
}
