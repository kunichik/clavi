import UIKit

protocol ClaviKeyboardViewDelegate: AnyObject {
    func didTapKey(_ key: KeyDef)
    func didTapClip(at index: Int)
    func didLongPressClip(at index: Int)
    func didTapClearClips()
    func didTapNextKeyboard()
}

/**
 * Custom keyboard view drawn with UIKit.
 * Renders: [clipboard strip] + [keyboard rows]
 */
class ClaviKeyboardView: UIView {

    weak var delegate: ClaviKeyboardViewDelegate?

    var rows: [[KeyDef]] = [] { didSet { rebuildKeys() } }
    var translitActive = false { didSet { updateTranslitButton() } }
    var currentLanguage: Language = .uk { didSet { updateLangButton() } }
    var clipItems: [String] = [] { didSet { updateStrip() } }

    // MARK: - Subviews

    private let stripScrollView = UIScrollView()
    private let stripStack = UIStackView()
    private let stripClearBtn = UIButton(type: .system)
    private let keysContainer = UIView()
    private var keyButtons: [UIButton: KeyDef] = [:]
    private var translitButton: UIButton?
    private var langButton: UIButton?

    // MARK: - Constants

    private let stripHeight: CGFloat = 40
    private let keySpacing: CGFloat = 6
    private let rowSpacing: CGFloat = 6
    private let keyCorner: CGFloat = 8

    // Colors
    private let bgColor       = UIColor(red: 0.15, green: 0.19, blue: 0.20, alpha: 1)
    private let keyColor      = UIColor(red: 0.22, green: 0.28, blue: 0.31, alpha: 1)
    private let specialColor  = UIColor(red: 0.27, green: 0.35, blue: 0.40, alpha: 1)
    private let textColor     = UIColor.white
    private let translitOnColor = UIColor(red: 0.31, green: 0.76, blue: 0.97, alpha: 1)
    private let langColor     = UIColor(red: 1.0,  green: 0.67, blue: 0.25, alpha: 1)
    private let stripBgColor  = UIColor(red: 0.11, green: 0.17, blue: 0.19, alpha: 1)
    private let chipColor     = UIColor(red: 0.22, green: 0.28, blue: 0.31, alpha: 1)

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

        // Strip
        stripScrollView.backgroundColor = stripBgColor
        stripScrollView.showsHorizontalScrollIndicator = false
        stripScrollView.translatesAutoresizingMaskIntoConstraints = false
        addSubview(stripScrollView)

        stripStack.axis = .horizontal
        stripStack.spacing = 6
        stripStack.alignment = .center
        stripStack.translatesAutoresizingMaskIntoConstraints = false
        stripScrollView.addSubview(stripStack)

        stripClearBtn.setTitle("×", for: .normal)
        stripClearBtn.setTitleColor(.red, for: .normal)
        stripClearBtn.titleLabel?.font = .systemFont(ofSize: 18, weight: .bold)
        stripClearBtn.translatesAutoresizingMaskIntoConstraints = false
        stripClearBtn.addTarget(self, action: #selector(clearStrip), for: .touchUpInside)
        addSubview(stripClearBtn)

        // Keys container
        keysContainer.translatesAutoresizingMaskIntoConstraints = false
        addSubview(keysContainer)

        NSLayoutConstraint.activate([
            stripScrollView.leadingAnchor.constraint(equalTo: leadingAnchor),
            stripScrollView.trailingAnchor.constraint(equalTo: trailingAnchor, constant: -44),
            stripScrollView.topAnchor.constraint(equalTo: topAnchor),
            stripScrollView.heightAnchor.constraint(equalToConstant: stripHeight),

            stripClearBtn.trailingAnchor.constraint(equalTo: trailingAnchor, constant: -4),
            stripClearBtn.centerYAnchor.constraint(equalTo: stripScrollView.centerYAnchor),
            stripClearBtn.widthAnchor.constraint(equalToConstant: 40),

            stripStack.leadingAnchor.constraint(equalTo: stripScrollView.leadingAnchor, constant: 8),
            stripStack.trailingAnchor.constraint(equalTo: stripScrollView.trailingAnchor, constant: -8),
            stripStack.topAnchor.constraint(equalTo: stripScrollView.topAnchor, constant: 4),
            stripStack.bottomAnchor.constraint(equalTo: stripScrollView.bottomAnchor, constant: -4),
            stripStack.heightAnchor.constraint(equalToConstant: stripHeight - 8),

            keysContainer.leadingAnchor.constraint(equalTo: leadingAnchor),
            keysContainer.trailingAnchor.constraint(equalTo: trailingAnchor),
            keysContainer.topAnchor.constraint(equalTo: stripScrollView.bottomAnchor),
            keysContainer.bottomAnchor.constraint(equalTo: bottomAnchor),
        ])

        updateStrip()
    }

    // MARK: - Strip

    private func updateStrip() {
        stripStack.arrangedSubviews.forEach { $0.removeFromSuperview() }
        stripScrollView.isHidden = clipItems.isEmpty
        stripClearBtn.isHidden = clipItems.isEmpty

        clipItems.enumerated().forEach { (i, label) in
            let btn = UIButton(type: .system)
            btn.setTitle(label, for: .normal)
            btn.setTitleColor(UIColor(red: 0.88, green: 0.97, blue: 0.98, alpha: 1), for: .normal)
            btn.titleLabel?.font = .systemFont(ofSize: 13)
            btn.backgroundColor = chipColor
            btn.layer.cornerRadius = 6
            btn.contentEdgeInsets = UIEdgeInsets(top: 4, left: 10, bottom: 4, right: 10)
            btn.tag = i
            btn.addTarget(self, action: #selector(chipTapped(_:)), for: .touchUpInside)

            let lp = UILongPressGestureRecognizer(target: self, action: #selector(chipLongPressed(_:)))
            btn.addGestureRecognizer(lp)

            stripStack.addArrangedSubview(btn)
        }
    }

    @objc private func chipTapped(_ sender: UIButton) {
        delegate?.didTapClip(at: sender.tag)
    }

    @objc private func chipLongPressed(_ gr: UILongPressGestureRecognizer) {
        guard gr.state == .began, let btn = gr.view as? UIButton else { return }
        delegate?.didLongPressClip(at: btn.tag)
    }

    @objc private func clearStrip() {
        delegate?.didTapClearClips()
    }

    // MARK: - Keys

    private func rebuildKeys() {
        keysContainer.subviews.forEach { $0.removeFromSuperview() }
        keyButtons.removeAll()
        translitButton = nil
        langButton = nil

        let rowCount = CGFloat(rows.count)
        let totalRowSpacing = rowSpacing * (rowCount + 1)

        for (rowIdx, row) in rows.enumerated() {
            let totalWeight = row.reduce(0) { $0 + $1.widthWeight }

            for (_, key) in row.enumerated() {
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

                // Store references to special buttons
                if case .translit = key.action { translitButton = btn }
                if case .langSwitch = key.action { langButton = btn }

                // Layout using auto-layout with multiplier trick
                let keyHeightFraction = 1.0 / rowCount
                let keyTopFraction = CGFloat(rowIdx) / rowCount
                let keyWidthFraction = key.widthWeight / totalWeight

                // We'll use manual frame layout in layoutSubviews for flexibility
                btn.tag = rowIdx * 100 + row.firstIndex(where: { $0.label == key.label && $0.isSpecial == key.isSpecial }) ?? 0
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
        if key.isSpecial && key.label.count > 1 {
            return .systemFont(ofSize: 13, weight: .medium)
        }
        return .systemFont(ofSize: 20, weight: .regular)
    }

    @objc private func keyTapped(_ sender: UIButton) {
        guard let key = keyButtons[sender] else { return }
        UIImpactFeedbackGenerator(style: .light).impactOccurred()
        delegate?.didTapKey(key)
    }

    private func updateTranslitButton() {
        translitButton?.backgroundColor = translitActive
            ? translitOnColor.withAlphaComponent(0.3)
            : specialColor
        translitButton?.setTitleColor(translitActive ? translitOnColor : textColor, for: .normal)
    }

    private func updateLangButton() {
        langButton?.setTitleColor(langColor, for: .normal)
    }
}
