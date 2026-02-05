# -*- coding: utf-8 -*-
# LaprdusTTS NVDA SynthDriver
# Croatian/Serbian text-to-speech synthesizer for NVDA
# Compatible with NVDA 2019.3 to 2025.3

import queue as Queue
import threading
from collections import OrderedDict

import addonHandler
import synthDriverHandler
from synthDriverHandler import synthIndexReached, synthDoneSpeaking, VoiceInfo
from logHandler import log
import time
import os as _os
import tempfile as _tempfile

# String types - Python 3 only (NVDA 2019.3+ requires Python 3)
string_types = (str,)

# Debug file logging - only enabled when debug flag file exists
# To enable: create an empty file named "laprdus_debug" in the temp directory
_DEBUG_ENABLED = _os.path.exists(_os.path.join(_tempfile.gettempdir(), "laprdus_debug"))
_DEBUG_LOG_PATH = _os.path.join(_tempfile.gettempdir(), "laprdus_debug.log") if _DEBUG_ENABLED else None

def _debug_log(msg):
    """Log debug message to file if debug mode is enabled."""
    if not _DEBUG_ENABLED or _DEBUG_LOG_PATH is None:
        return
    try:
        with open(_DEBUG_LOG_PATH, "a", encoding="utf-8") as f:
            f.write("[%s] %s\n" % (time.strftime("%H:%M:%S"), msg))
    except Exception:
        pass

# Initialize translation support for the addon
addonHandler.initTranslation()

# Import driver settings with fallback for different NVDA versions
try:
    from autoSettingsUtils.driverSetting import NumericDriverSetting, BooleanDriverSetting
    HAS_AUTO_SETTINGS = True
except ImportError:
    HAS_AUTO_SETTINGS = False

# Import speech commands with fallback for different NVDA versions
try:
    from speech.commands import IndexCommand, CharacterModeCommand, BreakCommand
except ImportError:
    try:
        from speech import IndexCommand, CharacterModeCommand, BreakCommand
    except ImportError:
        # Fallback for very old NVDA versions
        IndexCommand = None
        CharacterModeCommand = None
        BreakCommand = None

# =============================================================================
# Constants
# =============================================================================

# Audio format: 22050 Hz, 16-bit mono
SAMPLE_RATE = 22050
BITS_PER_SAMPLE = 16
CHANNELS = 1

# Rate boost configuration
RATE_MIN = 0.5          # Minimum rate (same for both modes)
RATE_NORMAL_MAX = 2.0   # Maximum rate without boost
RATE_BOOST_MAX = 4.0    # Maximum rate with boost enabled


def _nvda_to_rate_factor(nvda_rate, rate_boost=False):
    """Convert NVDA rate (0-100) to rate factor with 50=1.0x.

    Without boost: 0 → 0.5x, 50 → 1.0x, 100 → 2.0x
    With boost:    0 → 0.5x, 50 → 1.0x, 100 → 4.0x
    """
    max_rate = RATE_BOOST_MAX if rate_boost else RATE_NORMAL_MAX
    if nvda_rate <= 50:
        # 0-50 maps to 0.5-1.0
        return RATE_MIN + (nvda_rate / 50.0) * (1.0 - RATE_MIN)
    else:
        # 50-100 maps to 1.0-max_rate
        return 1.0 + ((nvda_rate - 50) / 50.0) * (max_rate - 1.0)


def _nvda_to_laprdus_pitch(nvda_pitch):
    """Convert NVDA pitch (0-100) to Laprdus user_pitch (0.5-2.0).

    0 → 0.5x, 50 → 1.0x (neutral), 100 → 2.0x
    This maps to user_pitch range which preserves formants.
    """
    if nvda_pitch <= 50:
        # 0-50 maps to 0.5-1.0
        return 0.5 + (nvda_pitch / 50.0) * 0.5
    else:
        # 50-100 maps to 1.0-2.0
        return 1.0 + ((nvda_pitch - 50) / 50.0)


def _nvda_to_laprdus_volume(nvda_volume):
    """Convert NVDA volume (0-100) to Laprdus volume (0.0-1.0)."""
    return nvda_volume / 100.0




# =============================================================================
# Custom Settings Factory Functions (only if autoSettingsUtils is available)
# =============================================================================

def _createCustomSettings():
    """Create custom settings if autoSettingsUtils is available.

    Note: All Laprdus-specific settings (pauses, number mode, inflection)
    are now managed via the shared settings.json file edited by laprdgui.exe.
    This function returns an empty list as no NVDA-specific settings are needed.
    """
    return []


# =============================================================================
# SynthDriver Implementation
# =============================================================================

class SynthDriver(synthDriverHandler.SynthDriver):
    """LaprdusTTS NVDA synthesizer driver."""

    # Driver identification
    name = "laprdus"
    description = "Laprdus"

    # Supported settings - MUST be a tuple
    # Base settings always available
    _baseSettings = [
        synthDriverHandler.SynthDriver.VoiceSetting(),
        synthDriverHandler.SynthDriver.RateSetting(),
        synthDriverHandler.SynthDriver.RateBoostSetting(),
        synthDriverHandler.SynthDriver.PitchSetting(),
        synthDriverHandler.SynthDriver.VolumeSetting(),
    ]
    # Add custom settings if autoSettingsUtils is available
    _baseSettings.extend(_createCustomSettings())
    supportedSettings = tuple(_baseSettings)

    # Supported speech commands - use set literal
    supportedCommands = {
        IndexCommand,
        CharacterModeCommand,
        BreakCommand,
    } - {None}  # Remove None if any imports failed

    # Supported notifications - MUST include both per NVDA docs
    supportedNotifications = {synthIndexReached, synthDoneSpeaking}

    @classmethod
    def check(cls):
        """Check if the driver is available."""
        # Temporarily simplified to isolate discovery issues
        return True

    def __init__(self):
        """Initialize the synthesizer."""
        super(SynthDriver, self).__init__()

        _debug_log("=== LaprdusTTS Initializing ===")
        log.debug("LaprdusTTS: Initializing...")

        # Import here to avoid issues during check()
        from . import _laprdus
        self._laprdus = _laprdus

        # Create the engine
        self._engine = _laprdus.LaprdusEngine()

        # Audio player - import nvwave here
        import nvwave
        self._nvwave = nvwave
        self._player = None

        # Threading
        self._speakQueue = Queue.Queue()
        self._speakThread = None
        self._isSpeaking = False
        self._shouldStop = threading.Event()
        self._spellingDone = threading.Event()  # Signaled when spelling item completes
        self._speakLock = threading.Lock()  # Protects _isSpeaking state
        self._playerLock = threading.Lock()  # Protects _player and _syncPlayer access

        # Voice settings (NVDA scale 0-100)
        self._rate = 50
        self._pitch = 50
        self._volume = 100
        self._rateBoost = False

        # Voice selection
        self._voice = "josip"  # Default voice
        self._availableVoices = None  # Cached voice dict

        # Current playback sample rate (for rate changes)
        self._currentSampleRate = SAMPLE_RATE

        # Character mode tracking
        self._characterMode = False

        # Initialize with default voice
        self._engine.set_voice(self._voice)

        # Load pronunciation dictionary
        self._engine.load_dictionary()

        # Load spelling dictionary for character mode
        self._engine.load_spelling_dictionary()

        # Apply initial settings
        self._applySettings()

        # Load shared settings from settings.json (written by laprdgui.exe)
        # This syncs number mode, pause settings, and inflection with the GUI
        self._loadSharedSettings()

        # Start background speech thread
        self._startSpeakThread()

        log.debug("LaprdusTTS: Initialized successfully")

    def _startSpeakThread(self):
        """Start the background speech thread."""
        self._shouldStop.clear()
        self._speakThread = threading.Thread(target=self._speakThreadFunc)
        self._speakThread.daemon = True
        self._speakThread.start()

    def _speakThreadFunc(self):
        """Background thread for speech processing."""
        while not self._shouldStop.is_set():
            try:
                item = self._speakQueue.get(timeout=0.1)
            except Queue.Empty:
                continue

            if item is None:
                # Poison pill - exit thread
                break

            # Support tuple formats: (text, index), (text, index, spelled), (text, index, spelled, isSpelling)
            if len(item) == 2:
                text, index = item
                useSpelledSynthesis = False
                isSpellingItem = False
            elif len(item) == 3:
                text, index, useSpelledSynthesis = item
                isSpellingItem = False
            else:
                text, index, useSpelledSynthesis, isSpellingItem = item
            self._synthesizeAndPlay(text, index, useSpelledSynthesis, isSpellingItem)

    def _synthesizeAndPlay(self, text, index, useSpelledSynthesis=False, isSpellingItem=False):
        """Synthesize and play audio in the background thread.

        IMPORTANT for Say All: The synthIndexReached notification must be sent
        BEFORE audio playback completes so NVDA can prepare the next segment.
        This allows continuous reading without gaps.
        """
        if self._shouldStop.is_set():
            if isSpellingItem:
                self._spellingDone.set()
            return

        self._isSpeaking = True

        # Check if text is a single character - always use spelled synthesis for single chars
        stripped_text = text.strip()
        is_single_char = len(stripped_text) == 1

        try:

            log.debug("LaprdusTTS: Synthesizing text='%s' spelled=%s single_char=%s" % (
                text[:50], useSpelledSynthesis, is_single_char))

            # Use spelled synthesis for single characters or when explicitly requested
            if is_single_char or useSpelledSynthesis:
                # For single characters, use the stripped text
                synth_text = stripped_text if is_single_char else text
                result = self._engine.synthesize_spelled(synth_text)
            else:
                result = self._engine.synthesize(text)

            if result is None:
                log.debug("LaprdusTTS: Synthesis returned None for: %s" % text[:50])
                # Still notify index reached if we have one
                if index is not None:
                    synthDriverHandler.synthIndexReached.notify(synth=self, index=index)
                # Check if queue is empty and notify done
                with self._speakLock:
                    if self._speakQueue.empty():
                        self._isSpeaking = False
                        do_notify = True
                    else:
                        do_notify = False
                if do_notify:
                    synthDriverHandler.synthDoneSpeaking.notify(synth=self)
                if isSpellingItem:
                    self._spellingDone.set()
                return

            samples, sample_rate, bits_per_sample, channels = result

            if self._shouldStop.is_set():
                if isSpellingItem:
                    self._spellingDone.set()
                return

            # Use actual sample rate from synthesis - rate control is handled by
            # Sonic library in the engine via set_speed(), not by playback sample rate
            # Only hold lock during player creation, not during playback (which blocks)
            with self._playerLock:
                if self._player is None or self._currentSampleRate != sample_rate:
                    if self._player is not None:
                        try:
                            self._player.close()
                        except Exception:
                            pass
                    self._player = self._nvwave.WavePlayer(
                        channels=channels,
                        samplesPerSec=sample_rate,
                        bitsPerSample=bits_per_sample,
                    )
                    self._currentSampleRate = sample_rate
                player = self._player

            # CRITICAL for Say All: Notify index BEFORE starting audio playback
            # This allows NVDA to prepare the next segment while current audio plays
            if index is not None:
                _debug_log("synthIndexReached notified BEFORE playback, index=%s" % index)
                synthDriverHandler.synthIndexReached.notify(synth=self, index=index)

            # Play audio (outside lock so cancel() can interrupt)
            if player:
                player.feed(samples)
                player.idle()

        except Exception as e:
            log.error("LaprdusTTS: Synthesis error: %s" % str(e))
        finally:
            # Signal spelling completion if this was a spelling item
            if isSpellingItem:
                self._spellingDone.set()

            # Only notify synthDoneSpeaking when the queue is empty
            # Use lock to prevent race condition with isSpeaking property
            with self._speakLock:
                if self._speakQueue.empty():
                    self._isSpeaking = False
                    do_notify_done = True
                else:
                    do_notify_done = False
            if do_notify_done:
                synthDriverHandler.synthDoneSpeaking.notify(synth=self)
                _debug_log("synthDoneSpeaking notified")

    # Known symbol names that NVDA sends during spelling mode (Croatian/Serbian)
    # These are multi-character strings that represent single symbols
    _SPELLING_SYMBOL_NAMES = frozenset([
        'razmak', 'točka', 'zarez', 'uskličnik', 'upitnik', 'točka zarez',
        'dvotočka', 'crtica', 'donja crtica', 'apostrof', 'navodnik',
        'otvorena zagrada', 'zatvorena zagrada', 'otvorena uglata zagrada',
        'zatvorena uglata zagrada', 'otvorena vitičasta zagrada',
        'zatvorena vitičasta zagrada', 'kosa crta', 'obrnuta kosa crta',
        'at', 'ljestve', 'dolar', 'posto', 'karet', 'i', 'zvjezdica',
        'plus', 'jednako', 'manje od', 'veće od', 'okomita crta', 'tilda',
        'akcent', 'nula', 'jedan', 'dva', 'tri', 'četiri', 'pet', 'šest',
        'sedam', 'osam', 'devet', 'blank', 'novi red', 'tabulator',
        # English variants NVDA might use
        'space', 'dot', 'comma', 'exclamation', 'question', 'semicolon',
        'colon', 'dash', 'underscore', 'apostrophe', 'quote', 'tab',
        'new line', 'newline',
    ])

    def speak(self, speechSequence):
        """
        Speak the given speech sequence.

        Args:
            speechSequence: List of text strings and speech commands
        """
        # Debug: Log the speech sequence to file
        _debug_log("speak() called with %d items, characterMode=%s" % (
            len(speechSequence), self._characterMode))
        for i, item in enumerate(speechSequence):
            if isinstance(item, string_types):
                _debug_log("  [%d] Text: '%s'" % (i, item[:50] if len(item) > 50 else item))
            elif CharacterModeCommand is not None and isinstance(item, CharacterModeCommand):
                _debug_log("  [%d] CharacterModeCommand(state=%s)" % (i, item.state))
            elif IndexCommand is not None and isinstance(item, IndexCommand):
                _debug_log("  [%d] IndexCommand(index=%s)" % (i, item.index))
            elif BreakCommand is not None and isinstance(item, BreakCommand):
                _debug_log("  [%d] BreakCommand" % i)
            else:
                _debug_log("  [%d] Unknown: %s" % (i, type(item).__name__))

        # FIRST: Check if this sequence contains CharacterModeCommand(True)
        # When NVDA spells text, it wraps characters in CharacterModeCommand.
        # We must process CharacterModeCommand BEFORE any early-return logic.
        has_char_mode = False
        for item in speechSequence:
            if CharacterModeCommand is not None and isinstance(item, CharacterModeCommand):
                if item.state:
                    has_char_mode = True
                self._characterMode = item.state
                _debug_log("CharacterModeCommand detected, state=%s" % item.state)

        # If CharacterModeCommand(True) is in the sequence, queue for async playback
        if has_char_mode:
            for item in speechSequence:
                if isinstance(item, string_types):
                    text = item.strip() if item.strip() else item  # Keep spaces
                    if text:
                        _debug_log("Queue spelling (charMode=True): '%s'" % text)
                        # Queue with spelled synthesis, background thread will notify when done
                        self._speakQueue.put((text, None, True, False))
                elif IndexCommand is not None and isinstance(item, IndexCommand):
                    pass
            # Don't notify synthDoneSpeaking here - let background thread do it
            return

        # Check if this is a spelling request (single char OR symbol name)
        # NVDA's spell review sends each character/symbol as a separate speak() call
        # Symbol names like "razmak" (space) must also go through spelling path
        text_items = [item for item in speechSequence if isinstance(item, string_types)]
        if len(text_items) == 1:
            text = text_items[0].strip()
            is_single_char = len(text) == 1
            is_symbol_name = text.lower() in self._SPELLING_SYMBOL_NAMES

            if is_single_char or is_symbol_name:
                # Find index
                idx = None
                for item in speechSequence:
                    if IndexCommand is not None and isinstance(item, IndexCommand):
                        idx = item.index
                        break

                _debug_log("Queue spelling (async): '%s' idx=%s single=%s symbol=%s" % (
                    text, idx, is_single_char, is_symbol_name))

                # Queue to background thread - use spelled synthesis only for single chars
                # Symbol names should use normal synthesis (we want to say "razmak" not spell it)
                use_spelled = is_single_char
                self._speakQueue.put((text, idx, use_spelled, True))
                return

        # Normal async path for multi-word speech
        # IMPORTANT for Say All: In NVDA's speech sequence, IndexCommand typically
        # comes BEFORE the text it marks. This allows NVDA to track which segment
        # is currently being spoken and request the next segment when needed.
        textParts = []
        pendingIndex = None  # Index that will be associated with the NEXT text

        for item in speechSequence:
            if isinstance(item, string_types):
                textParts.append(item)

            elif IndexCommand is not None and isinstance(item, IndexCommand):
                # Index command marks the NEXT text segment
                # Flush any accumulated text with the pending index
                if textParts:
                    text = " ".join(textParts)
                    if text.strip():
                        self._speakQueue.put((text, pendingIndex, False))
                    textParts = []
                # Store this index for the NEXT text segment
                pendingIndex = item.index
                _debug_log("IndexCommand: pendingIndex=%s" % pendingIndex)

            elif BreakCommand is not None and isinstance(item, BreakCommand):
                # Pause - add silence marker
                textParts.append(" ")

        # Flush remaining text with the last pending index
        if textParts:
            text = " ".join(textParts)
            if text.strip():
                self._speakQueue.put((text, pendingIndex, False))

    def cancel(self):
        """Cancel current speech."""
        _debug_log("cancel() called, queue size=%d" % self._speakQueue.qsize())

        # Signal spelling completion to unblock any waiting speak() call
        self._spellingDone.set()

        # Clear the queue
        while not self._speakQueue.empty():
            try:
                self._speakQueue.get_nowait()
            except Queue.Empty:
                break

        # Cancel engine synthesis
        if self._engine:
            self._engine.cancel()

        # Stop both players (with lock to prevent race with background thread)
        with self._playerLock:
            if self._player:
                self._player.stop()

            if hasattr(self, '_syncPlayer') and self._syncPlayer:
                self._syncPlayer.stop()

        with self._speakLock:
            self._isSpeaking = False
        # Reset character mode on cancel
        self._characterMode = False

    def speakCharacter(self, character):
        """
        Speak a single character using the spelling dictionary.

        This method is called by NVDA when spelling out text character by character.
        It uses the spelling dictionary to pronounce characters correctly in Croatian.

        Note: This method blocks until the character has been spoken, which is
        required by NVDA's spelling mode contract.

        Args:
            character: A single character string to speak
        """
        log.debug("LaprdusTTS: speakCharacter() called with '%s'" % character)

        # Clear the event before queuing
        self._spellingDone.clear()

        # Queue the character for spelled synthesis with isSpellingItem=True
        self._speakQueue.put((character, None, True, True))

        # Wait for completion (blocks as required by NVDA contract)
        self._spellingDone.wait(timeout=2.0)

    def pause(self, switch):
        """
        Pause or resume speech.

        Args:
            switch: True to pause, False to resume
        """
        with self._playerLock:
            if self._player:
                self._player.pause(switch)

    def terminate(self):
        """Clean up resources."""
        log.debug("LaprdusTTS: Terminating...")

        # Stop speech thread
        self._shouldStop.set()
        self._speakQueue.put(None)  # Poison pill

        if self._speakThread:
            self._speakThread.join(timeout=1.0)

        # Clean up players (with lock to prevent race with background thread)
        with self._playerLock:
            if self._player:
                try:
                    self._player.close()
                except Exception:
                    pass
                self._player = None

            if hasattr(self, '_syncPlayer') and self._syncPlayer:
                try:
                    self._syncPlayer.close()
                except Exception:
                    pass
                self._syncPlayer = None

        # Clean up engine
        if self._engine:
            self._engine.destroy()
            self._engine = None

        log.debug("LaprdusTTS: Terminated")

    def _applySettings(self):
        """Apply current voice settings to the engine."""
        if self._engine:
            # Apply volume to engine
            self._engine.set_volume(_nvda_to_laprdus_volume(self._volume))
            # Apply user pitch preference - formant-preserving (no chipmunk effect)
            # Voice character pitch (base_pitch) is handled by the voice selection
            self._engine.set_user_pitch(_nvda_to_laprdus_pitch(self._pitch))
            # Apply speed/rate to engine (Sonic time-stretching)
            # This changes speed WITHOUT affecting pitch
            # Rate boost expands max rate from 2.0x to 4.0x
            self._engine.set_speed(_nvda_to_rate_factor(self._rate, self._rateBoost))

    def _loadSharedSettings(self):
        """Load settings from shared settings.json (written by laprdgui.exe).

        This allows the NVDA addon to use the same configuration as SAPI5,
        providing a unified settings experience via the GUI configurator.

        Settings loaded:
        - numbers.mode: 'words' or 'digits'
        - pauses.sentence, pauses.comma, pauses.newline: pause durations in ms
        - speech.inflection: whether to use pitch variation
        """
        import json
        import os

        settings_path = os.path.join(
            os.environ.get('APPDATA', ''),
            'Laprdus',
            'settings.json'
        )

        if not os.path.exists(settings_path):
            _debug_log("_loadSharedSettings: settings.json not found at %s" % settings_path)
            return

        try:
            with open(settings_path, 'r', encoding='utf-8') as f:
                settings = json.load(f)

            _debug_log("_loadSharedSettings: Loaded settings from %s" % settings_path)

            # Apply number mode
            numbers = settings.get('numbers', {})
            mode = numbers.get('mode', 'words')
            number_mode = 1 if mode == 'digits' else 0
            self._engine.set_number_mode(number_mode)
            _debug_log("_loadSharedSettings: number_mode=%s (%d)" % (mode, number_mode))

            # Apply pause settings
            pauses = settings.get('pauses', {})
            sentence_pause = pauses.get('sentence', 100)
            comma_pause = pauses.get('comma', 100)
            newline_pause = pauses.get('newline', 100)
            self._engine.set_sentence_pause(sentence_pause)
            self._engine.set_comma_pause(comma_pause)
            self._engine.set_newline_pause(newline_pause)
            _debug_log("_loadSharedSettings: pauses sentence=%d comma=%d newline=%d" % (
                sentence_pause, comma_pause, newline_pause))

            # Apply inflection setting
            speech = settings.get('speech', {})
            inflection = speech.get('inflection', True)
            self._engine.set_inflection(inflection)
            _debug_log("_loadSharedSettings: inflection=%s" % inflection)

        except Exception as e:
            _debug_log("_loadSharedSettings: Error loading settings: %s" % str(e))
            log.warning("LaprdusTTS: Failed to load shared settings: %s" % str(e))

    # =========================================================================
    # isSpeaking property - NVDA queries this to check if synth is busy
    # =========================================================================

    @property
    def isSpeaking(self):
        """Return True if currently speaking."""
        with self._speakLock:
            return self._isSpeaking or not self._speakQueue.empty()

    # =========================================================================
    # Rate property
    # =========================================================================

    def _get_rate(self):
        return self._rate

    def _set_rate(self, value):
        self._rate = value
        # Apply rate to engine using Sonic-based time-stretching
        # This changes speed WITHOUT affecting pitch
        # Rate boost expands max rate from 2.0x to 4.0x
        if self._engine:
            self._engine.set_speed(_nvda_to_rate_factor(value, self._rateBoost))

    # =========================================================================
    # Rate Boost property
    # =========================================================================

    def _get_rateBoost(self):
        return self._rateBoost

    def _set_rateBoost(self, enable):
        if enable == self._rateBoost:
            return
        # Toggle boost and re-apply rate to use new range
        self._rateBoost = enable
        if self._engine:
            self._engine.set_speed(_nvda_to_rate_factor(self._rate, self._rateBoost))

    # =========================================================================
    # Pitch property
    # =========================================================================

    def _get_pitch(self):
        return self._pitch

    def _set_pitch(self, value):
        self._pitch = value
        # Apply user pitch preference - formant-preserving
        # This changes pitch WITHOUT chipmunk effect (voice character stays the same)
        # Voice character pitch is handled via base_pitch when voice is selected
        if self._engine:
            self._engine.set_user_pitch(_nvda_to_laprdus_pitch(value))

    # =========================================================================
    # Volume property
    # =========================================================================

    def _get_volume(self):
        return self._volume

    def _set_volume(self, value):
        self._volume = value
        if self._engine:
            self._engine.set_volume(_nvda_to_laprdus_volume(value))

    # =========================================================================
    # Voice property
    # =========================================================================

    def _get_voice(self):
        return self._voice

    def _set_voice(self, value):
        if value != self._voice:
            self._voice = value
            if self._engine:
                # Set the new voice - this may reload phoneme data
                self._engine.set_voice(value)
                # Reload pronunciation dictionary (in case it was cleared)
                self._engine.load_dictionary()
                # Reapply pitch since base pitch may have changed
                # Reapply user pitch preference (formant-preserving)
                self._engine.set_user_pitch(_nvda_to_laprdus_pitch(self._pitch))

    @property
    def availableVoices(self):
        """Return available voices for NVDA voice selection."""
        if self._availableVoices is None:
            self._availableVoices = self._buildVoiceDict()
        return self._availableVoices

    def _buildVoiceDict(self):
        """Build voice dictionary from engine."""
        voices = OrderedDict()
        try:
            count = self._laprdus.get_voice_count()
            for i in range(count):
                info = self._laprdus.get_voice_info(i)
                if info:
                    voice_id = info["id"]
                    display_name = info["display_name"]
                    language = info["language_code"]
                    voices[voice_id] = VoiceInfo(
                        voice_id,
                        display_name,
                        language
                    )
        except Exception as e:
            log.error("LaprdusTTS: Error building voice dict: %s" % str(e))
            # Fallback to single default voice
            voices["josip"] = VoiceInfo(
                "josip",
                "Laprdus Josip (Croatian)",
                "hr-HR"
            )
        return voices
