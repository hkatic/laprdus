// -*- coding: utf-8 -*-
// utf8_to_utf16.hpp - UTF-8 to UTF-16 for strings handed to Java.
//
// JNI's NewStringUTF expects Modified UTF-8 and may abort (CheckJNI) or
// produce garbage on a byte sequence it does not accept, such as the text
// of a user's entry echoed back by a parser. NewString takes UTF-16 code
// units and accepts anything; this conversion feeds it, replacing every
// malformed sequence with U+FFFD instead of failing.

#ifndef LAPRDUS_UTF8_TO_UTF16_HPP
#define LAPRDUS_UTF8_TO_UTF16_HPP

#include <cstdint>
#include <string>

namespace laprdus {

inline std::u16string utf8_to_utf16(const std::string& utf8) {
    std::u16string out;
    out.reserve(utf8.size());
    const auto* p = reinterpret_cast<const unsigned char*>(utf8.data());
    size_t n = utf8.size();
    size_t i = 0;
    while (i < n) {
        uint32_t cp;
        unsigned char b = p[i];
        size_t len;
        if (b < 0x80) { cp = b; len = 1; }
        else if ((b & 0xE0) == 0xC0) { cp = b & 0x1F; len = 2; }
        else if ((b & 0xF0) == 0xE0) { cp = b & 0x0F; len = 3; }
        else if ((b & 0xF8) == 0xF0) { cp = b & 0x07; len = 4; }
        else { out.push_back(0xFFFD); i++; continue; }

        bool ok = i + len <= n;
        for (size_t k = 1; ok && k < len; k++) {
            if ((p[i + k] & 0xC0) != 0x80) ok = false;
            else cp = (cp << 6) | (p[i + k] & 0x3F);
        }
        // Overlong forms, surrogates and code points past U+10FFFF are malformed
        if (ok && ((len == 2 && cp < 0x80) || (len == 3 && cp < 0x800) ||
                   (len == 4 && (cp < 0x10000 || cp > 0x10FFFF)) ||
                   (cp >= 0xD800 && cp <= 0xDFFF))) {
            ok = false;
        }
        if (!ok) {
            out.push_back(0xFFFD);
            i++;  // resynchronize on the next byte
            continue;
        }
        if (cp >= 0x10000) {
            cp -= 0x10000;
            out.push_back(static_cast<char16_t>(0xD800 + (cp >> 10)));
            out.push_back(static_cast<char16_t>(0xDC00 + (cp & 0x3FF)));
        } else {
            out.push_back(static_cast<char16_t>(cp));
        }
        i += len;
    }
    return out;
}

} // namespace laprdus

#endif // LAPRDUS_UTF8_TO_UTF16_HPP
