// -*- coding: utf-8 -*-
// sapi_driver.hpp - Windows SAPI5 TTS driver
// Implements ISpTTSEngine for Windows Speech API integration

#ifndef LAPRDUS_SAPI_DRIVER_HPP
#define LAPRDUS_SAPI_DRIVER_HPP

#ifdef _WIN32

// ATL headers MUST be included before SAPI headers
#include <atlbase.h>
#include <atlcom.h>

// sphelper.h requires unqualified CComPtr
using namespace ATL;

#include <windows.h>
#include <sapi.h>
#include <sphelper.h>
#include <memory>

#include "core/tts_engine.hpp"

// Resource ID for registry script (must match laprdus_sapi5.rc)
#define IDR_LAPRDUS 101

// {D4B5A4E1-8B5C-4C5D-9E6F-1A2B3C4D5E6F}
// LaprdusTTS SAPI engine CLSID - declaration only, defined in sapi_driver.cpp
EXTERN_C const GUID CLSID_LaprdusTTS;

namespace laprdus {

/**
 * LaprdusSAPIDriver - SAPI5 TTS Engine Implementation
 *
 * Implements the ISpTTSEngine interface required by Windows SAPI.
 * This allows applications to use LaprdusTTS through the standard
 * Windows text-to-speech system.
 *
 * Registry entries required:
 * - HKLM\SOFTWARE\Microsoft\Speech\Voices\Tokens\Laprdus
 * - COM registration for CLSID_LaprdusTTS
 */
class ATL_NO_VTABLE LaprdusSAPIDriver :
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<LaprdusSAPIDriver, &CLSID_LaprdusTTS>,
    public ISpTTSEngine,
    public ISpObjectWithToken
{
public:
    LaprdusSAPIDriver();
    ~LaprdusSAPIDriver();

    DECLARE_REGISTRY_RESOURCEID(IDR_LAPRDUS)
    DECLARE_NOT_AGGREGATABLE(LaprdusSAPIDriver)

    BEGIN_COM_MAP(LaprdusSAPIDriver)
        COM_INTERFACE_ENTRY(ISpTTSEngine)
        COM_INTERFACE_ENTRY(ISpObjectWithToken)
    END_COM_MAP()

    DECLARE_PROTECT_FINAL_CONSTRUCT()

    HRESULT FinalConstruct();
    void FinalRelease();

    // ISpObjectWithToken
    STDMETHOD(SetObjectToken)(ISpObjectToken* pToken) override;
    STDMETHOD(GetObjectToken)(ISpObjectToken** ppToken) override;

    // ISpTTSEngine
    STDMETHOD(Speak)(
        DWORD dwSpeakFlags,
        REFGUID rguidFormatId,
        const WAVEFORMATEX* pWaveFormatEx,
        const SPVTEXTFRAG* pTextFragList,
        ISpTTSEngineSite* pOutputSite) override;

    STDMETHOD(GetOutputFormat)(
        const GUID* pTargetFmtId,
        const WAVEFORMATEX* pTargetWaveFormatEx,
        GUID* pOutputFormatId,
        WAVEFORMATEX** ppCoMemOutputWaveFormatEx) override;

private:
    CComPtr<ISpObjectToken> m_token;
    std::unique_ptr<TTSEngine> m_engine;
    bool m_initialized = false;
    float m_basePitch = 1.0f;  // Base pitch from voice token (for derived voices)

    // User settings (loaded from %APPDATA%\Laprdus\settings.json)
    float m_userSpeed = 1.0f;
    float m_userPitch = 1.0f;
    float m_userVolume = 1.0f;
    bool m_forceSpeed = false;
    bool m_forcePitch = false;
    bool m_forceVolume = false;

    // Initialize the TTS engine from token attributes
    HRESULT InitializeEngine();

    // Convert SAPI text fragments to plain text
    // Sets useSpelledSynthesis to true if SPVA_SpellOut is encountered
    std::wstring ExtractText(const SPVTEXTFRAG* pTextFragList, bool& useSpelledSynthesis);

    // Convert wide string to UTF-8
    std::string WideToUtf8(const std::wstring& wide);

    // Send audio to SAPI output site
    HRESULT SendAudio(ISpTTSEngineSite* pSite, const AudioBuffer& audio);

    // Check if synthesis should be cancelled
    bool ShouldStop(ISpTTSEngineSite* pSite);
};

// Factory for creating the SAPI driver
OBJECT_ENTRY_AUTO(CLSID_LaprdusTTS, LaprdusSAPIDriver)

} // namespace laprdus

#endif // _WIN32
#endif // LAPRDUS_SAPI_DRIVER_HPP
