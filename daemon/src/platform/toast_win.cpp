#include "clavi/platform/toast.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <shellapi.h>

#include <string>

namespace clavi {

namespace {

// Minimal balloon-tip tray notification using Shell_NotifyIcon.
// No external dependencies required.

class ToastWin final : public IToast {
public:
    ToastWin() {
        // Create a hidden message-only window to receive tray callbacks
        WNDCLASSEXW wc{};
        wc.cbSize        = sizeof(wc);
        wc.lpfnWndProc   = DefWindowProcW;
        wc.hInstance     = GetModuleHandleW(nullptr);
        wc.lpszClassName = L"ClaviToastWnd";
        RegisterClassExW(&wc);

        hwnd_ = CreateWindowExW(0, L"ClaviToastWnd", L"", 0,
                                0, 0, 0, 0, HWND_MESSAGE, nullptr,
                                GetModuleHandleW(nullptr), nullptr);
        if (hwnd_) init_tray();
    }

    ~ToastWin() override {
        if (hwnd_) {
            NOTIFYICONDATAW nid{};
            nid.cbSize = sizeof(nid);
            nid.hWnd   = hwnd_;
            nid.uID    = 1;
            Shell_NotifyIconW(NIM_DELETE, &nid);
            DestroyWindow(hwnd_);
        }
    }

    void show(std::string_view title, std::string_view body,
              int timeout_ms) override {
        if (!hwnd_) return;

        // Convert UTF-8 to UTF-16
        auto to_wide = [](std::string_view s, wchar_t* buf, int n) {
            MultiByteToWideChar(CP_UTF8, 0, s.data(),
                                static_cast<int>(s.size()), buf, n);
        };

        NOTIFYICONDATAW nid{};
        nid.cbSize      = sizeof(nid);
        nid.hWnd        = hwnd_;
        nid.uID         = 1;
        nid.uFlags      = NIF_INFO;
        nid.dwInfoFlags = NIIF_INFO;
        nid.uTimeout    = static_cast<UINT>(timeout_ms > 0 ? timeout_ms : 2000);

        to_wide(title, nid.szInfoTitle,
                static_cast<int>(std::size(nid.szInfoTitle)) - 1);
        to_wide(body, nid.szInfo,
                static_cast<int>(std::size(nid.szInfo)) - 1);

        Shell_NotifyIconW(NIM_MODIFY, &nid);
    }

private:
    HWND hwnd_{nullptr};

    void init_tray() {
        NOTIFYICONDATAW nid{};
        nid.cbSize           = sizeof(nid);
        nid.hWnd             = hwnd_;
        nid.uID              = 1;
        nid.uFlags           = NIF_ICON | NIF_TIP | NIF_MESSAGE;
        nid.uCallbackMessage = WM_USER + 1;
        nid.hIcon            = LoadIconW(nullptr, reinterpret_cast<LPCWSTR>(IDI_APPLICATION));
        wcscpy_s(nid.szTip, std::size(nid.szTip), L"Clavi");
        Shell_NotifyIconW(NIM_ADD, &nid);
    }
};

} // namespace

std::unique_ptr<IToast> IToast::create() {
    return std::make_unique<ToastWin>();
}

} // namespace clavi
