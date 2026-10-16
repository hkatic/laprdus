package com.hrvojekatic.laprdus.tts

import org.junit.Assert.assertArrayEquals
import org.junit.Assert.assertEquals
import org.junit.Assert.assertNull
import org.junit.Test
import java.util.Locale

/** The language codes, tags and names the service and the settings exchange. */
class LanguageTest {

    @Test
    fun `tags and ISO3 codes of the three languages are recognized`() {
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
    }

    @Test
    fun `language names are recognized`() {
        assertEquals("hr", Language.fromTag("Croatian"))
        assertEquals("hr", Language.fromTag("hrvatski"))
        assertEquals("sr", Language.fromTag("Serbian"))
        assertEquals("sr", Language.fromTag("srpski"))
        assertEquals("bs", Language.fromTag("Bosnian"))
        assertEquals("bs", Language.fromTag("bosanski"))
    }

    @Test
    fun `other languages are not`() {
        assertNull(Language.fromTag("en"))
        assertNull(Language.fromTag("eng"))
        assertNull(Language.fromTag("en-US"))
        assertNull(Language.fromTag("sl"))
        assertNull(Language.fromTag(""))
        assertNull(Language.fromTag(null))
        assertNull(Language.fromTag("h"))
    }

    @Test
    fun `codes map to tags, locales and ISO3 triples`() {
        assertEquals("hr-HR", Language.bcp47("hr"))
        assertEquals("sr-RS", Language.bcp47("sr"))
        assertEquals("bs-BA", Language.bcp47("bs"))
        assertEquals(Locale.forLanguageTag("sr-RS"), Language.locale("sr"))
        assertArrayEquals(arrayOf("hrv", "HRV", ""), Language.iso3("hr"))
        assertArrayEquals(arrayOf("srp", "SRB", ""), Language.iso3("sr"))
        assertArrayEquals(arrayOf("bos", "BIH", ""), Language.iso3("bs"))
    }

    @Test
    fun `the device language is the default when Laprdus speaks it`() {
        assertEquals("hr", Language.deviceDefault(Locale("hr", "HR")))
        assertEquals("sr", Language.deviceDefault(Locale("sr", "RS")))
        assertEquals("sr", Language.deviceDefault(Locale.forLanguageTag("sr-Latn-RS")))
        assertEquals("bs", Language.deviceDefault(Locale("bs", "BA")))
        assertEquals("hr", Language.deviceDefault(Locale.US))
        assertEquals("hr", Language.deviceDefault(Locale.GERMANY))
    }
}
