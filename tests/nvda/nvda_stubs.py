"""Stand-ins for the NVDA modules the Laprdus synthesizer driver imports.

They let the driver in nvda-addon/addon/synthDrivers/laprdus run under a
plain Python without NVDA: the speech commands follow NVDA's
source/speech/commands.py, the audio player records what the driver feeds
it, and the notifications record their calls. Import this module before the
driver (see test_driver.py).
"""
import builtins
import sys
import types

builtins._ = lambda s: s

# --- addonHandler
addonHandler = types.ModuleType("addonHandler")
addonHandler.initTranslation = lambda: None
sys.modules["addonHandler"] = addonHandler


# --- logHandler
class _Log:
    def _p(self, lvl, msg, *a):
        print("[%s] %s" % (lvl, msg % a if a else msg))

    def debug(self, msg, *a):
        pass

    def info(self, msg, *a):
        self._p("INFO", msg, *a)

    def warning(self, msg, *a):
        self._p("WARN", msg, *a)

    def error(self, msg, *a, **k):
        self._p("ERROR", msg, *a)

    def debugWarning(self, msg, *a):
        self._p("DBGWARN", msg, *a)


logHandler = types.ModuleType("logHandler")
logHandler.log = _Log()
sys.modules["logHandler"] = logHandler

# --- speech.commands (after NVDA's source/speech/commands.py)
commands = types.ModuleType("speech.commands")


class SpeechCommand:
    pass


class SynthCommand(SpeechCommand):
    pass


class SynthParamCommand(SynthCommand):
    isDefault = False


class IndexCommand(SynthCommand):
    def __init__(self, index):
        self.index = index


class CharacterModeCommand(SynthParamCommand):
    def __init__(self, state):
        self.state = state
        self.isDefault = not state


class LangChangeCommand(SynthParamCommand):
    def __init__(self, lang):
        self.lang = lang
        self.isDefault = not lang


class BreakCommand(SynthCommand):
    def __init__(self, time=0):
        self.time = time


class EndUtteranceCommand(SpeechCommand):
    pass


# NVDA resolves a prosody command's newValue against the saved setting
CONFIG = {"pitch": 50, "rate": 50, "volume": 100}


class BaseProsodyCommand(SynthParamCommand):
    settingName = None

    def __init__(self, offset=0, multiplier=1):
        if offset != 0 and multiplier != 1:
            raise ValueError("offset and multiplier both specified")
        self._offset = offset
        self._multiplier = multiplier
        self.isDefault = offset == 0 and multiplier == 1

    @property
    def defaultValue(self):
        return CONFIG[self.settingName]

    @property
    def multiplier(self):
        if self._multiplier != 1:
            return self._multiplier
        if self._offset == 0:
            return 1
        d = self.defaultValue
        return float(d + self._offset) / d

    @property
    def offset(self):
        if self._offset != 0:
            return self._offset
        if self._multiplier == 1:
            return 0
        d = self.defaultValue
        return int(d * self._multiplier - d)

    @property
    def newValue(self):
        if self._offset != 0:
            return self.defaultValue + self._offset
        if self._multiplier != 1:
            return int(self.defaultValue * self._multiplier)
        return self.defaultValue

    def __repr__(self):
        return "%s(offset=%d, multiplier=%g)" % (type(self).__name__, self._offset, self._multiplier)


class PitchCommand(BaseProsodyCommand):
    settingName = "pitch"


class VolumeCommand(BaseProsodyCommand):
    settingName = "volume"


class RateCommand(BaseProsodyCommand):
    settingName = "rate"


for _name, _cls in list(globals().items()):
    if isinstance(_cls, type) and issubclass(_cls, SpeechCommand):
        setattr(commands, _name, _cls)
speech = types.ModuleType("speech")
speech.commands = commands
sys.modules["speech"] = speech
sys.modules["speech.commands"] = commands


# --- synthDriverHandler
class _Action:
    def __init__(self, name):
        self.name = name
        self.calls = []

    def notify(self, **kw):
        self.calls.append(kw)


synthIndexReached = _Action("synthIndexReached")
synthDoneSpeaking = _Action("synthDoneSpeaking")


class VoiceInfo:
    def __init__(self, id, displayName, language=None):
        self.id, self.displayName, self.language = id, displayName, language


class SynthDriver:
    @classmethod
    def VoiceSetting(cls):
        return "voice"

    @classmethod
    def LanguageSetting(cls):
        return "language"

    @classmethod
    def RateSetting(cls, minStep=1):
        return "rate"

    @classmethod
    def RateBoostSetting(cls):
        return "rateBoost"

    @classmethod
    def PitchSetting(cls, minStep=1):
        return "pitch"

    @classmethod
    def InflectionSetting(cls, minStep=1):
        return "inflection"

    @classmethod
    def VolumeSetting(cls, minStep=1):
        return "volume"

    def __init__(self):
        pass


sdh = types.ModuleType("synthDriverHandler")
sdh.SynthDriver = SynthDriver
sdh.VoiceInfo = VoiceInfo
sdh.synthIndexReached = synthIndexReached
sdh.synthDoneSpeaking = synthDoneSpeaking
sys.modules["synthDriverHandler"] = sdh

# --- autoSettingsUtils
ds = types.ModuleType("autoSettingsUtils.driverSetting")


class DriverSetting:
    def __init__(self, *a, **k):
        pass


class NumericDriverSetting(DriverSetting):
    pass


class BooleanDriverSetting(DriverSetting):
    pass


ds.DriverSetting = DriverSetting
ds.NumericDriverSetting = NumericDriverSetting
ds.BooleanDriverSetting = BooleanDriverSetting
au = types.ModuleType("autoSettingsUtils.utils")


class StringParameterInfo:
    def __init__(self, id, displayName):
        self.id, self.displayName = id, displayName


au.StringParameterInfo = StringParameterInfo
asu = types.ModuleType("autoSettingsUtils")
asu.driverSetting = ds
asu.utils = au
sys.modules["autoSettingsUtils"] = asu
sys.modules["autoSettingsUtils.driverSetting"] = ds
sys.modules["autoSettingsUtils.utils"] = au

# --- languageHandler
lh = types.ModuleType("languageHandler")
lh.getLanguage = lambda: "hr"
lh.getLanguageDescription = lambda code: None
sys.modules["languageHandler"] = lh

# --- nvwave: records everything the driver feeds, in order
FED = []


class WavePlayer:
    def __init__(self, channels, samplesPerSec, bitsPerSample, **kw):
        self.rate = samplesPerSec

    def feed(self, data, *a, **k):
        FED.append(bytes(data))

    def idle(self):
        pass

    def sync(self):
        pass

    def stop(self):
        pass

    def pause(self, switch):
        pass

    def close(self):
        pass


nvwave = types.ModuleType("nvwave")
nvwave.WavePlayer = WavePlayer
sys.modules["nvwave"] = nvwave
