#include "clavi/platform/tray.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <shellapi.h>

#include <atomic>
#include <thread>

namespace clavi {

namespace {

constexpr UINT WM_TRAY = WM_USER + 100;
constexpr UINT IDM_TOGGLE = 1001;
constexpr UINT IDM_QUIT   = 1002;

class TrayWin final : public ITray {
public:
    ~TrayWin() override { shutdown(); }

    void init(Callbacks cbs) override {
        cbs_ = std::move(cbs);
        thread_ = std::thread([this]() { thread_main(); });
    }

    void set_enabled(bool enabled) override {
        enabled_.store(enabled);
        // Post a dummy message to wake the pump and update the tooltip
        if (hwnd_) PostMessageW(hwnd_, WM_USER + 200, 0, 0);
    }

    void shutdown() override {
        if (!running_.exchange(false)) return;
        if (hwnd_) PostMessageW(hwnd_, WM_QUIT, 0, 0);
        if (thread_.joinable()) thread_.join();
    }

private:
    Callbacks cbs_;
    std::thread thread_;
    std::atomic<bool> running_{true};
    std::atomic<bool> enabled_{true};
    HWND hwnd_{nullptr};
    NOTIFYICONDATAW nid_{};

    void thread_main() {
        WNDCLASSEXW wc{};
        wc.cbSize        = sizeof(wc);
        wc.lpfnWndProc   = wnd_proc_static;
        wc.hInstance     = GetModuleHandleW(nullptr);
        wc.lpszClassName = L"ClaviTrayWnd";
        RegisterClassExW(&wc);

        hwnd_ = CreateWindowExW(0, L"ClaviTrayWnd", L"", 0,
                                0, 0, 0, 0, HWND_MESSAGE, nullptr,
                                GetModuleHandleW(nullptr), nullptr);
        if (!hwnd_) return;

        SetWindowLongPtrW(hwnd_, GWLP_USERDATA,
                          reinterpret_cast<LONG_PTR>(this));

        // Add tray icon
        nid_.cbSize           = sizeof(nid_);
        nid_.hWnd             = hwnd_;
        nid_.uID              = 2; // different from toast's ID=1
        nid_.uFlags           = NIF_ICON | NIF_TIP | NIF_MESSAGE;
        nid_.uCallbackMessage = WM_TRAY;
        nid_.hIcon            = LoadIconW(nullptr,
                                    reinterpret_cast<LPCWSTR>(IDI_APPLICATION));
        wcscpy_s(nid_.szTip, std::size(nid_.szTip), L"Clavi — enabled");
        Shell_NotifyIconW(NIM_ADD, &nid_);

        MSG msg;
        while (running_.load() && GetMessageW(&msg, nullptr, 0, 0) > 0) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }

        Shell_NotifyIconW(NIM_DELETE, &nid_);
        DestroyWindow(hwnd_);
        hwnd_ = nullptr;
    }

    static LRESULT CALLBACK wnd_proc_static(HWND hwnd, UINT msg,
                                            WPARAM wp, LPARAM lp) {
        auto* self = reinterpret_cast<TrayWin*>(
            GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        if (self) return self->wnd_proc(hwnd, msg, wp, lp);
        return DefWindowProcW(hwnd, msg, wp, lp);
    }

    LRESULT wnd_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
        if (msg == WM_TRAY) {
            if (LOWORD(lp) == WM_RBUTTONUP || LOWORD(lp) == WM_CONTEXTMENU) {
                show_menu();
            }
            return 0;
        }
        if (msg == WM_COMMAND) {
            const UINT id = LOWORD(wp);
            if (id == IDM_TOGGLE && cbs_.on_toggle) cbs_.on_toggle();
            if (id == IDM_QUIT   && cbs_.on_quit)   cbs_.on_quit();
            return 0;
        }
        if (msg == WM_USER + 200) {
            // Update tooltip
            const bool on = enabled_.load();
            wcscpy_s(nid_.szTip, std::size(nid_.szTip),
                     on ? L"Clavi \u2014 enabled" : L"Clavi \u2014 paused");
            nid_.uFlags = NIF_TIP;
            Shell_NotifyIconW(NIM_MODIFY, &nid_);
            return 0;
        }
        return DefWindowProcW(hwnd, msg, wp, lp);
    }

    void show_menu() {
        HMENU menu = CreatePopupMenu();
        const bool on = enabled_.load();
        AppendMenuW(menu, MF_STRING, IDM_TOGGLE,
                    on ? L"Pause" : L"Enable");
        AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
        AppendMenuW(menu, MF_STRING, IDM_QUIT, L"Quit Clavi");

        POINT pt;
        GetCursorPos(&pt);
        SetForegroundWindow(hwnd_);
        TrackPopupMenu(menu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd_, nullptr);
        PostMessageW(hwnd_, WM_NULL, 0, 0);
        DestroyMenu(menu);
    }
};

} // namespace

std::unique_ptr<ITray> ITray::create() {
    return std::make_unique<TrayWin>();
}

} // namespace clavi
