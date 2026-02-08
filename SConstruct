# -*- coding: utf-8 -*-
# SConstruct - LaprdusTTS Build Configuration
# Cross-platform TTS engine build system

import os
import sys
from SCons.Script import *

# =============================================================================
# Version Information
# =============================================================================

VERSION_MAJOR = 1
VERSION_MINOR = 0
VERSION_PATCH = 0
VERSION_STRING = f"{VERSION_MAJOR}.{VERSION_MINOR}.{VERSION_PATCH}"

# =============================================================================
# Command Line Options
# =============================================================================

AddOption('--platform',
          dest='platform',
          type='choice',
          choices=['auto', 'windows', 'linux', 'android'],
          default='auto',
          help='Target platform (auto, windows, linux, android)')

AddOption('--arch',
          dest='arch',
          type='choice',
          choices=['x64', 'x86', 'arm64', 'arm'],
          default='x64',
          help='Target architecture')

AddOption('--build-config',
          dest='build_config',
          type='choice',
          choices=['debug', 'release', 'relwithdebinfo'],
          default='release',
          help='Build configuration')

AddOption('--android-ndk',
          dest='android_ndk',
          type='string',
          default=os.environ.get('ANDROID_NDK_HOME', ''),
          help='Path to Android NDK')

AddOption('--android-api',
          dest='android_api',
          type='int',
          default=21,
          help='Android API level')

AddOption('--enable-encryption',
          dest='enable_encryption',
          action='store_true',
          default=False,
          help='Enable phoneme data encryption')

AddOption('--phoneme-key',
          dest='phoneme_key',
          type='string',
          default='',
          help='Encryption key (64 hex chars)')

AddOption('--prefix',
          dest='prefix',
          type='string',
          default='/usr/local',
          help='Installation prefix (Linux only)')

# =============================================================================
# Platform Detection
# =============================================================================

def detect_platform():
    if sys.platform.startswith('win'):
        return 'windows'
    elif sys.platform.startswith('linux'):
        return 'linux'
    elif sys.platform == 'darwin':
        return 'macos'
    return 'linux'

target_platform = GetOption('platform')
if target_platform == 'auto':
    target_platform = detect_platform()

build_config = GetOption('build_config')
arch = GetOption('arch')

print(f"Building LaprdusTTS {VERSION_STRING}")
print(f"  Platform: {target_platform}")
print(f"  Architecture: {arch}")
print(f"  Configuration: {build_config}")

# =============================================================================
# Build Directories
# =============================================================================

build_dir = f'build/{target_platform}-{arch}-{build_config}'
dist_dir = f'dist/{target_platform}-{arch}'

# Normalize paths for the current OS
if sys.platform.startswith('win'):
    build_dir = build_dir.replace('/', '\\')
    dist_dir = dist_dir.replace('/', '\\')

# =============================================================================
# Environment Setup
# =============================================================================

if target_platform == 'windows':
    # Windows MSVC environment
    # Map architecture names to MSVC expected values
    msvc_arch_map = {
        'x64': 'amd64',
        'x86': 'x86',
        'arm64': 'arm64',
        'arm': 'arm',
    }
    msvc_arch = msvc_arch_map.get(arch, 'amd64')

    env = Environment(
        TARGET_ARCH=msvc_arch,
        MSVC_USE_SCRIPT=True,
        tools=['msvc', 'mslink', 'mslib']
    )

    # Compiler flags
    common_flags = [
        '/W4',
        '/EHsc',
        '/std:c++17',
        '/utf-8',
        '/permissive-',
        '/DUNICODE',
        '/D_UNICODE',
        '/DWIN32_LEAN_AND_MEAN',
        '/DNOMINMAX',
        '/wd4996',  # Disable deprecated warnings (Windows SDK)
        '/D_SILENCE_ALL_CXX17_DEPRECATION_WARNINGS',  # Silence C++17 STL warnings
    ]

    if arch == 'x64':
        common_flags.append('/D_WIN64')

    if build_config == 'debug':
        env.Append(CCFLAGS=common_flags + ['/Od', '/Zi', '/MDd', '/D_DEBUG'])
        env.Append(LINKFLAGS=['/DEBUG:FULL'])
    elif build_config == 'release':
        env.Append(CCFLAGS=common_flags + ['/O2', '/Oi', '/GL', '/MD', '/DNDEBUG'])
        env.Append(LINKFLAGS=['/LTCG', '/OPT:REF', '/OPT:ICF'])
    else:  # relwithdebinfo
        env.Append(CCFLAGS=common_flags + ['/O2', '/Zi', '/MD', '/DNDEBUG'])
        env.Append(LINKFLAGS=['/DEBUG', '/OPT:REF'])

elif target_platform == 'linux':
    # Linux GCC environment
    env = Environment(
        CC='gcc',
        CXX='g++',
        tools=['gcc', 'g++', 'gnulink']
    )

    common_flags = [
        '-Wall',
        '-Wextra',
        '-Wpedantic',
        '-std=c++17',
        '-fPIC',
        '-fvisibility=hidden',
    ]

    if arch == 'x64':
        common_flags.extend(['-m64'])
    elif arch == 'x86':
        common_flags.extend(['-m32'])

    if build_config == 'debug':
        env.Append(CCFLAGS=common_flags + ['-O0', '-g3', '-D_DEBUG'])
    elif build_config == 'release':
        env.Append(CCFLAGS=common_flags + ['-O3', '-flto', '-DNDEBUG'])
        env.Append(LINKFLAGS=['-flto', '-s'])
    else:
        env.Append(CCFLAGS=common_flags + ['-O2', '-g', '-DNDEBUG'])

elif target_platform == 'android':
    ndk_path = GetOption('android_ndk')
    api_level = GetOption('android_api')

    if not ndk_path or not os.path.exists(ndk_path):
        print("Error: Android NDK not found. Set --android-ndk")
        Exit(1)

    # Determine host tag
    if sys.platform.startswith('win'):
        host_tag = 'windows-x86_64'
    elif sys.platform == 'darwin':
        host_tag = 'darwin-x86_64'
    else:
        host_tag = 'linux-x86_64'

    # Architecture mapping
    arch_map = {
        'arm64': 'aarch64-linux-android',
        'arm': 'armv7a-linux-androideabi',
        'x64': 'x86_64-linux-android',
        'x86': 'i686-linux-android',
    }

    target_triple = arch_map.get(arch, 'aarch64-linux-android')
    toolchain = os.path.join(ndk_path, 'toolchains', 'llvm', 'prebuilt', host_tag)

    clang_target = f'{target_triple}{api_level}'

    env = Environment(
        CC=os.path.join(toolchain, 'bin', f'{clang_target}-clang'),
        CXX=os.path.join(toolchain, 'bin', f'{clang_target}-clang++'),
        AR=os.path.join(toolchain, 'bin', 'llvm-ar'),
    )

    if sys.platform.startswith('win'):
        env['CC'] += '.exe'
        env['CXX'] += '.exe'
        env['AR'] += '.exe'

    common_flags = [
        '-Wall',
        '-Wextra',
        '-std=c++17',
        '-fPIC',
        '-DANDROID',
        f'-D__ANDROID_API__={api_level}',
    ]

    if build_config == 'debug':
        env.Append(CCFLAGS=common_flags + ['-O0', '-g'])
    else:
        env.Append(CCFLAGS=common_flags + ['-O3', '-flto', '-DNDEBUG'])
        env.Append(LINKFLAGS=['-flto', '-s'])

# =============================================================================
# Include Paths
# =============================================================================

env.Append(CPPPATH=[
    '#include',
    '#src',
    '#src/audio/sonic',
])

# =============================================================================
# Version Defines
# =============================================================================

env.Append(CPPDEFINES=[
    ('LAPRDUS_VERSION_MAJOR', VERSION_MAJOR),
    ('LAPRDUS_VERSION_MINOR', VERSION_MINOR),
    ('LAPRDUS_VERSION_PATCH', VERSION_PATCH),
    f'LAPRDUS_VERSION_STRING=\\"{VERSION_STRING}\\"',
])

# =============================================================================
# Core Library Sources
# =============================================================================

core_sources = [
    'src/core/phoneme_mapper.cpp',
    'src/core/croatian_numbers.cpp',
    'src/core/inflection.cpp',
    'src/core/tts_engine.cpp',
    'src/core/voice_registry.cpp',
    'src/core/pronunciation_dict.cpp',
    'src/core/spelling_dict.cpp',
    'src/core/emoji_dict.cpp',
    'src/core/user_config.cpp',
    'src/audio/phoneme_data.cpp',
    'src/audio/audio_synthesizer.cpp',
    'src/audio/sonic_processor.cpp',
    'src/audio/sonic/sonic.c',
    'src/audio/formant_pitch.cpp',
    'src/c_api/laprdus_api.cpp',
    'src/laprdus.cpp',
]

# =============================================================================
# Build Phoneme Packer Tool
# =============================================================================

packer_env = env.Clone()
packer_sources = ['tools/phoneme_packer/packer.cpp']

phoneme_packer = packer_env.Program(
    target=f'{build_dir}/phoneme_packer',
    source=packer_sources
)

# =============================================================================
# Pack Phonemes (Multiple Voices)
# =============================================================================

# Check if source phonemes exist
phoneme_base_dir = 'phonemes'
if not os.path.exists(phoneme_base_dir):
    phoneme_base_dir = 'data/phonemes'

# Voice directories to pack
voice_dirs = ['Josip', 'Vlado']
phonemes_packed = []

if sys.platform.startswith('win'):
    packer_exe = f'{build_dir}\\phoneme_packer.exe'
else:
    packer_exe = f'./{build_dir}/phoneme_packer'

for voice_name in voice_dirs:
    voice_source_dir = os.path.join(phoneme_base_dir, voice_name)
    voice_bin = os.path.join(build_dir, f'{voice_name}.bin')

    # Skip if voice directory doesn't exist
    if not os.path.exists(voice_source_dir):
        print(f"Warning: Voice directory {voice_source_dir} not found, skipping")
        continue

    # Build pack command
    pack_cmd = f'{packer_exe} --input-dir {voice_source_dir} --output {voice_bin}'
    if GetOption('enable_encryption'):
        pack_cmd += ' --encrypt'
        key = GetOption('phoneme_key')
        if key:
            pack_cmd += f' --key {key}'

    voice_packed = env.Command(
        target=voice_bin,
        source=[phoneme_packer, Glob(f'{voice_source_dir}/*.wav')],
        action=pack_cmd
    )
    phonemes_packed.append(voice_packed)

# =============================================================================
# Copy Voice Data to Shared Location
# =============================================================================

# Central location for voice data used by all platforms
voice_data_dir = 'data/voices'

# Ensure directory exists
if not os.path.exists(voice_data_dir):
    os.makedirs(voice_data_dir)

# Copy packed voice files to shared location
voice_data_targets = []
for voice_name in voice_dirs:
    voice_bin_src = os.path.join(build_dir, f'{voice_name}.bin')
    voice_bin_dst = os.path.join(voice_data_dir, f'{voice_name}.bin')

    # Find the corresponding packed phoneme
    for packed in phonemes_packed:
        if voice_name in str(packed[0]):
            copy_voice = env.Command(
                target=voice_bin_dst,
                source=packed,
                action=Copy('$TARGET', '$SOURCE')
            )
            voice_data_targets.append(copy_voice)
            break

env.Alias('voice-data', voice_data_targets)

# =============================================================================
# Platform-Specific Targets
# =============================================================================

if target_platform == 'windows':
    # Windows system libraries
    env.Append(LIBS=['kernel32', 'user32', 'advapi32', 'ole32', 'oleaut32', 'uuid', 'shell32'])

    # Shared library (DLL)
    dll_env = env.Clone()
    dll_env.Append(CPPDEFINES=['LAPRDUS_EXPORTS'])

    dll = dll_env.SharedLibrary(
        target=f'{build_dir}/laprdus',
        source=core_sources
    )

    # Depends on phoneme data
    env.Depends(dll, phonemes_packed)

    # Include voice_data_targets so data/voices/ is always populated
    Default(dll, phonemes_packed, voice_data_targets)

    # =========================================================================
    # SAPI5 COM DLL Target
    # =========================================================================

    sapi5_env = env.Clone()

    # SAPI5-specific defines
    # Note: Do NOT use _ATL_NO_AUTOMATIC_NAMESPACE as sphelper.h expects unqualified CComPtr
    # _WINDLL is required for ATL CAtlDllModuleT
    sapi5_env.Append(CPPDEFINES=[
        'LAPRDUS_EXPORTS',
        'LAPRDUS_SAPI5',
        '_ATL_CSTRING_EXPLICIT_CONSTRUCTORS',
        '_WINDLL',
    ])

    # Additional include paths
    sapi5_env.Append(CPPPATH=[
        '#src/platform/windows/sapi5',
    ])

    # Add ATL static library (required for COM registration)
    if build_config == 'debug':
        sapi5_env.Append(LIBS=['atlsd'])  # Debug ATL
    else:
        sapi5_env.Append(LIBS=['atls'])   # Release ATL

    # SAPI5 build directory for separate object files
    sapi5_build_dir = f'{build_dir}/sapi5'

    # Compile SAPI5 objects to separate directory
    sapi5_source_files = [
        # Core TTS engine
        'src/core/phoneme_mapper.cpp',
        'src/core/croatian_numbers.cpp',
        'src/core/inflection.cpp',
        'src/core/tts_engine.cpp',
        'src/core/voice_registry.cpp',
        'src/core/pronunciation_dict.cpp',
        'src/core/spelling_dict.cpp',
        'src/core/emoji_dict.cpp',
        'src/core/user_config.cpp',
        'src/audio/phoneme_data.cpp',
        'src/audio/audio_synthesizer.cpp',
        'src/audio/sonic_processor.cpp',
        'src/audio/sonic/sonic.c',
        'src/audio/formant_pitch.cpp',
        # C API for NVDA and other consumers
        'src/c_api/laprdus_api.cpp',
        # SAPI5 platform code
        'src/platform/windows/sapi5/sapi_driver.cpp',
        'src/platform/windows/sapi5/dllmain.cpp',
    ]

    # Compile each source to sapi5 subdirectory
    sapi5_objects = []
    for src in sapi5_source_files:
        obj_name = os.path.splitext(os.path.basename(src))[0]
        obj = sapi5_env.Object(
            target=f'{sapi5_build_dir}/{obj_name}.obj',
            source=src
        )
        sapi5_objects.append(obj)

    # Resource file
    sapi5_rc = sapi5_env.RES(
        target=f'{sapi5_build_dir}/laprdus_sapi5.res',
        source='src/platform/windows/sapi5/laprdus_sapi5.rc'
    )

    # Module definition file for COM exports
    sapi5_def = 'src/platform/windows/sapi5/laprdus_sapi5.def'
    sapi5_env.Append(LINKFLAGS=[f'/DEF:{sapi5_def}'])

    # Build SAPI5 DLL with architecture-specific name
    dll_name = 'laprd32' if arch == 'x86' else 'laprd64'
    sapi5_dll = sapi5_env.SharedLibrary(
        target=f'{build_dir}/{dll_name}',
        source=sapi5_objects + [sapi5_rc]
    )

    # SAPI5 DLL depends on phoneme data
    env.Depends(sapi5_dll, phonemes_packed)

    # Add SAPI5 target alias - includes voice data copy to data/voices/
    env.Alias('sapi5', [sapi5_dll, phonemes_packed, voice_data_targets])

    # =========================================================================
    # Windows Command-Line Interface
    # - x64: statically linked (no DLL dependency, avoids shipping extra DLL)
    # - x86: dynamically linked (avoids Windows Defender false positive)
    # =========================================================================
    cli_env = env.Clone()

    # Windows Multimedia library for audio playback
    cli_env.Append(LIBS=['winmm'])

    # CLI build directory for separate object files
    cli_build_dir = f'{build_dir}/cli'

    if arch == 'x64':
        # 64-bit: Static linking - compile core sources directly into CLI
        cli_env.Append(CPPDEFINES=['LAPRDUS_EXPORTS'])

        cli_all_sources = [
            # Core TTS engine
            'src/core/phoneme_mapper.cpp',
            'src/core/croatian_numbers.cpp',
            'src/core/inflection.cpp',
            'src/core/tts_engine.cpp',
            'src/core/voice_registry.cpp',
            'src/core/pronunciation_dict.cpp',
            'src/core/spelling_dict.cpp',
            'src/core/emoji_dict.cpp',
            'src/core/user_config.cpp',
            'src/audio/phoneme_data.cpp',
            'src/audio/audio_synthesizer.cpp',
            'src/audio/sonic_processor.cpp',
            'src/audio/sonic/sonic.c',
            'src/audio/formant_pitch.cpp',
            # C API
            'src/c_api/laprdus_api.cpp',
            # CLI-specific
            'src/platform/windows/cli/laprdus_cli_windows.cpp',
            'src/platform/windows/cli/getopt.c',
        ]

        # Compile each source to cli subdirectory
        cli_objects = []
        for src in cli_all_sources:
            obj_name = os.path.splitext(os.path.basename(src))[0]
            obj = cli_env.Object(
                target=f'{cli_build_dir}/{obj_name}.obj',
                source=src
            )
            cli_objects.append(obj)

        # Build CLI executable (statically linked)
        cli = cli_env.Program(
            target=f'{build_dir}/laprdus',
            source=cli_objects,
            LIBS=cli_env['LIBS']
        )

        # Add CLI target alias (no DLL dependency)
        env.Alias('cli', [cli, phonemes_packed, voice_data_targets])

    else:
        # 32-bit: Dynamic linking - link against laprdus.dll
        # This avoids Windows Defender false positives on x86 static builds

        cli_source_files = [
            'src/platform/windows/cli/laprdus_cli_windows.cpp',
            'src/platform/windows/cli/getopt.c',
            'src/core/user_config.cpp',  # Not exported from DLL
        ]

        # Compile CLI objects to separate directory
        cli_objects = []
        for src in cli_source_files:
            obj_name = os.path.splitext(os.path.basename(src))[0]
            obj = cli_env.Object(
                target=f'{cli_build_dir}/{obj_name}.obj',
                source=src
            )
            cli_objects.append(obj)

        # Build CLI executable (dynamically linked)
        cli = cli_env.Program(
            target=f'{build_dir}/laprdus',
            source=cli_objects,
            LIBS=cli_env['LIBS'] + ['laprdus'],
            LIBPATH=[build_dir]
        )

        # CLI depends on DLL
        env.Depends(cli, dll)

        # Add CLI target alias (includes DLL dependency)
        env.Alias('cli', [cli, dll, phonemes_packed, voice_data_targets])

    # =========================================================================
    # Windows CLI Tests
    # =========================================================================
    test_env = env.Clone()

    # Test build directory
    test_build_dir = f'{build_dir}/tests'

    # Test source files
    test_source = 'tests/windows/test_cli.cpp'

    # Compile test to separate directory
    test_obj = test_env.Object(
        target=f'{test_build_dir}/test_cli.obj',
        source=test_source
    )

    # Build test executable - links against laprdus library
    if arch == 'x64':
        # For x64, we can link against the DLL or statically
        # Using DLL for tests to verify API exports work correctly
        test_exe = test_env.Program(
            target=f'{build_dir}/test_cli',
            source=[test_obj],
            LIBS=test_env['LIBS'] + ['laprdus'],
            LIBPATH=[build_dir]
        )
        env.Depends(test_exe, dll)
    else:
        # For x86, link against DLL
        test_exe = test_env.Program(
            target=f'{build_dir}/test_cli',
            source=[test_obj],
            LIBS=test_env['LIBS'] + ['laprdus'],
            LIBPATH=[build_dir]
        )
        env.Depends(test_exe, dll)

    # Add test target alias
    env.Alias('test', [test_exe, cli, dll, phonemes_packed, voice_data_targets])

    # =========================================================================
    # Windows Configuration GUI
    # =========================================================================
    config_env = env.Clone()

    # Additional libraries for config GUI
    config_env.Append(LIBS=['comctl32', 'dwmapi', 'uxtheme', 'shlwapi', 'sapi', 'gdi32'])

    # Config GUI build directory
    config_build_dir = f'{build_dir}/config'

    # Config GUI source files (statically linked with core for simplicity)
    config_sources = [
        # Core (needed for UserConfig)
        'src/core/user_config.cpp',
        # Config GUI
        'src/platform/windows/config/main.cpp',
        'src/platform/windows/config/config_dialog.cpp',
        'src/platform/windows/config/dictionary_dialog.cpp',
        'src/platform/windows/config/dark_mode.cpp',
    ]

    # Compile config GUI objects to separate directory
    config_objects = []
    for src in config_sources:
        obj_name = os.path.splitext(os.path.basename(src))[0]
        obj = config_env.Object(
            target=f'{config_build_dir}/{obj_name}.obj',
            source=src
        )
        config_objects.append(obj)

    # Resource file
    config_rc = config_env.RES(
        target=f'{config_build_dir}/laprdus_config.res',
        source='src/platform/windows/config/laprdus_config.rc'
    )

    # Build config GUI executable
    config_gui = config_env.Program(
        target=f'{build_dir}/laprdgui',
        source=config_objects + [config_rc]
    )

    # Add config target alias
    env.Alias('config', config_gui)

elif target_platform == 'linux':
    # =========================================================================
    # Linux Shared Library
    # =========================================================================
    env.Append(LIBS=['pthread', 'dl', 'm'])
    env.Append(CPPDEFINES=['LAPRDUS_EXPORTS'])

    # Set SONAME for proper shared library versioning
    # This ensures binaries record 'liblaprdus.so.1' as their NEEDED library,
    # allowing versioned symlinks (liblaprdus.so.1 -> liblaprdus.so.1.0.0) to work
    env.Append(SHLINKFLAGS=['-Wl,-soname,liblaprdus.so.1'])

    lib = env.SharedLibrary(
        target=f'{build_dir}/liblaprdus',
        source=core_sources
    )

    env.Depends(lib, phonemes_packed)

    # Create versioned symlinks (liblaprdus.so.1 -> liblaprdus.so) for runtime linking
    lib_symlink = env.Command(
        f'{build_dir}/liblaprdus.so.1',
        lib,
        f'ln -sf liblaprdus.so $TARGET'
    )
    env.Clean(lib, lib_symlink)

    # Include voice_data_targets so data/voices/ is always populated
    Default(lib, lib_symlink, phonemes_packed, voice_data_targets)

    # =========================================================================
    # Linux Command-Line Interface
    # =========================================================================
    cli_env = env.Clone()

    # Check for audio libraries
    conf = Configure(cli_env)

    have_pulse = conf.CheckLib('pulse-simple') and conf.CheckLib('pulse')
    have_alsa = conf.CheckLib('asound')

    cli_env = conf.Finish()

    # Add audio backend defines
    if have_pulse:
        cli_env.Append(CPPDEFINES=['HAVE_PULSEAUDIO'])
        cli_env.Append(LIBS=['pulse-simple', 'pulse'])
        print("  Audio backend: PulseAudio")
    elif have_alsa:
        cli_env.Append(CPPDEFINES=['HAVE_ALSA'])
        cli_env.Append(LIBS=['asound'])
        print("  Audio backend: ALSA")
    else:
        print("  Audio backend: None (file output only)")

    # CLI build directory
    cli_build_dir = f'{build_dir}/cli'

    cli_sources = [
        'src/platform/linux/cli/laprdus_cli.cpp',
        'src/core/user_config.cpp',  # Not exported from shared library
    ]

    # Build CLI executable
    cli = cli_env.Program(
        target=f'{build_dir}/laprdus',
        source=cli_sources,
        LIBS=cli_env['LIBS'] + ['laprdus'],
        LIBPATH=[build_dir]
    )

    # CLI depends on library
    env.Depends(cli, lib)

    env.Alias('cli', [cli, lib, phonemes_packed])

    # =========================================================================
    # Speech Dispatcher Module
    # =========================================================================
    speechd_env = env.Clone()

    # Check for Speech Dispatcher types header (speechd_types.h)
    have_speechd = False
    try:
        speechd_env.ParseConfig('pkg-config --cflags speech-dispatcher')
        have_speechd = True
    except OSError:
        # Fallback to manual detection
        speechd_include_dirs = [
            '/usr/include/speech-dispatcher',
            '/usr/local/include/speech-dispatcher',
            '/usr/include/speech-dispatcher-modules',
        ]
        for inc_dir in speechd_include_dirs:
            if os.path.exists(inc_dir):
                speechd_env.Append(CPPPATH=[inc_dir])
                have_speechd = True
                break

    # Verify speechd_types.h exists (we bundle our own spd_module_main.h)
    if have_speechd:
        speechd_conf = Configure(speechd_env)
        have_speechd = speechd_conf.CheckHeader('speechd_types.h', language='C')
        speechd_env = speechd_conf.Finish()

    if have_speechd:
        print("  Speech Dispatcher: Found")

        # Add our bundled module framework to include path
        speechd_env.Append(CPPPATH=['src/platform/linux/speechd'])

        # Speech Dispatcher module sources (includes bundled framework)
        speechd_sources = [
            'src/platform/linux/speechd/module_main.c',
            'src/platform/linux/speechd/module_process.c',
            'src/platform/linux/speechd/module_readline.c',
            'src/platform/linux/speechd/laprdus_module.c',
        ]

        # Remove C++17 flag for C files, use C11
        speechd_cflags = [f for f in speechd_env['CCFLAGS'] if not f.startswith('-std=c++')]
        speechd_env.Replace(CCFLAGS=speechd_cflags + ['-std=c11'])

        # Build Speech Dispatcher module
        # Note: SD modules don't link to speechd - we bundle the framework
        speechd_module = speechd_env.Program(
            target=f'{build_dir}/sd_laprdus',
            source=speechd_sources,
            LIBS=['laprdus', 'pthread'],
            LIBPATH=[build_dir]
        )

        # Module depends on library
        env.Depends(speechd_module, lib)

        env.Alias('speechd', [speechd_module, lib, phonemes_packed])
    else:
        print("  Speech Dispatcher: Not found (module will not be built)")
        print("    Install libspeechd-dev or speech-dispatcher-dev to enable")

    # =========================================================================
    # Linux Install Targets
    # =========================================================================
    # Standard Linux installation paths
    prefix = GetOption('prefix')
    lib_dir = f'{prefix}/lib'
    bin_dir = f'{prefix}/bin'
    data_dir = f'{prefix}/share/laprdus'
    speechd_conf_dir = '/etc/speech-dispatcher/modules'

    # Detect system Speech Dispatcher module directory via pkg-config
    # SD only looks in its own module dir, not under a custom prefix
    import subprocess
    try:
        result = subprocess.run(['pkg-config', '--variable=modulebindir', 'speech-dispatcher'],
                                capture_output=True, text=True)
        speechd_module_dir = result.stdout.strip().rstrip('/')
    except (FileNotFoundError, OSError):
        speechd_module_dir = ''
    if not speechd_module_dir:
        # Fallback: check common locations
        for candidate in ['/usr/lib64/speech-dispatcher-modules', '/usr/lib/speech-dispatcher-modules']:
            if os.path.isdir(candidate):
                speechd_module_dir = candidate
                break
        else:
            speechd_module_dir = f'{prefix}/lib/speech-dispatcher-modules'

    # Install library
    install_lib = env.Install(lib_dir, lib)

    # Install CLI
    install_cli = env.Install(bin_dir, cli) if 'cli' in dir() else []

    # Install voice data
    install_voices = []
    for voice_name in voice_dirs:
        voice_bin = f'{build_dir}/{voice_name}.bin'
        install_voices.append(env.Install(data_dir, voice_bin))

    # Install dictionaries
    install_dicts = [
        env.Install(data_dir, 'data/dictionary/internal.json'),
        env.Install(data_dir, 'data/dictionary/spelling.json'),
        env.Install(data_dir, 'data/dictionary/emoji.json'),
    ]

    # Install Speech Dispatcher module and config
    install_speechd = []
    if have_speechd and 'speechd_module' in dir():
        install_speechd.append(env.Install(speechd_module_dir, speechd_module))
        install_speechd.append(env.Install(speechd_conf_dir,
            'src/platform/linux/speechd/laprdus.conf'))

    # Combined install target
    all_install = [install_lib, install_cli, install_voices, install_dicts, install_speechd]
    env.Alias('install', all_install)

    # =========================================================================
    # Post-install: Configure Speech Dispatcher automatically
    # =========================================================================
    def configure_speechd_action(target, source, env):
        """Configure Speech Dispatcher to use LaprdusTTS."""
        speechd_conf = '/etc/speech-dispatcher/speechd.conf'
        marker_start = '# BEGIN LAPRDUS TTS'
        marker_end = '# END LAPRDUS TTS'

        if not os.path.exists(speechd_conf):
            print("Note: Speech Dispatcher config not found. Module will be available when speechd is configured.")
            return 0

        # Check if already configured
        with open(speechd_conf, 'r') as f:
            content = f.read()

        if marker_start in content:
            print("LaprdusTTS already configured in Speech Dispatcher.")
            return 0

        if 'AddModule.*"laprdus"' in content or 'AddModule "laprdus"' in content:
            print("Note: Existing LaprdusTTS module entry found.")
            return 0

        print("Configuring Speech Dispatcher for LaprdusTTS...")

        config_block = '''
# BEGIN LAPRDUS TTS
# LaprdusTTS - Croatian/Serbian Text-to-Speech
# Added automatically by scons install
AddModule "laprdus" "sd_laprdus" "laprdus.conf"

# Set LaprdusTTS as default for Croatian and Serbian
LanguageDefaultModule "hr" "laprdus"
LanguageDefaultModule "sr" "laprdus"
LanguageDefaultModule "hr-HR" "laprdus"
LanguageDefaultModule "sr-RS" "laprdus"
# END LAPRDUS TTS
'''
        try:
            with open(speechd_conf, 'a') as f:
                f.write(config_block)
            print("LaprdusTTS configured successfully.")
            print("Restart Speech Dispatcher to apply changes: systemctl --user restart speech-dispatcher")
        except PermissionError:
            print("Warning: Could not modify speechd.conf (permission denied).")
            print("Run with sudo to configure Speech Dispatcher automatically.")

        return 0

    # Only run post-install if Speech Dispatcher module was built
    if have_speechd and 'speechd_module' in dir():
        configure_speechd = env.Command(
            target='configure-speechd',
            source=None,
            action=configure_speechd_action
        )
        env.Depends(configure_speechd, install_speechd)
        env.Alias('install', configure_speechd)
        env.AlwaysBuild(configure_speechd)

    # =========================================================================
    # Linux-all Target (build everything for Linux)
    # =========================================================================
    linux_all_targets = [lib, phonemes_packed, voice_data_targets]
    if 'cli' in dir():
        linux_all_targets.append(cli)
    if have_speechd and 'speechd_module' in dir():
        linux_all_targets.append(speechd_module)

    env.Alias('linux-all', linux_all_targets)

elif target_platform == 'android':
    # JNI shared library
    android_sources = core_sources + [
        'src/platform/android/jni_bridge.cpp',
    ]

    env.Append(LIBS=['log', 'android'])

    lib = env.SharedLibrary(
        target=f'{build_dir}/liblaprdus',
        source=core_sources  # JNI bridge added later
    )

    env.Depends(lib, phonemes_packed)

    # Copy to Android project
    abi_map = {
        'arm64': 'arm64-v8a',
        'arm': 'armeabi-v7a',
        'x64': 'x86_64',
        'x86': 'x86',
    }
    abi = abi_map.get(arch, 'arm64-v8a')

    android_install = env.Install(f'android/app/src/main/jniLibs/{abi}', lib)

    # Include voice_data_targets so data/voices/ is populated for Gradle
    Default(lib, android_install, phonemes_packed, voice_data_targets)

# =============================================================================
# NVDA Add-on Target
# =============================================================================

nvda_addon_dir = 'nvda-addon/addon/synthDrivers/laprdus'

def copy_nvda_addon_files(target, source, env):
    """Copy DLLs and phonemes.bin to NVDA addon directory."""
    import shutil

    # Ensure target directory exists
    target_dir = str(target[0].dir)
    if not os.path.exists(target_dir):
        os.makedirs(target_dir)

    # Copy files
    for src in source:
        src_path = str(src)
        if os.path.exists(src_path):
            dst_path = os.path.join(target_dir, os.path.basename(src_path))
            shutil.copy2(src_path, dst_path)
            print(f"Copied: {src_path} -> {dst_path}")

    return None

if target_platform == 'windows':
    # Define paths for both architectures
    x86_build_dir = 'build/windows-x86-release'
    x64_build_dir = 'build/windows-x64-release'

    nvda_addon_files = env.Command(
        target=f'{nvda_addon_dir}/laprd32.dll',
        source=[
            f'{x86_build_dir}/laprd32.dll',
            f'{x64_build_dir}/laprd64.dll',
            # Use voice data from data/voices/ (generated by voice-data target)
            'data/voices/Josip.bin',
            'data/voices/Vlado.bin',
            'data/dictionary/internal.json',
            'data/dictionary/spelling.json',
            'data/dictionary/emoji.json',
        ],
        action=copy_nvda_addon_files
    )

    # NVDA addon files depend on voice data being generated
    env.Depends(nvda_addon_files, voice_data_targets)

    # Build the .nvda-addon package
    def build_nvda_addon(target, source, env):
        """Run scons in nvda-addon directory to build the package."""
        import subprocess
        result = subprocess.run(
            ['scons'],
            cwd='nvda-addon',
            shell=True
        )
        return result.returncode

    nvda_addon_package = env.Command(
        target='nvda-addon/laprdus-1.0.0.nvda-addon',
        source=[nvda_addon_files],
        action=build_nvda_addon
    )

    env.Alias('nvda-addon', nvda_addon_package)

# =============================================================================
# Documentation Target
# =============================================================================

def generate_html_docs(target, source, env):
    """Convert Markdown documentation to HTML."""
    import subprocess

    # Check if markdown or pandoc is available
    markdown_converters = [
        ('pandoc', ['pandoc', '-f', 'markdown', '-t', 'html', '--standalone', '--toc', '--css=style.css']),
        ('markdown', ['markdown']),
        ('python-markdown', ['python', '-m', 'markdown'])
    ]

    converter = None
    converter_cmd = None

    for name, cmd in markdown_converters:
        try:
            result = subprocess.run([cmd[0], '--version'],
                                  capture_output=True,
                                  timeout=5)
            if result.returncode == 0:
                converter = name
                converter_cmd = cmd
                print(f"Using {name} for documentation generation")
                break
        except (FileNotFoundError, subprocess.TimeoutExpired):
            continue

    if not converter:
        print("WARNING: No Markdown converter found (pandoc, markdown, or python-markdown)")
        print("Installing python-markdown as fallback...")
        try:
            subprocess.run([sys.executable, '-m', 'pip', 'install', 'markdown'], check=True)
            converter = 'python-markdown'
            converter_cmd = [sys.executable, '-m', 'markdown']
            print("Successfully installed python-markdown")
        except subprocess.CalledProcessError:
            print("ERROR: Could not install python-markdown. Please install manually:")
            print("  pip install markdown")
            print("Or install pandoc from: https://pandoc.org/installing.html")
            return 1

    # Ensure output directory exists
    os.makedirs('docs/html', exist_ok=True)

    # Convert each markdown file to HTML
    success_count = 0
    for src in source:
        src_path = str(src)
        filename = os.path.basename(src_path)
        base_name = os.path.splitext(filename)[0]
        dst_path = f'docs/html/{base_name}.html'

        try:
            if converter == 'pandoc':
                # Pandoc with full options
                cmd = converter_cmd + ['-o', dst_path, src_path]
                result = subprocess.run(cmd, capture_output=True, text=True, timeout=30)

                if result.returncode == 0:
                    print(f"Generated: {dst_path}")
                    success_count += 1
                else:
                    print(f"ERROR converting {src_path}: {result.stderr}")
                    return 1
            else:
                # Simple markdown converter - read and redirect
                with open(src_path, 'r', encoding='utf-8') as f:
                    md_content = f.read()

                # Add basic HTML wrapper
                html_header = f'''<!DOCTYPE html>
<html lang="hr">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>{base_name}</title>
    <style>
        body {{
            font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, "Helvetica Neue", Arial, sans-serif;
            line-height: 1.6;
            max-width: 900px;
            margin: 0 auto;
            padding: 20px;
            color: #333;
        }}
        h1, h2, h3, h4, h5, h6 {{
            margin-top: 1.5em;
            margin-bottom: 0.5em;
            font-weight: 600;
        }}
        h1 {{ font-size: 2.5em; border-bottom: 2px solid #333; padding-bottom: 0.3em; }}
        h2 {{ font-size: 2em; border-bottom: 1px solid #ccc; padding-bottom: 0.3em; }}
        h3 {{ font-size: 1.5em; }}
        code {{
            background-color: #f4f4f4;
            padding: 2px 6px;
            border-radius: 3px;
            font-family: "Consolas", "Monaco", "Courier New", monospace;
        }}
        pre {{
            background-color: #f4f4f4;
            padding: 15px;
            border-radius: 5px;
            overflow-x: auto;
        }}
        pre code {{
            background-color: transparent;
            padding: 0;
        }}
        table {{
            border-collapse: collapse;
            width: 100%;
            margin: 1em 0;
        }}
        th, td {{
            border: 1px solid #ddd;
            padding: 12px;
            text-align: left;
        }}
        th {{
            background-color: #f4f4f4;
            font-weight: 600;
        }}
        blockquote {{
            border-left: 4px solid #ccc;
            margin-left: 0;
            padding-left: 1em;
            color: #666;
        }}
        a {{
            color: #0066cc;
            text-decoration: none;
        }}
        a:hover {{
            text-decoration: underline;
        }}
        img {{
            max-width: 100%;
            height: auto;
        }}
    </style>
</head>
<body>
'''
                html_footer = '''
</body>
</html>
'''

                # Convert markdown to HTML using python-markdown
                result = subprocess.run(
                    converter_cmd,
                    input=md_content,
                    capture_output=True,
                    text=True,
                    timeout=30
                )

                if result.returncode == 0:
                    html_body = result.stdout
                    with open(dst_path, 'w', encoding='utf-8') as f:
                        f.write(html_header + html_body + html_footer)
                    print(f"Generated: {dst_path}")
                    success_count += 1
                else:
                    print(f"ERROR converting {src_path}: {result.stderr}")
                    return 1

        except subprocess.TimeoutExpired:
            print(f"ERROR: Timeout converting {src_path}")
            return 1
        except Exception as e:
            print(f"ERROR converting {src_path}: {e}")
            return 1

    print(f"\nDocumentation generation complete: {success_count}/{len(source)} files converted")
    print(f"Output directory: docs/html/")
    return 0

# Define markdown documentation files
doc_sources = [
    'README.md',
    'docs/laprdus.md',
    'docs/dev.md',
]

# Create Command to generate HTML docs
html_docs = env.Command(
    target='docs/html/index.html',  # Dummy target
    source=doc_sources,
    action=generate_html_docs
)

env.Alias('docs', html_docs)

# =============================================================================
# Help Text
# =============================================================================

Help(f"""
LaprdusTTS Build System - Version {VERSION_STRING}
================================================

Targets:
  scons                    Build default target for detected platform
  scons --platform=X       Build for specific platform (windows, linux, android)
  scons --build-config=X   Build configuration (debug, release, relwithdebinfo)
  scons --arch=X           Target architecture (x64, x86, arm64, arm)
  scons sapi5              Build SAPI5 DLL (Windows only)
  scons nvda-addon         Build NVDA add-on (Windows only, requires both x86 and x64 builds)
  scons cli                Build command-line utility (Linux only)
  scons speechd            Build Speech Dispatcher module (Linux only)
  scons linux-all          Build all Linux targets (library, CLI, Speech Dispatcher)
  scons docs               Generate HTML documentation from Markdown files
  scons install            Install (Linux only)
  scons -c                 Clean build artifacts

Examples:
  scons --platform=windows --build-config=release --arch=x64
  scons --platform=linux --build-config=release
  scons --platform=android --arch=arm64 --android-ndk=/path/to/ndk

Linux Build:
  # Build all Linux components:
  scons --platform=linux --build-config=release linux-all

  # Install to system:
  sudo scons --platform=linux --build-config=release install

  # Test with Speech Dispatcher:
  spd-say -o laprdus "Dobar dan!"

NVDA Add-on Build (Windows):
  # First build both architectures:
  scons --platform=windows --arch=x86 --build-config=release sapi5
  scons --platform=windows --arch=x64 --build-config=release sapi5
  # Then build the add-on:
  scons nvda-addon

Options:
  --enable-encryption    Enable phoneme data encryption
  --phoneme-key=KEY      Encryption key (64 hex characters)
  --prefix=PATH          Installation prefix (default: /usr/local, Linux only)
""")
