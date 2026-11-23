#!/bin/bash
#
# test_package_install.sh - Test package installation and functionality
#
# This script tests an installed LaprdusTTS (a .deb, .rpm or Arch package,
# or the tarball's install.sh) by verifying:
# 1. The library is installed and known to the dynamic linker
# 2. The CLI is installed and synthesizes, in every language
# 3. The voice data and dictionaries are in place
# 4. The Speech Dispatcher module lies where Speech Dispatcher loads its
#    modules from, answers the module protocol with audio, and a running
#    Speech Dispatcher lists it and its voices
# 5. The development files are installed
#
# Usage:
#   ./test_package_install.sh [prefix]
#
# Default prefix: /usr/local (the tarball's); the packages install to /usr.
#
# With LAPRDUS_TEST_STRICT=1 (installers/linux/ci/test.sh sets it) a check
# that cannot be made counts as a failure, not as a skip: in the test
# containers everything is installed, so a skip there hides a fault.
#

set -e

PREFIX="${1:-/usr/local}"
STRICT="${LAPRDUS_TEST_STRICT:-0}"
PASSED=0
FAILED=0
SKIPPED=0
WORK="$(mktemp -d)"
trap 'rm -rf "${WORK}"' EXIT

# Color output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# (Not ((PASSED++)): it returns 1 while the count is 0, and "set -e" ended
# the script at the first result.)
pass() {
    echo -e "${GREEN}PASS${NC}: $1"
    PASSED=$((PASSED + 1))
}

fail() {
    echo -e "${RED}FAIL${NC}: $1"
    FAILED=$((FAILED + 1))
}

skip() {
    if [ "${STRICT}" = 1 ]; then
        fail "$1 (a skip is a failure with LAPRDUS_TEST_STRICT=1)"
    else
        echo -e "${YELLOW}SKIP${NC}: $1"
        SKIPPED=$((SKIPPED + 1))
    fi
}

# Runs a command with its error output kept; shows it when the command fails.
run() {
    if "$@" 2>"${WORK}/stderr"; then
        return 0
    fi
    sed 's/^/    | /' "${WORK}/stderr" | head -n 20
    return 1
}

# =============================================================================
# Library Installation Tests
# =============================================================================

echo "=========================================="
echo "Testing LaprdusTTS Package Installation"
echo "Prefix: ${PREFIX}"
echo "=========================================="
echo ""

echo "--- Library Tests ---"

# The library's folder: lib (Arch, the tarball), lib64 (Fedora) or Debian's
# multiarch folder (lib/x86_64-linux-gnu).
LIBRARY=""
for dir in "${PREFIX}/lib" "${PREFIX}/lib64" "${PREFIX}"/lib/*-linux-gnu*; do
    if [ -f "${dir}/liblaprdus.so.2" ] || [ -L "${dir}/liblaprdus.so.2" ]; then
        LIBRARY="${dir}/liblaprdus.so.2"
        break
    fi
done
if [ -n "${LIBRARY}" ]; then
    pass "Library exists: ${LIBRARY}"
else
    fail "Library liblaprdus.so.2 not found in ${PREFIX}/lib, lib64 or lib/<multiarch>"
fi

# The dynamic linker finds it by its soname (ldconfig is in /sbin, which is
# not in an ordinary user's PATH on Debian)
LDCONFIG="$(command -v ldconfig || echo /sbin/ldconfig)"
if "${LDCONFIG}" -p | grep -q 'liblaprdus\.so\.2 '; then
    pass "Library is registered with ldconfig"
else
    fail "Library is not in the dynamic linker's cache (ldconfig -p)"
fi

# =============================================================================
# CLI Tests
# =============================================================================

echo ""
echo "--- CLI Tests ---"

# Test CLI exists
if [ -x "${PREFIX}/bin/laprdus" ]; then
    pass "CLI executable exists"
else
    fail "CLI executable not found"
    exit 1  # Can't continue without CLI
fi

# Test CLI help
if "${PREFIX}/bin/laprdus" -h 2>&1 | grep -q "LaprdusTTS"; then
    pass "CLI help works"
else
    fail "CLI help failed"
fi

# Test CLI version
if "${PREFIX}/bin/laprdus" -h 2>&1 | grep -qi "version"; then
    pass "CLI displays version info"
else
    skip "CLI version info not found in help"
fi

# Test CLI lists voices (every voice with its native language, djedo by its new id)
VOICES="$("${PREFIX}/bin/laprdus" -l 2>&1 || true)"
if grep -q "^zvonko .*hr-HR" <<<"${VOICES}" && grep -q "^djedo .*sr-RS" <<<"${VOICES}" &&
    grep -q "^mirsad .*bs-BA" <<<"${VOICES}" && grep -q "Every voice speaks every language" <<<"${VOICES}"; then
    pass "CLI lists voices"
else
    fail "CLI failed to list voices"
fi

# =============================================================================
# Voice Data Tests
# =============================================================================

echo ""
echo "--- Voice Data Tests ---"

DATA_DIR="${PREFIX}/share/laprdus"

# Test voice data directory exists
if [ -d "${DATA_DIR}" ]; then
    pass "Voice data directory exists"
else
    fail "Voice data directory not found: ${DATA_DIR}"
fi

# Test Josip voice data
if [ -f "${DATA_DIR}/Josip.bin" ]; then
    pass "Josip voice data exists"

    # Check file size
    JOSIP_SIZE=$(stat -c%s "${DATA_DIR}/Josip.bin" 2>/dev/null || stat -f%z "${DATA_DIR}/Josip.bin" 2>/dev/null)
    if [ "${JOSIP_SIZE}" -gt 100000 ]; then
        pass "Josip voice data has valid size (${JOSIP_SIZE} bytes)"
    else
        fail "Josip voice data too small"
    fi
else
    fail "Josip voice data not found"
fi

# Test Vlado voice data
if [ -f "${DATA_DIR}/Vlado.bin" ]; then
    pass "Vlado voice data exists"
else
    fail "Vlado voice data not found"
fi

# Test dictionaries
for DICTIONARY in internal.json spelling.json emoji.json emoji_sl.json; do
    if [ -f "${DATA_DIR}/${DICTIONARY}" ]; then
        pass "Dictionary ${DICTIONARY} exists"
    else
        fail "Dictionary ${DICTIONARY} not found"
    fi
done

# =============================================================================
# Synthesis Tests
# =============================================================================

echo ""
echo "--- Synthesis Tests ---"

TEST_OUTPUT="${WORK}/test.wav"

# A WAV file written by the CLI, with more than its header.
synthesized() {
    local size
    [ -f "${TEST_OUTPUT}" ] || return 1
    size=$(stat -c%s "${TEST_OUTPUT}" 2>/dev/null || stat -f%z "${TEST_OUTPUT}" 2>/dev/null)
    rm -f "${TEST_OUTPUT}"
    [ "${size}" -gt 1000 ]
}

# Test basic synthesis
if run "${PREFIX}/bin/laprdus" -o "${TEST_OUTPUT}" "Test" && synthesized; then
    pass "Basic synthesis produces audio"
else
    fail "Basic synthesis failed"
fi

# Test the recorded voices
if run "${PREFIX}/bin/laprdus" -v josip -o "${TEST_OUTPUT}" "Dobar dan!" && synthesized; then
    pass "Croatian synthesis works (Josip)"
else
    fail "Croatian synthesis failed (Josip)"
fi

if run "${PREFIX}/bin/laprdus" -v vlado -o "${TEST_OUTPUT}" "Zdravo!" && synthesized; then
    pass "Serbian synthesis works (Vlado)"
else
    fail "Serbian synthesis failed (Vlado)"
fi

# Test the language option: every voice speaks every language
for LANG_CODE in hr sr bs sl; do
    if run "${PREFIX}/bin/laprdus" -v zvonko -L "${LANG_CODE}" -o "${TEST_OUTPUT}" "1000" && synthesized; then
        pass "Language ${LANG_CODE} works with -L"
    else
        fail "Language ${LANG_CODE} failed with -L"
    fi
done

# Test the renamed voice and its old id
if run "${PREFIX}/bin/laprdus" -v djedo -o "${TEST_OUTPUT}" "Zdravo!" && synthesized &&
    run "${PREFIX}/bin/laprdus" -v djed -o "${TEST_OUTPUT}" "Zdravo!" && synthesized; then
    pass "Voice djedo works by its new and old id"
else
    fail "Voice djedo failed"
fi

# Test rate parameter
if run "${PREFIX}/bin/laprdus" -r 1.5 -o "${TEST_OUTPUT}" "Brzo" && synthesized; then
    pass "Rate parameter works"
else
    fail "Rate parameter failed"
fi

# Test pitch parameter
if run "${PREFIX}/bin/laprdus" -p 1.5 -o "${TEST_OUTPUT}" "Visoko" && synthesized; then
    pass "Pitch parameter works"
else
    fail "Pitch parameter failed"
fi

# Test volume parameter
if run "${PREFIX}/bin/laprdus" -V 0.5 -o "${TEST_OUTPUT}" "Tiho" && synthesized; then
    pass "Volume parameter works"
else
    fail "Volume parameter failed"
fi

# =============================================================================
# Speech Dispatcher Tests
# =============================================================================

echo ""
echo "--- Speech Dispatcher Tests ---"

SPEECHD_CONF="/etc/speech-dispatcher/speechd.conf"
MODULE_CONF="/etc/speech-dispatcher/modules/laprdus.conf"

# The folder Speech Dispatcher loads its modules from: the one that holds its
# own (sd_dummy, sd_espeak-ng, ...). Debian and Ubuntu:
# /usr/lib/speech-dispatcher-modules; Fedora: /usr/lib64/...; Arch:
# /usr/lib/speech-dispatcher/speech-dispatcher-modules.
SPEECHD_MODULES=""
for dir in /usr/lib64/speech-dispatcher-modules /usr/lib/speech-dispatcher-modules \
    /usr/lib/speech-dispatcher/speech-dispatcher-modules /usr/lib/*/speech-dispatcher-modules \
    /usr/libexec/speech-dispatcher-modules; do
    for module in "${dir}"/sd_*; do
        if [ -x "${module}" ] && [ "${module##*/}" != sd_laprdus ]; then
            SPEECHD_MODULES="${dir}"
            break 2
        fi
    done
done

# Speaks a text through the module protocol, as Speech Dispatcher does:
# INIT, SPEAK, the text and a lone period, then QUIT once the module has
# answered 702 END (a QUIT sooner would stop the speech). The module's
# answers, with the audio, are left in ${WORK}/module.out.
speak_through_module() {
    local module="$1" fifo="${WORK}/module.in" out="${WORK}/module.out" pid
    rm -f "${fifo}" "${out}"
    mkfifo "${fifo}"
    "${module}" "${MODULE_CONF}" <"${fifo}" >"${out}" 2>"${WORK}/module.err" &
    pid=$!
    # A module that has died must fail the check, not end the script with
    # SIGPIPE at the next write.
    trap '' PIPE
    exec 3>"${fifo}"
    printf 'INIT\nSPEAK\nDobar dan, ovo je proba.\n.\n' >&3 2>/dev/null || true
    for _ in $(seq 1 120); do
        if grep -aq '^70[23] ' "${out}" || ! kill -0 "${pid}" 2>/dev/null; then
            break
        fi
        sleep 0.5
    done
    if kill -0 "${pid}" 2>/dev/null; then
        printf 'QUIT\n' >&3 2>/dev/null || true
    fi
    exec 3>&-
    trap - PIPE
    wait "${pid}" || true
}

if [ -z "${SPEECHD_MODULES}" ]; then
    skip "Speech Dispatcher's module folder not found (is speech-dispatcher installed?)"
elif [ -x "${SPEECHD_MODULES}/sd_laprdus" ]; then
    pass "Speech Dispatcher module installed where Speech Dispatcher loads modules: ${SPEECHD_MODULES}"
else
    fail "sd_laprdus is not in ${SPEECHD_MODULES}, where Speech Dispatcher loads its modules from"
fi

# Check module configuration
if [ -f "${MODULE_CONF}" ]; then
    pass "Module configuration installed"
else
    fail "Module configuration not found: ${MODULE_CONF}"
fi

# The packages' scripts and install.sh write a block into speechd.conf
if [ ! -f "${SPEECHD_CONF}" ]; then
    skip "${SPEECHD_CONF} not found (is speech-dispatcher installed?)"
elif grep -q '# BEGIN LAPRDUS TTS' "${SPEECHD_CONF}" && grep -q 'LanguageDefaultModule "sl" "laprdus"' "${SPEECHD_CONF}"; then
    pass "Speech Dispatcher configured: Laprdus for hr, sr, bs and sl"
else
    fail "No LAPRDUS TTS block for hr, sr, bs and sl in ${SPEECHD_CONF}"
fi

# The module itself, through the protocol: no server and no sound card needed
if [ -n "${SPEECHD_MODULES}" ] && [ -x "${SPEECHD_MODULES}/sd_laprdus" ]; then
    speak_through_module "${SPEECHD_MODULES}/sd_laprdus"
    if grep -aq '^299 OK LOADED SUCCESSFULLY' "${WORK}/module.out" &&
        grep -aq '^200 OK SPEAKING' "${WORK}/module.out" &&
        grep -aq '^705 AUDIO' "${WORK}/module.out" &&
        grep -aq '^702 END' "${WORK}/module.out"; then
        SAMPLES=$(grep -ao '705-num_samples=[0-9]*' "${WORK}/module.out" | cut -d= -f2 | awk '{n += $1} END {print n + 0}')
        pass "Module speaks through the module protocol (${SAMPLES} samples)"
    else
        fail "Module did not speak through the module protocol; its answers:"
        grep -a '^[0-9][0-9][0-9][ -]' "${WORK}/module.out" | grep -av '^705-' | head -n 10 | sed 's/^/    | /'
        sed 's/^/    | /' "${WORK}/module.err" | head -n 10
    fi
else
    skip "Module protocol not tested: sd_laprdus not found"
fi

# A running Speech Dispatcher loads the module and lists its voices
if command -v spd-say >/dev/null 2>&1; then
    pass "spd-say command available"

    # A server of this user's own is started by spd-say when none runs; it
    # needs a runtime folder, which a container or a cron job may not have.
    if [ -z "${XDG_RUNTIME_DIR:-}" ]; then
        export XDG_RUNTIME_DIR="${WORK}/runtime"
        mkdir -p "${XDG_RUNTIME_DIR}"
        chmod 700 "${XDG_RUNTIME_DIR}"
    fi
    # Into files, not $(...): a server spd-say starts could keep a pipe open,
    # and the test would wait for it to end.
    timeout 60 spd-say -O >"${WORK}/spd-modules" 2>&1 </dev/null || true
    if grep -qw "laprdus" "${WORK}/spd-modules"; then
        pass "Module registered with Speech Dispatcher (spd-say -O)"

        timeout 60 spd-say -o laprdus -L >"${WORK}/spd-voices" 2>&1 </dev/null || true
        if grep -qi "zvonko" "${WORK}/spd-voices"; then
            pass "Speech Dispatcher lists the module's voices (spd-say -o laprdus -L)"
        else
            fail "Speech Dispatcher did not list the module's voices:"
            head -n 10 "${WORK}/spd-voices" | sed 's/^/    | /'
        fi
    else
        fail "Module not listed by spd-say -O (speech-dispatcher may need a restart); it said:"
        head -n 10 "${WORK}/spd-modules" | sed 's/^/    | /'
    fi
else
    skip "spd-say not available"
fi

# =============================================================================
# Development Files Tests
# =============================================================================

echo ""
echo "--- Development Files Tests ---"

if [ -f "${PREFIX}/include/laprdus/laprdus_api.h" ]; then
    pass "API header installed"
else
    skip "API header not found (dev package may not be installed)"
fi

# The link a program is linked with (-llaprdus); the .so.2 is what it runs with
if [ -n "${LIBRARY}" ] && [ -L "$(dirname "${LIBRARY}")/liblaprdus.so" ]; then
    pass "Development link liblaprdus.so installed"
else
    skip "Development link liblaprdus.so not found (dev package may not be installed)"
fi

# =============================================================================
# Summary
# =============================================================================

echo ""
echo "=========================================="
echo "Test Summary"
echo "=========================================="
echo -e "${GREEN}Passed:${NC}  ${PASSED}"
echo -e "${RED}Failed:${NC}  ${FAILED}"
echo -e "${YELLOW}Skipped:${NC} ${SKIPPED}"
echo ""

if [ ${FAILED} -eq 0 ]; then
    echo -e "${GREEN}All tests passed!${NC}"
    exit 0
else
    echo -e "${RED}Some tests failed!${NC}"
    exit 1
fi
