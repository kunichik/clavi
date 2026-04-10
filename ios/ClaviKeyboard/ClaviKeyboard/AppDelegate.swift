import UIKit

@main
final class AppDelegate: UIResponder, UIApplicationDelegate {

    var window: UIWindow?

    func application(
        _ application: UIApplication,
        didFinishLaunchingWithOptions launchOptions: [UIApplication.LaunchOptionsKey: Any]?
    ) -> Bool {
        let settingsVC = SettingsViewController()
        let nav = UINavigationController(rootViewController: settingsVC)

        // Dark navigation bar to match the app palette
        let appearance = UINavigationBarAppearance()
        appearance.configureWithOpaqueBackground()
        appearance.backgroundColor = UIColor(red: 0.149, green: 0.196, blue: 0.220, alpha: 1)
        appearance.titleTextAttributes = [.foregroundColor: UIColor.white]
        appearance.largeTitleTextAttributes = [.foregroundColor: UIColor.white]
        nav.navigationBar.standardAppearance = appearance
        nav.navigationBar.scrollEdgeAppearance = appearance
        nav.navigationBar.tintColor = UIColor(red: 0.310, green: 0.761, blue: 0.969, alpha: 1)

        window = UIWindow(frame: UIScreen.main.bounds)
        window?.rootViewController = nav
        window?.backgroundColor = UIColor(red: 0.149, green: 0.196, blue: 0.220, alpha: 1)
        window?.makeKeyAndVisible()
        return true
    }
}
