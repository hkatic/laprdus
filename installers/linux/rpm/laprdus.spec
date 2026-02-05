Name:           laprdus
Version:        1.0.0
Release:        1%{?dist}
Summary:        Croatian/Serbian Text-to-Speech Engine

License:        GPL-3.0-or-later
URL:            https://hrvojekatic.com/laprdus
Source0:        %{name}-%{version}.tar.xz

BuildRequires:  gcc-c++
BuildRequires:  scons
BuildRequires:  pulseaudio-libs-devel
BuildRequires:  alsa-lib-devel
BuildRequires:  speech-dispatcher-devel
BuildRequires:  glib2-devel

%description
LaprdusTTS is a concatenative text-to-speech engine supporting Croatian
and Serbian languages. It provides natural-sounding speech synthesis
using pre-recorded phoneme samples.

Features:
* Multiple voices: Josip (Croatian), Vlado (Serbian), and derived voices
* Speech Dispatcher integration for use with Orca screen reader
* Command-line utility for terminal-based speech synthesis
* Adjustable speech rate, pitch, and volume
* Punctuation-based voice inflection
* Number-to-words conversion

%package speechd
Summary:        LaprdusTTS Speech Dispatcher module
Requires:       %{name}%{?_isa} = %{version}-%{release}
Requires:       speech-dispatcher

%description speechd
This package provides the Speech Dispatcher output module for LaprdusTTS,
enabling Croatian/Serbian text-to-speech in applications that use
Speech Dispatcher, such as Orca screen reader.

%package devel
Summary:        Development files for LaprdusTTS
Requires:       %{name}%{?_isa} = %{version}-%{release}

%description devel
This package provides the development headers for LaprdusTTS,
allowing developers to integrate Croatian/Serbian text-to-speech
into their applications.

%prep
%autosetup

%build
scons --platform=linux --build-config=release linux-all

%install
# Install library
install -D -m 755 build/linux-x64-release/liblaprdus.so \
    %{buildroot}%{_libdir}/liblaprdus.so.1.0.0
ln -s liblaprdus.so.1.0.0 %{buildroot}%{_libdir}/liblaprdus.so.1
ln -s liblaprdus.so.1 %{buildroot}%{_libdir}/liblaprdus.so

# Install CLI
install -D -m 755 build/linux-x64-release/laprdus \
    %{buildroot}%{_bindir}/laprdus

# Install voice data
install -D -m 644 build/linux-x64-release/Josip.bin \
    %{buildroot}%{_datadir}/laprdus/Josip.bin
install -D -m 644 build/linux-x64-release/Vlado.bin \
    %{buildroot}%{_datadir}/laprdus/Vlado.bin

# Install dictionaries
install -D -m 644 data/dictionary/internal.json \
    %{buildroot}%{_datadir}/laprdus/internal.json
install -D -m 644 data/dictionary/spelling.json \
    %{buildroot}%{_datadir}/laprdus/spelling.json

# Install Speech Dispatcher module
install -D -m 755 build/linux-x64-release/sd_laprdus \
    %{buildroot}%{_libdir}/speech-dispatcher-modules/sd_laprdus
install -D -m 644 src/platform/linux/speechd/laprdus.conf \
    %{buildroot}%{_sysconfdir}/speech-dispatcher/modules/laprdus.conf

# Install development files
install -D -m 644 include/laprdus/laprdus_api.h \
    %{buildroot}%{_includedir}/laprdus/laprdus_api.h
install -D -m 644 include/laprdus/types.hpp \
    %{buildroot}%{_includedir}/laprdus/types.hpp
install -D -m 644 include/laprdus/laprdus.hpp \
    %{buildroot}%{_includedir}/laprdus/laprdus.hpp

%ldconfig_scriptlets

%post speechd
# Configure Speech Dispatcher for LaprdusTTS
SPEECHD_CONF="/etc/speech-dispatcher/speechd.conf"
MARKER_START="# BEGIN LAPRDUS TTS"
MARKER_END="# END LAPRDUS TTS"

if [ -f "$SPEECHD_CONF" ]; then
    # Check if already configured
    if ! grep -q "$MARKER_START" "$SPEECHD_CONF" 2>/dev/null; then
        # Check for existing manual configuration
        if ! grep -q 'AddModule.*"laprdus"' "$SPEECHD_CONF" 2>/dev/null; then
            echo "Configuring Speech Dispatcher for LaprdusTTS..."
            cat >> "$SPEECHD_CONF" << 'EOF'

# BEGIN LAPRDUS TTS
# LaprdusTTS - Croatian/Serbian Text-to-Speech
# Added automatically by package installation
AddModule "laprdus" "sd_laprdus" "laprdus.conf"

# Set LaprdusTTS as default for Croatian and Serbian
LanguageDefaultModule "hr" "laprdus"
LanguageDefaultModule "sr" "laprdus"
LanguageDefaultModule "hr-HR" "laprdus"
LanguageDefaultModule "sr-RS" "laprdus"
# END LAPRDUS TTS
EOF
            echo "LaprdusTTS configured successfully."
        fi
    fi
fi

# Restart speech-dispatcher if running
systemctl try-restart speech-dispatcher 2>/dev/null || true

%preun speechd
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

%postun speechd
# Restart speech-dispatcher after removal
if [ $1 -eq 0 ]; then
    systemctl try-restart speech-dispatcher 2>/dev/null || true
fi

%files
%license LICENSE
%doc README.md
%{_libdir}/liblaprdus.so.1*
%{_bindir}/laprdus
%{_datadir}/laprdus/

%files speechd
%{_libdir}/speech-dispatcher-modules/sd_laprdus
%config(noreplace) %{_sysconfdir}/speech-dispatcher/modules/laprdus.conf

%files devel
%{_libdir}/liblaprdus.so
%{_includedir}/laprdus/

%changelog
* Fri Jan 31 2025 Hrvoje Katic <hrvoje.katic@gmail.com> - 1.0.0-1
- Initial package
- Croatian and Serbian text-to-speech synthesis
- Speech Dispatcher integration for Orca screen reader
- Command-line utility
- Five voices: Josip, Vlado, Detence, Baba, Djedo
