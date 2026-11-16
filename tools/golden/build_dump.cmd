@echo off
rem Builds tools\golden\dump.cpp with the flags of the release DLL.
rem Run from the repository root; writes build\golden\dump.exe.
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul || exit /b 1
if not exist build\golden mkdir build\golden
cl /nologo /W3 /EHsc /std:c++17 /utf-8 /permissive- /O2 /Oi /GL /MD /DNDEBUG /Iinclude /Isrc ^
  /Fobuild\golden\ /Febuild\golden\dump.exe tools\golden\dump.cpp ^
  src\core\phoneme_mapper.cpp src\core\croatian_numbers.cpp src\formant\klatt_synth.cpp ^
  src\formant\formant_phonemes.cpp src\formant\formant_lexicon.cpp src\formant\formant_frontend.cpp ^
  src\formant\formant_slovenian.cpp src\formant\formant_intonation.cpp src\formant\formant_synthesizer.cpp ^
  /link /LTCG
