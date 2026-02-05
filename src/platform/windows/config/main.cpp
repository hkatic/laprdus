// -*- coding: utf-8 -*-
// main.cpp - LaprdusTTS configuration GUI entry point

#ifdef _WIN32

#include <windows.h>
#include <commctrl.h>
#include "config_dialog.hpp"

// Link with required libraries
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "ole32.lib")

// Enable visual styles
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

int WINAPI wWinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR lpCmdLine,
    _In_ int nShowCmd
) {
    // Suppress unused parameter warnings
    (void)hPrevInstance;
    (void)lpCmdLine;
    (void)nShowCmd;

    // Set DPI awareness for proper scaling on high-DPI displays
    // Use SetProcessDpiAwarenessContext if available (Windows 10 1703+)
    HMODULE hUser32 = GetModuleHandleW(L"user32.dll");
    if (hUser32) {
        typedef BOOL(WINAPI* SetProcessDpiAwarenessContextProc)(DPI_AWARENESS_CONTEXT);
        auto pSetProcessDpiAwarenessContext =
            (SetProcessDpiAwarenessContextProc)GetProcAddress(hUser32, "SetProcessDpiAwarenessContext");

        if (pSetProcessDpiAwarenessContext) {
            pSetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
        } else {
            // Fallback for older Windows versions
            typedef BOOL(WINAPI* SetProcessDPIAwareProc)();
            auto pSetProcessDPIAware =
                (SetProcessDPIAwareProc)GetProcAddress(hUser32, "SetProcessDPIAware");

            if (pSetProcessDPIAware) {
                pSetProcessDPIAware();
            }
        }
    }

    // Run the configuration dialog
    return laprdus::RunConfigDialog(hInstance);
}

#endif // _WIN32
