%global debug_package %{nil}

Name:           laprdus
Version:        2.1.0
Release:        1%{?dist}
Summary:        Croatian/Serbian/Bosnian/Slovenian Text-to-Speech Engine

License:        GPL-3.0-or-later
URL:            https://hrvojekatic.com/laprdus
Source0:        %{name}-%{version}.tar.xz

BuildRequires:  cargo
BuildRequires:  rust >= 1.85
BuildRequires:  gcc
BuildRequires:  pkgconfig
BuildRequires:  alsa-lib-devel

Requires:       speech-dispatcher

%description
LaprdusTTS is a text-to-speech engine for Croatian, Serbian, Bosnian and
Slovenian,
with voices synthesized by rule (formant synthesis) and voices rendered
from pre-recorded phoneme samples.

Features:
* Voices: Josip, Vlado, Detence, Baba, Đedo, Zvonko (default), Stojan,
  Mirsad and ten singing voices; every voice speaks Croatian, Serbian,
  Bosnian and Slovenian
* Speech Dispatcher integration for use with Orca screen reader
* Command-line utility for terminal-based speech synthesis
* Adjustable speech rate, pitch, and volume
* Punctuation-based voice inflection
* Number-to-words conversion

%package devel
Summary:        Development files for LaprdusTTS
Requires:       %{name}%{?_isa} = %{version}-%{release}

%description devel
This package provides the development headers for LaprdusTTS,
allowing developers to integrate Croatian, Serbian, Bosnian and Slovenian
text-to-speech into their applications.

%prep
%autosetup

%build
# The voice data and dictionaries, then the CLI, the Speech Dispatcher module
# and the C API library.
cargo --locked xtask build --locked

%install
# The library (liblaprdus.so.%{version} and its links), the CLI, the header,
# the voice data and dictionaries, the Speech Dispatcher module (it needs no
# Speech Dispatcher headers to build) and its configuration.
cargo --locked xtask install --locked --destdir %{buildroot} \
    --prefix %{_prefix} --libdir %{_libdir} \
    --moduledir %{_libdir}/speech-dispatcher-modules

%ldconfig_scriptlets

%post
# Configure Speech Dispatcher for LaprdusTTS
SPEECHD_CONF="/etc/speech-dispatcher/speechd.conf"
MARKER_START="# BEGIN LAPRDUS TTS"
MARKER_END="# END LAPRDUS TTS"

if [ -f "$SPEECHD_CONF" ]; then
    # A block written by a version that did not know Slovenian (or Bosnian)
    # yet is removed here and written again below.
    if grep -q "$MARKER_START" "$SPEECHD_CONF" 2>/dev/null && \
       ! grep -q 'LanguageDefaultModule "sl" "laprdus"' "$SPEECHD_CONF" 2>/dev/null; then
        sed -i "/$MARKER_START/,/$MARKER_END/d" "$SPEECHD_CONF"
    fi
    # Check if already configured
    if ! grep -q "$MARKER_START" "$SPEECHD_CONF" 2>/dev/null; then
        # Check for existing manual configuration
        if ! grep -q 'AddModule.*"laprdus"' "$SPEECHD_CONF" 2>/dev/null; then
            echo "Configuring Speech Dispatcher for LaprdusTTS..."
            # SD 0.12+ autodetects modules when no AddModule lines are present.
            # Adding AddModule disables autodetection for all other modules.
            if grep -q '^[[:space:]]*AddModule' "$SPEECHD_CONF" 2>/dev/null; then
                # Explicit mode: add our AddModule too
                cat >> "$SPEECHD_CONF" << 'EOF'

# BEGIN LAPRDUS TTS
# LaprdusTTS - Croatian/Serbian/Bosnian/Slovenian Text-to-Speech
AddModule "laprdus" "sd_laprdus" "laprdus.conf"
LanguageDefaultModule "hr" "laprdus"
LanguageDefaultModule "sr" "laprdus"
LanguageDefaultModule "hr-HR" "laprdus"
LanguageDefaultModule "sr-RS" "laprdus"
LanguageDefaultModule "bs" "laprdus"
LanguageDefaultModule "bs-BA" "laprdus"
LanguageDefaultModule "sl" "laprdus"
LanguageDefaultModule "sl-SI" "laprdus"
# END LAPRDUS TTS
EOF
            else
                # Autodetection mode: only add language defaults
                cat >> "$SPEECHD_CONF" << 'EOF'

# BEGIN LAPRDUS TTS
# LaprdusTTS - Croatian/Serbian/Bosnian/Slovenian Text-to-Speech
LanguageDefaultModule "hr" "laprdus"
LanguageDefaultModule "sr" "laprdus"
LanguageDefaultModule "hr-HR" "laprdus"
LanguageDefaultModule "sr-RS" "laprdus"
LanguageDefaultModule "bs" "laprdus"
LanguageDefaultModule "bs-BA" "laprdus"
LanguageDefaultModule "sl" "laprdus"
LanguageDefaultModule "sl-SI" "laprdus"
# END LAPRDUS TTS
EOF
            fi
            echo "LaprdusTTS configured successfully."
        fi
    fi
fi

# Restart speech-dispatcher if running (try system service, then user services)
systemctl try-restart speech-dispatcher 2>/dev/null || true
for uid in $(loginctl list-users --no-legend 2>/dev/null | awk '{print $1}'); do
    systemctl --user -M "${uid}@" try-restart speech-dispatcher 2>/dev/null || true
done

%preun
# Remove configuration on package removal (not upgrade)
if [ $1 -eq 0 ]; then
    SPEECHD_CONF="/etc/speech-dispatcher/speechd.conf"
    MARKER_START="# BEGIN LAPRDUS TTS"
    MARKER_END="# END LAPRDUS TTS"

    if [ -f "$SPEECHD_CONF" ]; then
        if grep -q "$MARKER_START" "$SPEECHD_CONF" 2>/dev/null; then
            echo "Removing LaprdusTTS from Speech Dispatcher configuration..."
            sed -i "/$MARKER_START/,/$MARKER_END/d" "$SPEECHD_CONF"
        fi
        # Also remove language-only block if present
        sed -i '/# BEGIN LAPRDUS TTS LANGUAGES/,/# END LAPRDUS TTS LANGUAGES/d' "$SPEECHD_CONF" 2>/dev/null || true
    fi
fi

%postun
# Restart speech-dispatcher after removal
if [ $1 -eq 0 ]; then
    systemctl try-restart speech-dispatcher 2>/dev/null || true
    for uid in $(loginctl list-users --no-legend 2>/dev/null | awk '{print $1}'); do
        systemctl --user -M "${uid}@" try-restart speech-dispatcher 2>/dev/null || true
    done
    # Fallback: kill any remaining speech-dispatcher processes to force
    # a clean restart on next client connection (via socket activation).
    # This handles cases where systemctl cannot reach the user's service
    # (e.g., socket-activated user services on Fedora).
    killall speech-dispatcher 2>/dev/null || true
fi

%files
%license LICENSE
%doc readme.md
%{_libdir}/liblaprdus.so.2*
%{_bindir}/laprdus
%{_datadir}/laprdus/
%{_libdir}/speech-dispatcher-modules/sd_laprdus
%config(noreplace) %{_sysconfdir}/speech-dispatcher/modules/laprdus.conf

%files devel
%{_libdir}/liblaprdus.so
%{_includedir}/laprdus/

%changelog
* Fri Nov 27 2026 Hrvoje Katic <hrvoje.katic@gmail.com> - 2.1.0-1
- In development

* Wed Oct 07 2026 Hrvoje Katic <hrvoje.katic@gmail.com> - 2.0.0-1
- Laprdus 2.0
- New formant voices synthesized by rule: Zvonko (the default voice), Stojan and Mirsad, and ten singing voices
- Every voice speaks Croatian, Serbian and Bosnian: the language is chosen apart from the voice (CLI -L; Speech Dispatcher SET LANGUAGE keeps the voice)
- Voice ids are plain ASCII (djedo for Đedo); the old ids djed, klapa, pjevac, pevac and solist are still accepted
- Library SONAME bumped to liblaprdus.so.2
- Built with Cargo: the engine and the programs are written in Rust

* Sun Feb 08 2026 Hrvoje Katic <hrvoje.katic@gmail.com> - 1.0.0-1
- Merge speechd module into main package for single-package installation
- Add SONAME to shared library for proper dependency resolution
- Include emoji dictionary

* Fri Jan 31 2025 Hrvoje Katic <hrvoje.katic@gmail.com> - 1.0.0-1
- Initial package
- Croatian and Serbian text-to-speech synthesis
- Speech Dispatcher integration for Orca screen reader
- Command-line utility
- Five voices: Josip, Vlado, Detence, Baba, Djedo
