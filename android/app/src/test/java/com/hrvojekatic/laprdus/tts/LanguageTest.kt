package com.hrvojekatic.laprdus.tts

import org.junit.Assert.assertArrayEquals
import org.junit.Assert.assertEquals
import org.junit.Assert.assertNull
import org.junit.Test
import java.util.Locale

/** The language codes, tags and names the service and the settings exchange. */
class LanguageTest {

    @Test
    fun `tags and ISO3 codes of the four languages are recognized`() {
        assertEquals("hr", Language.fromTag("hr"))
        assertEquals("hr", Language.fromTag("hr-HR"))
        assertEquals("hr", Language.fromTag("hr_HR"))
        assertEquals("hr", Language.fromTag("hrv"))
        assertEquals("hr", Language.fromTag("HRV"))
        assertEquals("sr", Language.fromTag("sr"))
        assertEquals("sr", Language.fromTag("srp"))
        assertEquals("sr", Language.fromTag("sr-Latn-RS"))
        assertEquals("sr", Language.fromTag("sr-Cyrl"))
        assertEquals("bs", Language.fromTag("bs"))
        assertEquals("bs", Language.fromTag("bos"))
        assertEquals("bs", Language.fromTag("bs-BA"))
        assertEquals("sl", Language.fromTag("sl"))
        assertEquals("sl", Language.fromTag("slv"))
        assertEquals("sl", Language.fromTag("sl-SI"))
        assertEquals("sl", Language.fromTag("SL_si"))
    }

    @Test
    fun `language names are recognized`() {
        assertEquals("hr", Language.fromTag("Croatian"))
        assertEquals("hr", Language.fromTag("hrvatski"))
        assertEquals("sr", Language.fromTag("Serbian"))
        assertEquals("sr", Language.fromTag("srpski"))
        assertEquals("bs", Language.fromTag("Bosnian"))
        assertEquals("bs", Language.fromTag("bosanski"))
        assertEquals("sl", Language.fromTag("Slovenian"))
        assertEquals("sl", Language.fromTag("slovenščina"))
    }

    @Test
    fun `other languages are not`() {
        assertNull(Language.fromTag("en"))
        assertNull(Language.fromTag("eng"))
        assertNull(Language.fromTag("en-US"))
        assertNull(Language.fromTag("cs"))
        assertNull(Language.fromTag(""))
        assertNull(Language.fromTag(null))
        assertNull(Language.fromTag("h"))
    }

    @Test
    fun `codes map to tags, locales and ISO3 triples`() {
        assertEquals("hr-HR", Language.bcp47("hr"))
        assertEquals("sr-RS", Language.bcp47("sr"))
        assertEquals("bs-BA", Language.bcp47("bs"))
        assertEquals("sl-SI", Language.bcp47("sl"))
        assertEquals(Locale.forLanguageTag("sr-RS"), Language.locale("sr"))
        assertArrayEquals(arrayOf("hrv", "HRV", ""), Language.iso3("hr"))
        assertArrayEquals(arrayOf("srp", "SRB", ""), Language.iso3("sr"))
        assertArrayEquals(arrayOf("bos", "BIH", ""), Language.iso3("bs"))
        assertArrayEquals(arrayOf("slv", "SVN", ""), Language.iso3("sl"))
    }

    @Test
    fun `the system language is spoken unless the chosen one is forced`() {
        // A Croatian phone speaks Croatian whatever the setting says ...
        assertEquals("hr", Language.effective(false, "sl", Locale("hr", "HR")))
        assertEquals("sl", Language.effective(false, "hr", Locale("sl", "SI")))
        // ... unless the language is forced
        assertEquals("sl", Language.effective(true, "sl", Locale("hr", "HR")))
        // A phone in a language Laprdus does not speak gets the chosen one
        assertEquals("sl", Language.effective(false, "sl", Locale.US))
        assertEquals("hr", Language.effective(false, "xx", Locale.US))
        assertEquals("sl", Language.deviceLanguage(Locale("sl", "SI")))
        assertNull(Language.deviceLanguage(Locale.GERMANY))
    }

    @Test
    fun `the device language is the default when Laprdus speaks it`() {
        assertEquals("hr", Language.deviceDefault(Locale("hr", "HR")))
        assertEquals("sr", Language.deviceDefault(Locale("sr", "RS")))
        assertEquals("sr", Language.deviceDefault(Locale.forLanguageTag("sr-Latn-RS")))
        assertEquals("bs", Language.deviceDefault(Locale("bs", "BA")))
        assertEquals("sl", Language.deviceDefault(Locale("sl", "SI")))
        assertEquals("hr", Language.deviceDefault(Locale.US))
        assertEquals("hr", Language.deviceDefault(Locale.GERMANY))
    }
}
