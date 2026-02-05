#!/bin/bash
#
# test_package_install.sh - Test package installation and functionality
#
# This script tests the LaprdusTTS Linux package installation by verifying:
# 1. Library is installed correctly
# 2. CLI is installed and functional
# 3. Voice data is accessible
# 4. Speech Dispatcher module works (if installed)
#
# Usage:
#   ./test_package_install.sh [prefix]
#
# Default prefix: /usr/local
#

set -e

PREFIX="${1:-/usr/local}"
PASSED=0
FAILED=0
SKIPPED=0

# Color output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

pass() {
    echo -e "${GREEN}PASS${NC}: $1"
    ((PASSED++))
}

fail() {
    echo -e "${RED}FAIL${NC}: $1"
    ((FAILED++))
}

skip() {
    echo -e "${YELLOW}SKIP${NC}: $1"
    ((SKIPPED++))
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

# Test library exists
if [ -f "${PREFIX}/lib/liblaprdus.so" ] || [ -f "${PREFIX}/lib/liblaprdus.so.1" ]; then
    pass "Library file exists"
else
    fail "Library file not found in ${PREFIX}/lib/"
fi

# Test library is loadable
if ldconfig -p | grep -q liblaprdus; then
    pass "Library is registered with ldconfig"
else
    # Try to load manually
    if LD_LIBRARY_PATH="${PREFIX}/lib" ldd "${PREFIX}/bin/laprdus" 2>/dev/null | grep -q liblaprdus; then
        pass "Library is loadable (manual path)"
    else
        fail "Library is not loadable"
    fi
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

# Test CLI lists voices
if "${PREFIX}/bin/laprdus" -l 2>&1 | grep -q "josip"; then
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
if [ -f "${DATA_DIR}/internal.json" ]; then
    pass "Internal dictionary exists"
else
    fail "Internal dictionary not found"
fi

if [ -f "${DATA_DIR}/spelling.json" ]; then
    pass "Spelling dictionary exists"
else
    fail "Spelling dictionary not found"
fi

# =============================================================================
# Synthesis Tests
# =============================================================================

echo ""
echo "--- Synthesis Tests ---"

TEST_OUTPUT="/tmp/laprdus_test_$$.wav"

# Test basic synthesis
if "${PREFIX}/bin/laprdus" -o "${TEST_OUTPUT}" "Test" 2>/dev/null; then
    if [ -f "${TEST_OUTPUT}" ]; then
        FILE_SIZE=$(stat -c%s "${TEST_OUTPUT}" 2>/dev/null || stat -f%z "${TEST_OUTPUT}" 2>/dev/null)
        if [ "${FILE_SIZE}" -gt 100 ]; then
            pass "Basic synthesis produces audio"
        else
            fail "Synthesis output too small"
        fi
        rm -f "${TEST_OUTPUT}"
    else
        fail "Synthesis output file not created"
    fi
else
    fail "Basic synthesis failed"
fi

# Test Croatian synthesis
if "${PREFIX}/bin/laprdus" -v josip -o "${TEST_OUTPUT}" "Dobar dan!" 2>/dev/null; then
    if [ -f "${TEST_OUTPUT}" ]; then
        pass "Croatian synthesis works"
        rm -f "${TEST_OUTPUT}"
    else
        fail "Croatian synthesis output not created"
    fi
else
    fail "Croatian synthesis failed"
fi

# Test Serbian synthesis
if "${PREFIX}/bin/laprdus" -v vlado -o "${TEST_OUTPUT}" "Zdravo!" 2>/dev/null; then
    if [ -f "${TEST_OUTPUT}" ]; then
        pass "Serbian synthesis works"
        rm -f "${TEST_OUTPUT}"
    else
        fail "Serbian synthesis output not created"
    fi
else
    fail "Serbian synthesis failed"
fi

# Test rate parameter
if "${PREFIX}/bin/laprdus" -r 1.5 -o "${TEST_OUTPUT}" "Brzo" 2>/dev/null; then
    pass "Rate parameter works"
    rm -f "${TEST_OUTPUT}"
else
    fail "Rate parameter failed"
fi

# Test pitch parameter
if "${PREFIX}/bin/laprdus" -p 1.5 -o "${TEST_OUTPUT}" "Visoko" 2>/dev/null; then
    pass "Pitch parameter works"
    rm -f "${TEST_OUTPUT}"
else
    fail "Pitch parameter failed"
fi

# Test volume parameter
if "${PREFIX}/bin/laprdus" -V 0.5 -o "${TEST_OUTPUT}" "Tiho" 2>/dev/null; then
    pass "Volume parameter works"
    rm -f "${TEST_OUTPUT}"
else
    fail "Volume parameter failed"
fi

# =============================================================================
# Speech Dispatcher Tests
# =============================================================================

echo ""
echo "--- Speech Dispatcher Tests ---"

# Check if speech-dispatcher is running
if command -v spd-say >/dev/null 2>&1; then
    pass "spd-say command available"

    # Check if laprdus module is installed
    if [ -f "/usr/lib/speech-dispatcher-modules/sd_laprdus" ] || \
       [ -f "${PREFIX}/lib/speech-dispatcher-modules/sd_laprdus" ]; then
        pass "Speech Dispatcher module installed"

        # Check module configuration
        if [ -f "/etc/speech-dispatcher/modules/laprdus.conf" ]; then
            pass "Module configuration installed"
        else
            skip "Module configuration not in /etc"
        fi

        # Try to list modules (may require speechd running)
        if spd-say -L 2>&1 | grep -q "laprdus"; then
            pass "Module registered with Speech Dispatcher"

            # Try synthesis with module
            if timeout 5 spd-say -o laprdus -w "Test" 2>/dev/null; then
                pass "Speech Dispatcher synthesis works"
            else
                skip "Speech Dispatcher synthesis timed out or failed"
            fi
        else
            skip "Module not listed (speech-dispatcher may need restart)"
        fi
    else
        skip "Speech Dispatcher module not installed"
    fi
else
    skip "spd-say not available"
fi

# =============================================================================
# Headers Test (for development package)
# =============================================================================

echo ""
echo "--- Development Files Tests ---"

if [ -f "${PREFIX}/include/laprdus/laprdus_api.h" ]; then
    pass "API header installed"
else
    skip "API header not found (dev package may not be installed)"
fi

if [ -f "${PREFIX}/include/laprdus/types.hpp" ]; then
    pass "Types header installed"
else
    skip "Types header not found"
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
