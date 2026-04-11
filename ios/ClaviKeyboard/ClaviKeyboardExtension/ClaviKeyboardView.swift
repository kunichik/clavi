import UIKit

protocol ClaviKeyboardViewDelegate: AnyObject {
    func didTapKey(_ key: KeyDef)
    func didTapClip(at index: Int)
    func didLongPressClip(at index: Int)
    func didTapClearClips()
    func didTapDiacritic(_ variant: String)
    func didTapFix(_ fix: TextFixEngine.Fix)
    func didDismissFix()
    func didTapTranslation(_ suggestion: TranslationEngine.TranslationSuggestion)
    func didDismissTranslation()
    func didTapPrediction(_ word: String)
    func didTapNextKeyboard()
}

/**
 * Custom keyboard view (UIKit).
 *
 * Strip priority (only one shown at a time):
 *   1. Fix strip (green)      — TextFixEngine suggestion
 *   2. Diacritics strip (teal)— last letter has variants
 *   3. Clipboard strip (dark) — paste history
 */
class ClaviKeyboardView: UIView {

    weak var delegate: ClaviKeyboardViewDelegate?

    var rows: [[KeyDef]] = [] { didSet { rebuildKeys() } }
    var translitActive = false { didSet { updateTranslitButton() } }
    var currentLanguage: Language = .uk { didSet { updateLangButton() } }

    // Strip data — setting any of these triggers priority re-evaluation
    var clipItems: [String] = []        { didSet { updateStripVisibility() } }
    var diacriticItems: [String] = []   { didSet { updateStripVisibility() } }
    var fixSuggestion: TextFixEngine.Fix? { didSet { updateStripVisibility() } }
    var translationSuggestion: TranslationEngine.TranslationSuggestion? { didSet { updateStripVisibility() } }
    var predictionItems: [String] = [] { didSet { updateStripVisibility() } }

    // MARK: - Subviews

    // Clipboard strip
    private let clipScrollView  = UIScrollView()
    private let clipStack       = UIStackView()
    private let clipClearBtn    = UIButton(type: .system)

    // Diacritics strip
    private let diacrStrip      = UIView()
    private let diacrStack      = UIStackView()

    // Fix strip
    private let fixStrip        = UIView()
    private let fixStack        = UIStackView()

    // Translation strip
    private let translStrip     = UIView()
    private let translStack     = UIStackView()

    // Predictions strip
    private let predStrip       = UIView()
    private let predStack       = UIStackView()

    private let keysContainer   = UIView()
    private var keyButtons: [UIButton: KeyDef] = [:]
    private var translitButton: UIButton?
    private var langButton: UIButton?

    // MARK: - Constants

    private let stripHeight: CGFloat = 44
    private let keySpacing: CGFloat  = 6
    private let rowSpacing: CGFloat  = 6
    private let keyCorner: CGFloat   = 8

    // Colors
    private let bgColor        = UIColor(red: 0.15, green: 0.19, blue: 0.20, alpha: 1)
    private let keyColor       = UIColor(red: 0.22, green: 0.28, blue: 0.31, alpha: 1)
    private let specialColor   = UIColor(red: 0.27, green: 0.35, blue: 0.40, alpha: 1)
    private let textColor      = UIColor.white
    private let translitOnColor = UIColor(red: 0.31, green: 0.76, blue: 0.97, alpha: 1)
    private let langColor      = UIColor(red: 1.0,  green: 0.67, blue: 0.25, alpha: 1)
    private let clipBgColor    = UIColor(red: 0.11, green: 0.17, blue: 0.19, alpha: 1)
    private let chipColor      = UIColor(red: 0.22, green: 0.28, blue: 0.31, alpha: 1)
    private let diacrBgColor   = UIColor(red: 0.04, green: 0.26, blue: 0.26, alpha: 1)
    private let fixBgColor     = UIColor(red: 0.09, green: 0.19, blue: 0.14, alpha: 1)
    private let translBgColor  = UIColor(red: 0.05, green: 0.28, blue: 0.63, alpha: 1)
    private let predBgColor    = UIColor(red: 0.11, green: 0.14, blue: 0.16, alpha: 1)

    // MARK: - Init

    override init(frame: CGRect) {
        super.init(frame: frame)
        setup()
    }

    required init?(coder: NSCoder) {
        super.init(coder: coder)
        setup()
    }

    private func setup() {
        backgroundColor = bgColor

        setupClipStrip()
        setupDiacrStrip()
        setupFixStrip()
        setupTranslStrip()
        setupPredStrip()

        keysContainer.translatesAutoresizingMaskIntoConstraints = false
        addSubview(keysContainer)

        NSLayoutConstraint.activate([
            // All three strips occupy the same top slot — visibility switches between them
            clipScrollView.leadingAnchor.constraint(equalTo: leadingAnchor),
            clipScrollView.trailingAnchor.constraint(equalTo: trailingAnchor, constant: -44),
            clipScrollView.topAnchor.constraint(equalTo: topAnchor),
            clipScrollView.heightAnchor.constraint(equalToConstant: stripHeight),

            clipClearBtn.trailingAnchor.constraint(equalTo: trailingAnchor, constant: -4),
            clipClearBtn.centerYAnchor.constraint(equalTo: clipScrollView.centerYAnchor),
            clipClearBtn.widthAnchor.constraint(equalToConstant: 40),

            clipStack.leadingAnchor.constraint(equalTo: clipScrollView.leadingAnchor, constant: 8),
            clipStack.trailingAnchor.constraint(equalTo: clipScrollView.trailingAnchor, constant: -8),
            clipStack.topAnchor.constraint(equalTo: clipScrollView.topAnchor, constant: 4),
            clipStack.bottomAnchor.constraint(equalTo: clipScrollView.bottomAnchor, constant: -4),

            diacrStrip.leadingAnchor.constraint(equalTo: leadingAnchor),
            diacrStrip.trailingAnchor.constraint(equalTo: trailingAnchor),
            diacrStrip.topAnchor.constraint(equalTo: topAnchor),
            diacrStrip.heightAnchor.constraint(equalToConstant: stripHeight),

            diacrStack.leadingAnchor.constraint(equalTo: diacrStrip.leadingAnchor, constant: 8),
            diacrStack.trailingAnchor.constraint(equalTo: diacrStrip.trailingAnchor, constant: -8),
            diacrStack.centerYAnchor.constraint(equalTo: diacrStrip.centerYAnchor),

            fixStrip.leadingAnchor.constraint(equalTo: leadingAnchor),
            fixStrip.trailingAnchor.constraint(equalTo: trailingAnchor),
            fixStrip.topAnchor.constraint(equalTo: topAnchor),
            fixStrip.heightAnchor.constraint(equalToConstant: stripHeight),

            fixStack.leadingAnchor.constraint(equalTo: fixStrip.leadingAnchor, constant: 8),
            fixStack.trailingAnchor.constraint(equalTo: fixStrip.trailingAnchor, constant: -8),
            fixStack.centerYAnchor.constraint(equalTo: fixStrip.centerYAnchor),

            translStrip.leadingAnchor.constraint(equalTo: leadingAnchor),
            translStrip.trailingAnchor.constraint(equalTo: trailingAnchor),
            translStrip.topAnchor.constraint(equalTo: topAnchor),
            translStrip.heightAnchor.constraint(equalToConstant: stripHeight),

            translStack.leadingAnchor.constraint(equalTo: translStrip.leadingAnchor, constant: 8),
            translStack.trailingAnchor.constraint(equalTo: translStrip.trailingAnchor, constant: -8),
            translStack.centerYAnchor.constraint(equalTo: translStrip.centerYAnchor),

            predStrip.leadingAnchor.constraint(equalTo: leadingAnchor),
            predStrip.trailingAnchor.constraint(equalTo: trailingAnchor),
            predStrip.topAnchor.constraint(equalTo: topAnchor),
            predStrip.heightAnchor.constraint(equalToConstant: stripHeight),

            predStack.leadingAnchor.constraint(equalTo: predStrip.leadingAnchor),
            predStack.trailingAnchor.constraint(equalTo: predStrip.trailingAnchor),
            predStack.topAnchor.constraint(equalTo: predStrip.topAnchor),
            predStack.bottomAnchor.constraint(equalTo: predStrip.bottomAnchor),

            keysContainer.leadingAnchor.constraint(equalTo: leadingAnchor),
            keysContainer.trailingAnchor.constraint(equalTo: trailingAnchor),
            keysContainer.topAnchor.constraint(equalTo: topAnchor, constant: stripHeight),
            keysContainer.bottomAnchor.constraint(equalTo: bottomAnchor),
        ])

        updateStripVisibility()
    }

    // MARK: - Clipboard strip setup

    private func setupClipStrip() {
        clipScrollView.backgroundColor = clipBgColor
        clipScrollView.showsHorizontalScrollIndicator = false
        clipScrollView.translatesAutoresizingMaskIntoConstraints = false
        addSubview(clipScrollView)

        clipStack.axis = .horizontal
        clipStack.spacing = 6
        clipStack.alignment = .center
        clipStack.translatesAutoresizingMaskIntoConstraints = false
        clipScrollView.addSubview(clipStack)

        clipClearBtn.setTitle("×", for: .normal)
        clipClearBtn.setTitleColor(.red, for: .normal)
        clipClearBtn.titleLabel?.font = .systemFont(ofSize: 18, weight: .bold)
        clipClearBtn.translatesAutoresizingMaskIntoConstraints = false
        clipClearBtn.addTarget(self, action: #selector(clearClips), for: .touchUpInside)
        addSubview(clipClearBtn)
    }

    // MARK: - Diacritics strip setup

    private func setupDiacrStrip() {
        diacrStrip.backgroundColor = diacrBgColor
        diacrStrip.translatesAutoresizingMaskIntoConstraints = false
        addSubview(diacrStrip)

        diacrStack.axis = .horizontal
        diacrStack.spacing = 8
        diacrStack.alignment = .center
        diacrStack.translatesAutoresizingMaskIntoConstraints = false
        diacrStrip.addSubview(diacrStack)
    }

    // MARK: - Fix strip setup

    private func setupFixStrip() {
        fixStrip.backgroundColor = fixBgColor
        fixStrip.translatesAutoresizingMaskIntoConstraints = false
        addSubview(fixStrip)

        fixStack.axis = .horizontal
        fixStack.spacing = 8
        fixStack.alignment = .center
        fixStack.translatesAutoresizingMaskIntoConstraints = false
        fixStrip.addSubview(fixStack)
    }

    // MARK: - Predictions strip setup

    private func setupPredStrip() {
        predStrip.backgroundColor = predBgColor
        predStrip.translatesAutoresizingMaskIntoConstraints = false
        addSubview(predStrip)

        // Equal-width distribution — 3 word buttons spanning full width, iOS-style
        predStack.axis = .horizontal
        predStack.distribution = .fillEqually
        predStack.spacing = 0
        predStack.translatesAutoresizingMaskIntoConstraints = false
        predStrip.addSubview(predStack)
    }

    // MARK: - Translation strip setup

    private func setupTranslStrip() {
        translStrip.backgroundColor = translBgColor
        translStrip.translatesAutoresizingMaskIntoConstraints = false
        addSubview(translStrip)

        translStack.axis = .horizontal
        translStack.spacing = 8
        translStack.alignment = .center
        translStack.translatesAutoresizingMaskIntoConstraints = false
        translStrip.addSubview(translStack)
    }

    // MARK: - Strip visibility (priority: fix > translation > diacritics > predictions > clipboard)

    private func updateStripVisibility() {
        let showFix   = fixSuggestion != nil
        let showTransl = !showFix && translationSuggestion != nil
        let showDiacr  = !showFix && !showTransl && !diacriticItems.isEmpty
        let showPred   = !showFix && !showTransl && !showDiacr && !predictionItems.isEmpty
        let showClip   = !showFix && !showTransl && !showDiacr && !showPred && !clipItems.isEmpty

        fixStrip.isHidden       = !showFix
        translStrip.isHidden    = !showTransl
        diacrStrip.isHidden     = !showDiacr
        predStrip.isHidden      = !showPred
        clipScrollView.isHidden = !showClip
        clipClearBtn.isHidden   = !showClip

        if showFix    { rebuildFixStrip() }
        if showTransl { rebuildTranslStrip() }
        if showDiacr  { rebuildDiacrStrip() }
        if showPred   { rebuildPredStrip() }
        if showClip   { rebuildClipStrip() }
    }

    // MARK: - Rebuild strip content

    private func rebuildClipStrip() {
        clipStack.arrangedSubviews.forEach { $0.removeFromSuperview() }
        for (i, label) in clipItems.enumerated() {
            let btn = chipButton(title: label, color: UIColor(red: 0.88, green: 0.97, blue: 0.98, alpha: 1), bg: chipColor)
            btn.tag = i
            btn.addTarget(self, action: #selector(clipChipTapped(_:)), for: .touchUpInside)
            let lp = UILongPressGestureRecognizer(target: self, action: #selector(clipChipLongPressed(_:)))
            btn.addGestureRecognizer(lp)
            clipStack.addArrangedSubview(btn)
        }
    }

    private func rebuildDiacrStrip() {
        diacrStack.arrangedSubviews.forEach { $0.removeFromSuperview() }

        let label = UILabel()
        label.text = "díacr:"
        label.font = .systemFont(ofSize: 11)
        label.textColor = UIColor(white: 1, alpha: 0.5)
        diacrStack.addArrangedSubview(label)

        for (i, variant) in diacriticItems.enumerated() {
            let isFirst = (i == 0)
            let isLast  = (i == diacriticItems.count - 1)
            let bg: UIColor = isFirst
                ? UIColor(red: 0.0, green: 0.6, blue: 0.6, alpha: 0.6)
                : (isLast ? chipColor.withAlphaComponent(0.5) : chipColor)
            let btn = chipButton(title: variant, color: .white, bg: bg)
            btn.titleLabel?.font = .systemFont(ofSize: 18, weight: isFirst ? .semibold : .regular)
            btn.tag = i
            btn.addTarget(self, action: #selector(diacrChipTapped(_:)), for: .touchUpInside)
            diacrStack.addArrangedSubview(btn)
        }
    }

    private func rebuildFixStrip() {
        fixStack.arrangedSubviews.forEach { $0.removeFromSuperview() }
        guard let fix = fixSuggestion else { return }

        let label = UILabel()
        label.text = "✦ fix:"
        label.font = .systemFont(ofSize: 11)
        label.textColor = UIColor(white: 1, alpha: 0.6)
        fixStack.addArrangedSubview(label)

        // Preview chip — shows the fixed text (truncated)
        let preview = String(fix.fixed.trimmingCharacters(in: .whitespaces).suffix(40))
        let previewBtn = chipButton(title: preview, color: UIColor(red: 0.6, green: 1.0, blue: 0.7, alpha: 1),
                                    bg: UIColor(red: 0.0, green: 0.4, blue: 0.2, alpha: 0.7))
        previewBtn.addTarget(self, action: #selector(fixApplyTapped), for: .touchUpInside)
        fixStack.addArrangedSubview(previewBtn)

        // Reason hint
        let hint = UILabel()
        hint.text = fix.description
        hint.font = .systemFont(ofSize: 10)
        hint.textColor = UIColor(white: 1, alpha: 0.4)
        fixStack.addArrangedSubview(hint)

        // Spacer
        let spacer = UIView()
        spacer.setContentHuggingPriority(.defaultLow, for: .horizontal)
        fixStack.addArrangedSubview(spacer)

        // Dismiss button
        let dismissBtn = chipButton(title: "✕", color: UIColor(white: 1, alpha: 0.6), bg: chipColor.withAlphaComponent(0.5))
        dismissBtn.addTarget(self, action: #selector(fixDismissTapped), for: .touchUpInside)
        fixStack.addArrangedSubview(dismissBtn)
    }

    private func rebuildPredStrip() {
        predStack.arrangedSubviews.forEach { $0.removeFromSuperview() }

        for (i, word) in predictionItems.prefix(3).enumerated() {
            let btn = UIButton(type: .custom)
            btn.setTitle(word, for: .normal)
            btn.setTitleColor(.white, for: .normal)
            btn.titleLabel?.font = .systemFont(ofSize: 15)
            btn.backgroundColor = .clear
            btn.tag = i
            btn.addTarget(self, action: #selector(predChipTapped(_:)), for: .touchUpInside)
            predStack.addArrangedSubview(btn)

            // Divider between chips (not after last)
            if i < predictionItems.prefix(3).count - 1 {
                let div = UIView()
                div.backgroundColor = UIColor(white: 1, alpha: 0.15)
                div.translatesAutoresizingMaskIntoConstraints = false
                div.widthAnchor.constraint(equalToConstant: 1).isActive = true
                predStack.addArrangedSubview(div)
            }
        }
    }

    private func rebuildTranslStrip() {
        translStack.arrangedSubviews.forEach { $0.removeFromSuperview() }
        guard let suggestion = translationSuggestion else { return }

        // "🌐 en→uk:" label
        let label = UILabel()
        label.text = "\u{1F310} \(suggestion.sourceLang)→\(suggestion.targetLang):"
        label.font = .systemFont(ofSize: 11)
        label.textColor = UIColor(white: 1, alpha: 0.55)
        translStack.addArrangedSubview(label)

        // Translation chip (main tap target)
        let preview = String(suggestion.translated.prefix(35)) +
                      (suggestion.translated.count > 35 ? "…" : "")
        let transBtnBg = UIColor(red: 0.10, green: 0.46, blue: 0.82, alpha: 0.85)
        let transBtn = chipButton(title: preview,
                                  color: UIColor(red: 0.7, green: 0.9, blue: 1.0, alpha: 1),
                                  bg: transBtnBg)
        transBtn.addTarget(self, action: #selector(translationApplyTapped), for: .touchUpInside)
        translStack.addArrangedSubview(transBtn)

        // Original hint
        let origLabel = UILabel()
        let origPreview = String(suggestion.original.prefix(25)) +
                          (suggestion.original.count > 25 ? "…" : "")
        origLabel.text = origPreview
        origLabel.font = .systemFont(ofSize: 10)
        origLabel.textColor = UIColor(white: 1, alpha: 0.35)
        translStack.addArrangedSubview(origLabel)

        // Spacer
        let spacer = UIView()
        spacer.setContentHuggingPriority(.defaultLow, for: .horizontal)
        translStack.addArrangedSubview(spacer)

        // Dismiss button
        let dismissBtn = chipButton(title: "✕", color: UIColor(white: 1, alpha: 0.6),
                                    bg: chipColor.withAlphaComponent(0.5))
        dismissBtn.addTarget(self, action: #selector(translationDismissTapped), for: .touchUpInside)
        translStack.addArrangedSubview(dismissBtn)
    }

    // MARK: - Helper

    private func chipButton(title: String, color: UIColor, bg: UIColor) -> UIButton {
        let btn = UIButton(type: .custom)
        btn.setTitle(title, for: .normal)
        btn.setTitleColor(color, for: .normal)
        btn.titleLabel?.font = .systemFont(ofSize: 13)
        btn.backgroundColor = bg
        btn.layer.cornerRadius = 6
        btn.contentEdgeInsets = UIEdgeInsets(top: 4, left: 10, bottom: 4, right: 10)
        return btn
    }

    // MARK: - Strip actions

    @objc private func clipChipTapped(_ sender: UIButton) {
        delegate?.didTapClip(at: sender.tag)
    }

    @objc private func clipChipLongPressed(_ gr: UILongPressGestureRecognizer) {
        guard gr.state == .began, let btn = gr.view as? UIButton else { return }
        delegate?.didLongPressClip(at: btn.tag)
    }

    @objc private func clearClips() {
        delegate?.didTapClearClips()
    }

    @objc private func diacrChipTapped(_ sender: UIButton) {
        guard sender.tag < diacriticItems.count else { return }
        delegate?.didTapDiacritic(diacriticItems[sender.tag])
    }

    @objc private func fixApplyTapped() {
        guard let fix = fixSuggestion else { return }
        delegate?.didTapFix(fix)
    }

    @objc private func fixDismissTapped() {
        delegate?.didDismissFix()
    }

    @objc private func predChipTapped(_ sender: UIButton) {
        guard sender.tag < predictionItems.count else { return }
        delegate?.didTapPrediction(predictionItems[sender.tag])
    }

    @objc private func translationApplyTapped() {
        guard let suggestion = translationSuggestion else { return }
        delegate?.didTapTranslation(suggestion)
    }

    @objc private func translationDismissTapped() {
        delegate?.didDismissTranslation()
    }

    // MARK: - Keys

    private func rebuildKeys() {
        keysContainer.subviews.forEach { $0.removeFromSuperview() }
        keyButtons.removeAll()
        translitButton = nil
        langButton = nil

        for (rowIdx, row) in rows.enumerated() {
            for key in row {
                let btn = UIButton(type: .system)
                btn.setTitle(key.label, for: .normal)
                btn.setTitleColor(textColor, for: .normal)
                btn.titleLabel?.font = keyFont(for: key)
                btn.backgroundColor = key.isSpecial ? specialColor : keyColor
                btn.layer.cornerRadius = keyCorner
                btn.translatesAutoresizingMaskIntoConstraints = false
                keysContainer.addSubview(btn)
                keyButtons[btn] = key
                btn.addTarget(self, action: #selector(keyTapped(_:)), for: .touchUpInside)

                if case .translit = key.action { translitButton = btn }
                if case .langSwitch = key.action { langButton = btn }
                btn.tag = rowIdx * 100 + (row.firstIndex(where: { $0.label == key.label }) ?? 0)
            }
        }

        setNeedsLayout()
        updateTranslitButton()
        updateLangButton()
    }

    override func layoutSubviews() {
        super.layoutSubviews()
        guard !rows.isEmpty else { return }

        let w = keysContainer.bounds.width
        let h = keysContainer.bounds.height
        let rowCount = CGFloat(rows.count)
        let totalRowSpacing = rowSpacing * (rowCount + 1)
        let keyH = (h - totalRowSpacing) / rowCount

        var btnIterator = keysContainer.subviews.makeIterator()
        var y = rowSpacing

        for row in rows {
            let totalWeight = row.reduce(0.0) { $0 + $1.widthWeight }
            let totalColSpacing = keySpacing * CGFloat(row.count + 1)
            let unitW = (w - totalColSpacing) / totalWeight
            var x = keySpacing

            for key in row {
                guard let btn = btnIterator.next() else { continue }
                let keyW = unitW * key.widthWeight
                btn.frame = CGRect(x: x, y: y, width: keyW, height: keyH)
                x += keyW + keySpacing
            }
            y += keyH + rowSpacing
        }
    }

    private func keyFont(for key: KeyDef) -> UIFont {
        key.isSpecial && key.label.count > 1
            ? .systemFont(ofSize: 13, weight: .medium)
            : .systemFont(ofSize: 20, weight: .regular)
    }

    @objc private func keyTapped(_ sender: UIButton) {
        guard let key = keyButtons[sender] else { return }
        UIImpactFeedbackGenerator(style: .light).impactOccurred()
        delegate?.didTapKey(key)
    }

    private func updateTranslitButton() {
        translitButton?.backgroundColor = translitActive
            ? translitOnColor.withAlphaComponent(0.3) : specialColor
        translitButton?.setTitleColor(translitActive ? translitOnColor : textColor, for: .normal)
    }

    private func updateLangButton() {
        langButton?.setTitleColor(langColor, for: .normal)
    }
}
