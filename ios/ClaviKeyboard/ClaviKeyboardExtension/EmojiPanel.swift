import UIKit

/**
 * Emoji picker panel (UIKit).
 * Overlays the full keyboard view on long-press of the space bar.
 *
 * Layout (top → bottom):
 *   Header bar: "Emoji" label + × dismiss button
 *   Recent strip: last 8 used emoji (horizontal scroll)
 *   Emoji grid: UICollectionView, 8 columns, scrollable
 */
class EmojiPanel: UIView {

    var onEmojiSelected: ((String) -> Void)?
    var onDismiss: (() -> Void)?

    // UICollectionView reuse id
    private let cellId = "EmojiCell"
    private let cols = 8

    private let headerView  = UIView()
    private let recentScroll = UIScrollView()
    private let recentStack  = UIStackView()
    private var collectionView: UICollectionView!

    private var displayedEmoji: [String] = []

    private let bgColor   = UIColor(red: 0.08, green: 0.10, blue: 0.12, alpha: 1)
    private let hdrColor  = UIColor(red: 0.11, green: 0.14, blue: 0.17, alpha: 1)
    private let chipBg    = UIColor(red: 0.18, green: 0.22, blue: 0.26, alpha: 1)

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
        EmojiData.shared.load()
        displayedEmoji = EmojiData.shared.all

        setupHeader()
        setupRecentStrip()
        setupGrid()
        layoutPanel()
        refreshRecent()
    }

    // MARK: - Header

    private func setupHeader() {
        headerView.backgroundColor = hdrColor
        headerView.translatesAutoresizingMaskIntoConstraints = false
        addSubview(headerView)

        let title = UILabel()
        title.text = "Emoji"
        title.font = .systemFont(ofSize: 14, weight: .semibold)
        title.textColor = .white
        title.translatesAutoresizingMaskIntoConstraints = false
        headerView.addSubview(title)

        let closeBtn = UIButton(type: .system)
        closeBtn.setTitle("✕", for: .normal)
        closeBtn.setTitleColor(UIColor(red: 1, green: 0.4, blue: 0.4, alpha: 1), for: .normal)
        closeBtn.titleLabel?.font = .systemFont(ofSize: 18, weight: .bold)
        closeBtn.translatesAutoresizingMaskIntoConstraints = false
        closeBtn.addTarget(self, action: #selector(dismissTapped), for: .touchUpInside)
        headerView.addSubview(closeBtn)

        NSLayoutConstraint.activate([
            title.leadingAnchor.constraint(equalTo: headerView.leadingAnchor, constant: 12),
            title.centerYAnchor.constraint(equalTo: headerView.centerYAnchor),
            closeBtn.trailingAnchor.constraint(equalTo: headerView.trailingAnchor, constant: -8),
            closeBtn.centerYAnchor.constraint(equalTo: headerView.centerYAnchor),
            closeBtn.widthAnchor.constraint(equalToConstant: 36),
        ])
    }

    // MARK: - Recent strip

    private func setupRecentStrip() {
        recentScroll.showsHorizontalScrollIndicator = false
        recentScroll.backgroundColor = UIColor(red: 0.10, green: 0.13, blue: 0.16, alpha: 1)
        recentScroll.translatesAutoresizingMaskIntoConstraints = false
        addSubview(recentScroll)

        recentStack.axis = .horizontal
        recentStack.spacing = 4
        recentStack.alignment = .center
        recentStack.translatesAutoresizingMaskIntoConstraints = false
        recentScroll.addSubview(recentStack)

        NSLayoutConstraint.activate([
            recentStack.leadingAnchor.constraint(equalTo: recentScroll.leadingAnchor, constant: 8),
            recentStack.trailingAnchor.constraint(equalTo: recentScroll.trailingAnchor, constant: -8),
            recentStack.topAnchor.constraint(equalTo: recentScroll.topAnchor, constant: 4),
            recentStack.bottomAnchor.constraint(equalTo: recentScroll.bottomAnchor, constant: -4),
        ])
    }

    private func refreshRecent() {
        recentStack.arrangedSubviews.forEach { $0.removeFromSuperview() }
        let recent = loadRecent()
        if recent.isEmpty {
            let hint = UILabel()
            hint.text = "No recent emoji"
            hint.font = .systemFont(ofSize: 11)
            hint.textColor = UIColor(white: 1, alpha: 0.4)
            recentStack.addArrangedSubview(hint)
        } else {
            recent.forEach { emoji in recentStack.addArrangedSubview(emojiButton(emoji, size: 26)) }
        }
    }

    // MARK: - Collection grid

    private func setupGrid() {
        let layout = UICollectionViewFlowLayout()
        layout.minimumInteritemSpacing = 0
        layout.minimumLineSpacing = 0
        collectionView = UICollectionView(frame: .zero, collectionViewLayout: layout)
        collectionView.backgroundColor = bgColor
        collectionView.register(EmojiCell.self, forCellWithReuseIdentifier: cellId)
        collectionView.dataSource = self
        collectionView.delegate = self
        collectionView.translatesAutoresizingMaskIntoConstraints = false
        addSubview(collectionView)
    }

    // MARK: - Layout constraints

    private func layoutPanel() {
        NSLayoutConstraint.activate([
            headerView.topAnchor.constraint(equalTo: topAnchor),
            headerView.leadingAnchor.constraint(equalTo: leadingAnchor),
            headerView.trailingAnchor.constraint(equalTo: trailingAnchor),
            headerView.heightAnchor.constraint(equalToConstant: 36),

            recentScroll.topAnchor.constraint(equalTo: headerView.bottomAnchor),
            recentScroll.leadingAnchor.constraint(equalTo: leadingAnchor),
            recentScroll.trailingAnchor.constraint(equalTo: trailingAnchor),
            recentScroll.heightAnchor.constraint(equalToConstant: 44),

            collectionView.topAnchor.constraint(equalTo: recentScroll.bottomAnchor),
            collectionView.leadingAnchor.constraint(equalTo: leadingAnchor),
            collectionView.trailingAnchor.constraint(equalTo: trailingAnchor),
            collectionView.bottomAnchor.constraint(equalTo: bottomAnchor),
        ])
    }

    // MARK: - Helper

    private func emojiButton(_ emoji: String, size: CGFloat) -> UIButton {
        let btn = UIButton(type: .custom)
        btn.setTitle(emoji, for: .normal)
        btn.titleLabel?.font = .systemFont(ofSize: size)
        btn.backgroundColor = .clear
        btn.widthAnchor.constraint(equalToConstant: size + 10).isActive = true
        btn.heightAnchor.constraint(equalToConstant: size + 10).isActive = true
        btn.addTarget(self, action: #selector(recentEmojiTapped(_:)), for: .touchUpInside)
        return btn
    }

    // MARK: - Actions

    @objc private func dismissTapped() {
        onDismiss?()
    }

    @objc private func recentEmojiTapped(_ sender: UIButton) {
        guard let emoji = sender.title(for: .normal) else { return }
        select(emoji)
    }

    private func select(_ emoji: String) {
        addToRecent(emoji)
        onEmojiSelected?(emoji)
    }

    // MARK: - Recent persistence

    private let recentKey = "emoji_recent"

    private func loadRecent() -> [String] {
        let raw = UserDefaults(suiteName: "group.com.clavi.keyboard")?.string(forKey: recentKey) ?? ""
        return raw.split(separator: ",").map(String.init).filter { !$0.isEmpty }
    }

    private func addToRecent(_ emoji: String) {
        var list = loadRecent()
        list.removeAll { $0 == emoji }
        list.insert(emoji, at: 0)
        let joined = list.prefix(8).joined(separator: ",")
        UserDefaults(suiteName: "group.com.clavi.keyboard")?.set(joined, forKey: recentKey)
        refreshRecent()
    }
}

// MARK: - UICollectionViewDataSource / Delegate

extension EmojiPanel: UICollectionViewDataSource, UICollectionViewDelegateFlowLayout {

    func collectionView(_ collectionView: UICollectionView, numberOfItemsInSection section: Int) -> Int {
        displayedEmoji.count
    }

    func collectionView(_ collectionView: UICollectionView, cellForItemAt indexPath: IndexPath) -> UICollectionViewCell {
        let cell = collectionView.dequeueReusableCell(withReuseIdentifier: cellId, for: indexPath) as! EmojiCell
        cell.configure(emoji: displayedEmoji[indexPath.item])
        return cell
    }

    func collectionView(_ collectionView: UICollectionView,
                        layout collectionViewLayout: UICollectionViewLayout,
                        sizeForItemAt indexPath: IndexPath) -> CGSize {
        let side = collectionView.bounds.width / CGFloat(cols)
        return CGSize(width: side, height: side)
    }

    func collectionView(_ collectionView: UICollectionView, didSelectItemAt indexPath: IndexPath) {
        select(displayedEmoji[indexPath.item])
    }
}

// MARK: - EmojiCell

private class EmojiCell: UICollectionViewCell {
    private let label = UILabel()

    override init(frame: CGRect) {
        super.init(frame: frame)
        label.font = .systemFont(ofSize: 26)
        label.textAlignment = .center
        label.translatesAutoresizingMaskIntoConstraints = false
        contentView.addSubview(label)
        NSLayoutConstraint.activate([
            label.centerXAnchor.constraint(equalTo: contentView.centerXAnchor),
            label.centerYAnchor.constraint(equalTo: contentView.centerYAnchor),
        ])
        // Tap highlight
        let highlight = UIView()
        highlight.backgroundColor = UIColor(white: 1, alpha: 0.12)
        highlight.layer.cornerRadius = 6
        highlight.alpha = 0
        highlight.translatesAutoresizingMaskIntoConstraints = false
        contentView.insertSubview(highlight, at: 0)
        NSLayoutConstraint.activate([
            highlight.leadingAnchor.constraint(equalTo: contentView.leadingAnchor, constant: 2),
            highlight.trailingAnchor.constraint(equalTo: contentView.trailingAnchor, constant: -2),
            highlight.topAnchor.constraint(equalTo: contentView.topAnchor, constant: 2),
            highlight.bottomAnchor.constraint(equalTo: contentView.bottomAnchor, constant: -2),
        ])
    }

    required init?(coder: NSCoder) { fatalError() }

    func configure(emoji: String) { label.text = emoji }

    override var isHighlighted: Bool {
        didSet {
            contentView.subviews.first?.alpha = isHighlighted ? 1 : 0
        }
    }
}
