// -*- coding: utf-8 -*-
// dllmain.cpp - SAPI5 DLL entry points

#ifdef _WIN32

#include <windows.h>
#include <atlbase.h>
#include <atlcom.h>
#include <sapi.h>
#include <sphelper.h>
#include <string>

// Type library GUID for ATL
// {D4B5A4E1-8B5C-4C5D-9E6F-1A2B3C4D5E7F}
// Same CLSID for both 32-bit and 64-bit - Windows registry virtualization handles routing
struct __declspec(uuid("D4B5A4E1-8B5C-4C5D-9E6F-1A2B3C4D5E7F")) LIBID_LaprdusLib;
#define LAPRDUS_CLSID_STRING L"{D4B5A4E1-8B5C-4C5D-9E6F-1A2B3C4D5E6F}"

// Resource ID for registry script
#define IDR_LAPRDUS 101

// ATL Module
class CLaprdusModule : public ATL::CAtlDllModuleT<CLaprdusModule>
{
public:
    DECLARE_LIBID(__uuidof(LIBID_LaprdusLib))
    DECLARE_REGISTRY_APPID_RESOURCEID(IDR_LAPRDUS, "{D4B5A4E1-8B5C-4C5D-9E6F-1A2B3C4D5E7F}")
};

CLaprdusModule _AtlModule;

// DLL Entry Point
extern "C" BOOL WINAPI DllMain(
    HINSTANCE hInstance,
    DWORD dwReason,
    LPVOID lpReserved)
{
    (void)hInstance;  // Unused
    return _AtlModule.DllMain(dwReason, lpReserved);
}

// COM Registration
STDAPI DllCanUnloadNow(void)
{
    return _AtlModule.DllCanUnloadNow();
}

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    return _AtlModule.DllGetClassObject(rclsid, riid, ppv);
}

STDAPI DllRegisterServer(void)
{
    // Only register COM class - voice tokens are handled by the installer
    return _AtlModule.DllRegisterServer();
}

STDAPI DllUnregisterServer(void)
{
    // Only unregister COM class - voice tokens are handled by the installer
    return _AtlModule.DllUnregisterServer();
}

STDAPI DllInstall(BOOL bInstall, LPCWSTR pszCmdLine)
{
    (void)pszCmdLine;  // Unused
    HRESULT hr = E_FAIL;

    if (bInstall) {
        hr = DllRegisterServer();
        if (FAILED(hr)) {
            DllUnregisterServer();
        }
    } else {
        hr = DllUnregisterServer();
    }

    return hr;
}

#endif // _WIN32
