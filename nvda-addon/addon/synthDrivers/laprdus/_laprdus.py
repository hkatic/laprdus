# -*- coding: utf-8 -*-
# _laprdus.py - ctypes bindings for LaprdusTTS DLL
# Python 2/3 compatible for NVDA 2019.3+

from __future__ import unicode_literals, print_function, division

import ctypes
import sys
import os

# Architecture detection
_is_64bit = sys.maxsize > 2**32

# Directory structure
_addon_dir = os.path.dirname(os.path.abspath(__file__))
_voices_dir = os.path.join(_addon_dir, "voices")
_dicts_dir = os.path.join(_addon_dir, "dictionaries")

# DLL path detection
_dll_name = "laprd64.dll" if _is_64bit else "laprd32.dll"
_dll_path = os.path.join(_addon_dir, _dll_name)
_phonemes_path = os.path.join(_voices_dir, "Josip.bin")  # Default voice

# Module-level library reference
_lib = None

# =============================================================================
# Error codes (matching laprdus_api.h)
# =============================================================================

LAPRDUS_OK = 0
LAPRDUS_ERROR_INVALID_HANDLE = -1
LAPRDUS_ERROR_NOT_INITIALIZED = -2
LAPRDUS_ERROR_INVALID_PATH = -3
LAPRDUS_ERROR_LOAD_FAILED = -4
LAPRDUS_ERROR_SYNTHESIS_FAILED = -5
LAPRDUS_ERROR_OUT_OF_MEMORY = -6
LAPRDUS_ERROR_CANCELLED = -7
LAPRDUS_ERROR_INVALID_PARAMETER = -8
LAPRDUS_ERROR_DECRYPTION_FAILED = -9
LAPRDUS_ERROR_FILE_NOT_FOUND = -10
LAPRDUS_ERROR_INVALID_FORMAT = -11

# =============================================================================
# ctypes Structure Definitions
# =============================================================================

class LaprdusAudioFormat(ctypes.Structure):
    """Audio format information structure."""
    _fields_ = [
        ("sample_rate", ctypes.c_uint32),
        ("bits_per_sample", ctypes.c_uint16),
        ("channels", ctypes.c_uint16),
    ]


class LaprdusVoiceInfo(ctypes.Structure):
    """Voice information structure."""
    _fields_ = [
        ("id", ctypes.c_char_p),
        ("display_name", ctypes.c_char_p),
        ("language_code", ctypes.c_char_p),
        ("language_lcid", ctypes.c_uint16),
        ("gender", ctypes.c_char_p),
        ("age", ctypes.c_char_p),
        ("base_pitch", ctypes.c_float),
        ("base_voice_id", ctypes.c_char_p),
        ("data_filename", ctypes.c_char_p),
    ]


# Type aliases
LaprdusHandle = ctypes.c_void_p
LaprdusStreamHandle = ctypes.c_void_p
LaprdusError = ctypes.c_int

# =============================================================================
# Library Loading
# =============================================================================

def _load_library():
    """Load the LaprdusTTS DLL."""
    global _lib
    if _lib is not None:
        return _lib

    if not os.path.exists(_dll_path):
        raise OSError("LaprdusTTS DLL not found: %s" % _dll_path)

    try:
        _lib = ctypes.windll.LoadLibrary(_dll_path)
    except OSError as e:
        raise OSError("Failed to load LaprdusTTS DLL: %s" % str(e))

    # Configure function signatures
    _configure_functions(_lib)

    return _lib

def _configure_functions(lib):
    """Configure ctypes function signatures."""

    # Lifecycle functions
    lib.laprdus_create.argtypes = []
    lib.laprdus_create.restype = LaprdusHandle

    lib.laprdus_destroy.argtypes = [LaprdusHandle]
    lib.laprdus_destroy.restype = None

    lib.laprdus_init_from_file.argtypes = [
        LaprdusHandle,
        ctypes.c_char_p,
        ctypes.c_void_p,
        ctypes.c_size_t
    ]
    lib.laprdus_init_from_file.restype = LaprdusError

    lib.laprdus_is_initialized.argtypes = [LaprdusHandle]
    lib.laprdus_is_initialized.restype = ctypes.c_int

    # Voice configuration
    lib.laprdus_set_speed.argtypes = [LaprdusHandle, ctypes.c_float]
    lib.laprdus_set_speed.restype = LaprdusError

    lib.laprdus_set_pitch.argtypes = [LaprdusHandle, ctypes.c_float]
    lib.laprdus_set_pitch.restype = LaprdusError

    lib.laprdus_set_user_pitch.argtypes = [LaprdusHandle, ctypes.c_float]
    lib.laprdus_set_user_pitch.restype = LaprdusError

    lib.laprdus_set_volume.argtypes = [LaprdusHandle, ctypes.c_float]
    lib.laprdus_set_volume.restype = LaprdusError

    lib.laprdus_set_inflection_enabled.argtypes = [LaprdusHandle, ctypes.c_int]
    lib.laprdus_set_inflection_enabled.restype = LaprdusError

    # Synthesis functions
    lib.laprdus_synthesize.argtypes = [
        LaprdusHandle,
        ctypes.c_char_p,
        ctypes.POINTER(ctypes.POINTER(ctypes.c_int16)),
        ctypes.POINTER(LaprdusAudioFormat)
    ]
    lib.laprdus_synthesize.restype = ctypes.c_int32

    lib.laprdus_free_buffer.argtypes = [ctypes.POINTER(ctypes.c_int16)]
    lib.laprdus_free_buffer.restype = None

    lib.laprdus_cancel.argtypes = [LaprdusHandle]
    lib.laprdus_cancel.restype = None

    # Utility functions
    lib.laprdus_get_error_message.argtypes = [LaprdusHandle]
    lib.laprdus_get_error_message.restype = ctypes.c_char_p

    lib.laprdus_get_version.argtypes = []
    lib.laprdus_get_version.restype = ctypes.c_char_p

    lib.laprdus_get_default_format.argtypes = [ctypes.POINTER(LaprdusAudioFormat)]
    lib.laprdus_get_default_format.restype = None

    lib.laprdus_error_to_string.argtypes = [LaprdusError]
    lib.laprdus_error_to_string.restype = ctypes.c_char_p

    # Voice selection functions
    lib.laprdus_get_voice_count.argtypes = []
    lib.laprdus_get_voice_count.restype = ctypes.c_uint32

    lib.laprdus_get_voice_info.argtypes = [
        ctypes.c_uint32,
        ctypes.POINTER(LaprdusVoiceInfo)
    ]
    lib.laprdus_get_voice_info.restype = LaprdusError

    lib.laprdus_get_voice_info_by_id.argtypes = [
        ctypes.c_char_p,
        ctypes.POINTER(LaprdusVoiceInfo)
    ]
    lib.laprdus_get_voice_info_by_id.restype = LaprdusError

    lib.laprdus_set_voice.argtypes = [
        LaprdusHandle,
        ctypes.c_char_p,
        ctypes.c_char_p
    ]
    lib.laprdus_set_voice.restype = LaprdusError

    lib.laprdus_get_current_voice.argtypes = [LaprdusHandle]
    lib.laprdus_get_current_voice.restype = ctypes.c_char_p

    # Dictionary functions
    lib.laprdus_load_dictionary.argtypes = [LaprdusHandle, ctypes.c_char_p]
    lib.laprdus_load_dictionary.restype = LaprdusError

    lib.laprdus_load_dictionary_from_memory.argtypes = [
        LaprdusHandle,
        ctypes.c_char_p,
        ctypes.c_size_t
    ]
    lib.laprdus_load_dictionary_from_memory.restype = LaprdusError

    lib.laprdus_clear_dictionary.argtypes = [LaprdusHandle]
    lib.laprdus_clear_dictionary.restype = None

    # Spelling dictionary functions
    lib.laprdus_load_spelling_dictionary.argtypes = [LaprdusHandle, ctypes.c_char_p]
    lib.laprdus_load_spelling_dictionary.restype = LaprdusError

    lib.laprdus_load_spelling_dictionary_from_memory.argtypes = [
        LaprdusHandle,
        ctypes.c_char_p,
        ctypes.c_size_t
    ]
    lib.laprdus_load_spelling_dictionary_from_memory.restype = LaprdusError

    lib.laprdus_clear_spelling_dictionary.argtypes = [LaprdusHandle]
    lib.laprdus_clear_spelling_dictionary.restype = None

    lib.laprdus_synthesize_spelled.argtypes = [
        LaprdusHandle,
        ctypes.c_char_p,
        ctypes.POINTER(ctypes.POINTER(ctypes.c_int16)),
        ctypes.POINTER(LaprdusAudioFormat)
    ]
    lib.laprdus_synthesize_spelled.restype = ctypes.c_int32

    # Emoji dictionary functions
    lib.laprdus_load_emoji_dictionary.argtypes = [LaprdusHandle, ctypes.c_char_p]
    lib.laprdus_load_emoji_dictionary.restype = LaprdusError

    lib.laprdus_load_emoji_dictionary_from_memory.argtypes = [
        LaprdusHandle,
        ctypes.c_char_p,
        ctypes.c_size_t
    ]
    lib.laprdus_load_emoji_dictionary_from_memory.restype = LaprdusError

    lib.laprdus_set_emoji_enabled.argtypes = [LaprdusHandle, ctypes.c_int]
    lib.laprdus_set_emoji_enabled.restype = LaprdusError

    lib.laprdus_is_emoji_enabled.argtypes = [LaprdusHandle]
    lib.laprdus_is_emoji_enabled.restype = ctypes.c_int

    lib.laprdus_clear_emoji_dictionary.argtypes = [LaprdusHandle]
    lib.laprdus_clear_emoji_dictionary.restype = None

    # Pause settings functions
    lib.laprdus_set_sentence_pause.argtypes = [LaprdusHandle, ctypes.c_uint32]
    lib.laprdus_set_sentence_pause.restype = LaprdusError

    lib.laprdus_set_comma_pause.argtypes = [LaprdusHandle, ctypes.c_uint32]
    lib.laprdus_set_comma_pause.restype = LaprdusError

    lib.laprdus_set_newline_pause.argtypes = [LaprdusHandle, ctypes.c_uint32]
    lib.laprdus_set_newline_pause.restype = LaprdusError

    lib.laprdus_get_sentence_pause.argtypes = [LaprdusHandle]
    lib.laprdus_get_sentence_pause.restype = ctypes.c_uint32

    lib.laprdus_get_comma_pause.argtypes = [LaprdusHandle]
    lib.laprdus_get_comma_pause.restype = ctypes.c_uint32

    lib.laprdus_get_newline_pause.argtypes = [LaprdusHandle]
    lib.laprdus_get_newline_pause.restype = ctypes.c_uint32

    lib.laprdus_set_spelling_pause.argtypes = [LaprdusHandle, ctypes.c_uint32]
    lib.laprdus_set_spelling_pause.restype = LaprdusError

    lib.laprdus_get_spelling_pause.argtypes = [LaprdusHandle]
    lib.laprdus_get_spelling_pause.restype = ctypes.c_uint32

    # Number mode functions
    lib.laprdus_set_number_mode.argtypes = [LaprdusHandle, ctypes.c_int]
    lib.laprdus_set_number_mode.restype = LaprdusError

    lib.laprdus_get_number_mode.argtypes = [LaprdusHandle]
    lib.laprdus_get_number_mode.restype = ctypes.c_int

# =============================================================================
# Public API Functions
# =============================================================================

def is_available():
    """Check if the LaprdusTTS library is available."""
    try:
        _load_library()
        return os.path.exists(_phonemes_path)
    except OSError:
        return False

def get_version():
    """Get the library version string."""
    lib = _load_library()
    version = lib.laprdus_get_version()
    if version:
        # Handle both Python 2 and 3
        if isinstance(version, bytes):
            return version.decode("utf-8")
        return version
    return "unknown"

def get_phonemes_path():
    """Get the path to the phonemes.bin file."""
    return _phonemes_path


def get_data_directory():
    """Get the directory containing voice data files."""
    return _voices_dir


def get_dictionaries_directory():
    """Get the directory containing dictionary files."""
    return _dicts_dir


def get_voice_count():
    """Get the number of available voices."""
    lib = _load_library()
    return lib.laprdus_get_voice_count()


def get_voice_info(index):
    """
    Get voice information by index.

    Args:
        index: Voice index (0 to count-1)

    Returns:
        dict with voice info, or None on failure
    """
    lib = _load_library()
    info = LaprdusVoiceInfo()
    result = lib.laprdus_get_voice_info(index, ctypes.byref(info))
    if result != LAPRDUS_OK:
        return None

    return _voice_info_to_dict(info)


def get_voice_info_by_id(voice_id):
    """
    Get voice information by ID.

    Args:
        voice_id: Voice ID string (e.g., "josip")

    Returns:
        dict with voice info, or None on failure
    """
    lib = _load_library()
    info = LaprdusVoiceInfo()

    if isinstance(voice_id, bytes):
        voice_id_bytes = voice_id
    else:
        voice_id_bytes = voice_id.encode("utf-8")

    result = lib.laprdus_get_voice_info_by_id(voice_id_bytes, ctypes.byref(info))
    if result != LAPRDUS_OK:
        return None

    return _voice_info_to_dict(info)


def _voice_info_to_dict(info):
    """Convert LaprdusVoiceInfo structure to Python dict."""
    def decode_str(s):
        if s is None:
            return None
        if isinstance(s, bytes):
            return s.decode("utf-8", errors="replace")
        return s

    return {
        "id": decode_str(info.id),
        "display_name": decode_str(info.display_name),
        "language_code": decode_str(info.language_code),
        "language_lcid": info.language_lcid,
        "gender": decode_str(info.gender),
        "age": decode_str(info.age),
        "base_pitch": info.base_pitch,
        "base_voice_id": decode_str(info.base_voice_id),
        "data_filename": decode_str(info.data_filename),
    }


# =============================================================================
# Engine Wrapper Class
# =============================================================================

class LaprdusEngine(object):
    """Python wrapper for the LaprdusTTS engine."""

    def __init__(self):
        """Create a new engine instance."""
        self._lib = _load_library()
        self._handle = self._lib.laprdus_create()
        if not self._handle:
            raise RuntimeError("Failed to create LaprdusTTS engine")
        self._initialized = False

    def initialize(self, phonemes_path=None):
        """Initialize the engine with phoneme data."""
        if phonemes_path is None:
            phonemes_path = _phonemes_path

        # Encode path for C API (Python 2/3 compatible)
        if isinstance(phonemes_path, bytes):
            path_bytes = phonemes_path
        else:
            path_bytes = phonemes_path.encode("utf-8")

        result = self._lib.laprdus_init_from_file(
            self._handle,
            path_bytes,
            None,  # No encryption key
            0
        )

        if result != LAPRDUS_OK:
            error_msg = self._get_error_message()
            raise RuntimeError("Failed to initialize engine: %s" % error_msg)

        self._initialized = True
        return True

    def is_initialized(self):
        """Check if the engine is initialized."""
        return self._initialized and bool(self._lib.laprdus_is_initialized(self._handle))

    def set_speed(self, speed):
        """Set speech speed (0.5 to 2.0, default 1.0)."""
        result = self._lib.laprdus_set_speed(self._handle, ctypes.c_float(speed))
        return result == LAPRDUS_OK

    def set_pitch(self, pitch):
        """Set voice character pitch (0.25 to 4.0, default 1.0).

        This shifts formants and changes voice character.
        Used for derived voices (child, grandma, grandpa).
        For user pitch preference, use set_user_pitch instead.
        """
        result = self._lib.laprdus_set_pitch(self._handle, ctypes.c_float(pitch))
        return result == LAPRDUS_OK

    def set_user_pitch(self, pitch):
        """Set user pitch preference (0.5 to 2.0, default 1.0).

        This preserves formants and keeps the voice character.
        Use this for user-controlled pitch adjustment (NVDA pitch slider).
        No chipmunk effect at higher pitches.
        """
        result = self._lib.laprdus_set_user_pitch(self._handle, ctypes.c_float(pitch))
        return result == LAPRDUS_OK

    def set_volume(self, volume):
        """Set volume (0.0 to 1.0, default 1.0)."""
        result = self._lib.laprdus_set_volume(self._handle, ctypes.c_float(volume))
        return result == LAPRDUS_OK

    def set_inflection(self, enabled):
        """Enable or disable punctuation-based inflection."""
        result = self._lib.laprdus_set_inflection_enabled(
            self._handle,
            1 if enabled else 0
        )
        return result == LAPRDUS_OK

    def synthesize(self, text):
        """
        Synthesize text to audio samples.

        Args:
            text: Text to synthesize (str or unicode)

        Returns:
            tuple: (samples as bytes, sample_rate, bits_per_sample, channels)
                   or None on failure
        """
        if not self._initialized:
            return None

        # Encode text for C API
        if isinstance(text, bytes):
            text_bytes = text
        else:
            text_bytes = text.encode("utf-8")

        # Prepare output parameters
        samples_ptr = ctypes.POINTER(ctypes.c_int16)()
        audio_format = LaprdusAudioFormat()

        # Call synthesis
        num_samples = self._lib.laprdus_synthesize(
            self._handle,
            text_bytes,
            ctypes.byref(samples_ptr),
            ctypes.byref(audio_format)
        )

        if num_samples <= 0:
            return None

        try:
            # Copy samples to Python bytes
            # Each sample is 2 bytes (int16)
            samples_bytes = ctypes.string_at(
                samples_ptr,
                num_samples * 2
            )

            return (
                samples_bytes,
                audio_format.sample_rate,
                audio_format.bits_per_sample,
                audio_format.channels
            )
        finally:
            # Free the C-allocated buffer
            self._lib.laprdus_free_buffer(samples_ptr)

    def cancel(self):
        """Cancel any ongoing synthesis."""
        self._lib.laprdus_cancel(self._handle)

    def set_voice(self, voice_id, data_directory=None):
        """
        Set the active voice.

        Args:
            voice_id: Voice ID string (e.g., "josip")
            data_directory: Directory containing voice .bin files (defaults to voices dir)

        Returns:
            True on success, False on failure
        """
        if data_directory is None:
            data_directory = _voices_dir

        if isinstance(voice_id, bytes):
            voice_id_bytes = voice_id
        else:
            voice_id_bytes = voice_id.encode("utf-8")

        if isinstance(data_directory, bytes):
            dir_bytes = data_directory
        else:
            dir_bytes = data_directory.encode("utf-8")

        result = self._lib.laprdus_set_voice(
            self._handle,
            voice_id_bytes,
            dir_bytes
        )

        if result == LAPRDUS_OK:
            self._initialized = True
            return True
        return False

    def get_current_voice(self):
        """
        Get the currently active voice ID.

        Returns:
            Voice ID string, or None if no voice is set
        """
        result = self._lib.laprdus_get_current_voice(self._handle)
        if result:
            if isinstance(result, bytes):
                return result.decode("utf-8", errors="replace")
            return result
        return None

    def load_dictionary(self, dictionary_path=None):
        """
        Load a pronunciation dictionary from a JSON file.

        Args:
            dictionary_path: Path to dictionary file (defaults to dictionaries/internal.json)

        Returns:
            True on success, False on failure
        """
        if dictionary_path is None:
            dictionary_path = os.path.join(_dicts_dir, "internal.json")

        if not os.path.exists(dictionary_path):
            return False

        if isinstance(dictionary_path, bytes):
            path_bytes = dictionary_path
        else:
            path_bytes = dictionary_path.encode("utf-8")

        result = self._lib.laprdus_load_dictionary(self._handle, path_bytes)
        return result == LAPRDUS_OK

    def clear_dictionary(self):
        """Clear all entries from the pronunciation dictionary."""
        self._lib.laprdus_clear_dictionary(self._handle)

    def load_spelling_dictionary(self, dictionary_path=None):
        """
        Load a spelling dictionary from a JSON file.

        Args:
            dictionary_path: Path to spelling dictionary file (defaults to dictionaries/spelling.json)

        Returns:
            True on success, False on failure
        """
        if dictionary_path is None:
            dictionary_path = os.path.join(_dicts_dir, "spelling.json")

        if not os.path.exists(dictionary_path):
            return False

        if isinstance(dictionary_path, bytes):
            path_bytes = dictionary_path
        else:
            path_bytes = dictionary_path.encode("utf-8")

        result = self._lib.laprdus_load_spelling_dictionary(self._handle, path_bytes)
        return result == LAPRDUS_OK

    def clear_spelling_dictionary(self):
        """Clear all entries from the spelling dictionary."""
        self._lib.laprdus_clear_spelling_dictionary(self._handle)

    def synthesize_spelled(self, text):
        """
        Synthesize text in spelling mode (character by character).

        Args:
            text: Text to spell (str or unicode)

        Returns:
            tuple: (samples as bytes, sample_rate, bits_per_sample, channels)
                   or None on failure
        """
        if not self._initialized:
            return None

        # Encode text for C API
        if isinstance(text, bytes):
            text_bytes = text
        else:
            text_bytes = text.encode("utf-8")

        # Prepare output parameters
        samples_ptr = ctypes.POINTER(ctypes.c_int16)()
        audio_format = LaprdusAudioFormat()

        # Call spelling synthesis
        num_samples = self._lib.laprdus_synthesize_spelled(
            self._handle,
            text_bytes,
            ctypes.byref(samples_ptr),
            ctypes.byref(audio_format)
        )

        if num_samples <= 0:
            return None

        try:
            # Copy samples to Python bytes
            samples_bytes = ctypes.string_at(
                samples_ptr,
                num_samples * 2
            )

            return (
                samples_bytes,
                audio_format.sample_rate,
                audio_format.bits_per_sample,
                audio_format.channels
            )
        finally:
            # Free the C-allocated buffer
            self._lib.laprdus_free_buffer(samples_ptr)

    def _get_error_message(self):
        """Get the last error message."""
        msg = self._lib.laprdus_get_error_message(self._handle)
        if msg:
            if isinstance(msg, bytes):
                return msg.decode("utf-8", errors="replace")
            return msg
        return "Unknown error"

    # =========================================================================
    # Emoji Dictionary Methods
    # =========================================================================

    def load_emoji_dictionary(self, dictionary_path=None):
        """
        Load an emoji dictionary from a JSON file.

        Args:
            dictionary_path: Path to emoji dictionary file (defaults to dictionaries/emoji.json)

        Returns:
            True on success, False on failure
        """
        if dictionary_path is None:
            dictionary_path = os.path.join(_dicts_dir, "emoji.json")

        if not os.path.exists(dictionary_path):
            return False

        if isinstance(dictionary_path, bytes):
            path_bytes = dictionary_path
        else:
            path_bytes = dictionary_path.encode("utf-8")

        result = self._lib.laprdus_load_emoji_dictionary(self._handle, path_bytes)
        return result == LAPRDUS_OK

    def set_emoji_enabled(self, enabled):
        """Enable or disable emoji processing.

        When enabled, emojis are converted to their text descriptions.
        Disabled by default.
        """
        result = self._lib.laprdus_set_emoji_enabled(
            self._handle,
            1 if enabled else 0
        )
        return result == LAPRDUS_OK

    def is_emoji_enabled(self):
        """Check if emoji processing is enabled."""
        return bool(self._lib.laprdus_is_emoji_enabled(self._handle))

    def clear_emoji_dictionary(self):
        """Clear all entries from the emoji dictionary."""
        self._lib.laprdus_clear_emoji_dictionary(self._handle)

    # =========================================================================
    # Pause Settings Methods
    # =========================================================================

    def set_sentence_pause(self, pause_ms):
        """Set pause duration after sentence-ending punctuation (. ! ?).

        Args:
            pause_ms: Pause duration in milliseconds (0-2000)

        Returns:
            True on success, False on failure
        """
        result = self._lib.laprdus_set_sentence_pause(
            self._handle,
            ctypes.c_uint32(pause_ms)
        )
        return result == LAPRDUS_OK

    def get_sentence_pause(self):
        """Get the sentence pause duration in milliseconds."""
        return self._lib.laprdus_get_sentence_pause(self._handle)

    def set_comma_pause(self, pause_ms):
        """Set pause duration after commas.

        Args:
            pause_ms: Pause duration in milliseconds (0-2000)

        Returns:
            True on success, False on failure
        """
        result = self._lib.laprdus_set_comma_pause(
            self._handle,
            ctypes.c_uint32(pause_ms)
        )
        return result == LAPRDUS_OK

    def get_comma_pause(self):
        """Get the comma pause duration in milliseconds."""
        return self._lib.laprdus_get_comma_pause(self._handle)

    def set_newline_pause(self, pause_ms):
        """Set pause duration at line breaks.

        Args:
            pause_ms: Pause duration in milliseconds (0-2000)

        Returns:
            True on success, False on failure
        """
        result = self._lib.laprdus_set_newline_pause(
            self._handle,
            ctypes.c_uint32(pause_ms)
        )
        return result == LAPRDUS_OK

    def get_newline_pause(self):
        """Get the newline pause duration in milliseconds."""
        return self._lib.laprdus_get_newline_pause(self._handle)

    def set_spelling_pause(self, pause_ms):
        """Set pause duration between spelled characters.

        Args:
            pause_ms: Pause duration in milliseconds (0-2000)

        Returns:
            True on success, False on failure
        """
        result = self._lib.laprdus_set_spelling_pause(
            self._handle,
            ctypes.c_uint32(pause_ms)
        )
        return result == LAPRDUS_OK

    def get_spelling_pause(self):
        """Get the spelling pause duration in milliseconds."""
        return self._lib.laprdus_get_spelling_pause(self._handle)

    # =========================================================================
    # Number Mode Methods
    # =========================================================================

    def set_number_mode(self, mode):
        """Set number processing mode.

        Args:
            mode: 0 for whole numbers (default), 1 for digit-by-digit

        Returns:
            True on success, False on failure
        """
        result = self._lib.laprdus_set_number_mode(self._handle, ctypes.c_int(mode))
        return result == LAPRDUS_OK

    def get_number_mode(self):
        """Get the current number processing mode.

        Returns:
            0 for whole numbers, 1 for digit-by-digit
        """
        return self._lib.laprdus_get_number_mode(self._handle)

    def destroy(self):
        """Destroy the engine and free resources."""
        if self._handle:
            self._lib.laprdus_destroy(self._handle)
            self._handle = None
            self._initialized = False

    def __del__(self):
        """Destructor to ensure cleanup."""
        self.destroy()
