# Windows 7 SP1, 8 and 8.1

The Windows programs (the SAPI5 driver, the command-line program and the
Configurator) are built twice:

| Build | For | Step | Staged in |
|-------|-----|------|-----------|
| ordinary | Windows 10 and later | `cargo xtask windows` | `dist/windows-x64/`, `dist/windows-x86/` |
| legacy | Windows 7 SP1, 8, 8.1 | `cargo xtask windows-legacy` | `dist/windows-legacy-x64/`, `dist/windows-legacy-x86/` |

Both are the same source: the crates in `../sapi5`, `../configurator` and
`src/platform/cli`. The files have the same names in both. The installer
carries both and installs the one for the Windows it runs on; the NVDA add-on
carries the 32-bit legacy driver and Configurator next to the ordinary ones
(`laprd32_legacy.dll`, `laprdgui32_legacy.exe`) and picks by the version of
Windows. A legacy program also runs on Windows 10 and 11.

## Why a second build

Rust's Windows targets (`x86_64-pc-windows-msvc`, `i686-pc-windows-msvc`)
need Windows 10 since Rust 1.78. Three things keep the ordinary programs from
loading on Windows 7; the loader refuses a program that imports from a DLL the
system does not have, before any of its code runs:

1. Rust's standard library imports `ProcessPrng` from `bcryptprimitives.dll`
   and `WaitOnAddress` from an API set, both of Windows 8.
2. The `windows` crate imports `CoTaskMemAlloc`, `CoTaskMemFree` and
   `CoCreateFreeThreadedMarshaler` from `combase.dll`, which Windows 8
   introduced.
3. The C runtime is linked dynamically (`VCRUNTIME140.dll` and the universal C
   runtime), which Windows 7, 8 and 8.1 do not come with.

## How it is built

- **Targets `x86_64-win7-windows-msvc` and `i686-win7-windows-msvc`.** Their
  standard library uses what Windows 7 has. They are tier 3: Rust ships no
  standard library for them, so it is compiled from its sources
  (`-Zbuild-std`), which only a nightly compiler does. `toolchain.txt` names
  the nightly; `cargo xtask windows-legacy` runs it through rustup
  (`LAPRDUS_LEGACY_TOOLCHAIN` overrides it). Everything else in the workspace
  keeps the stable compiler: there is no `rust-toolchain.toml`.

      rustup toolchain install nightly-2026-10-30 --component rust-src

- **The C runtime is linked statically** (`-Ctarget-feature=+crt-static`).
- **`combase.dll` is delay-loaded** (`/DELAYLOAD:combase.dll`, `delayimp.lib`):
  the loader no longer asks for it, and the linker's helper loads it at the
  first call of one of its three functions. The crate in `compat/`
  (`laprdus-win7`) defines the helper's hook, `__pfnDliNotifyHook2`, which
  answers the request for `combase.dll` with `ole32.dll`. `ole32.dll` has
  exported all three functions since before Windows 7, and from Windows 8 on
  forwards them to `combase.dll`. The SAPI5 driver and the Configurator link
  `laprdus-win7` only under `cfg(target_vendor = "win7")`; nothing else in
  their code differs between the builds.

The flags are `LEGACY_RUSTFLAGS` in `tools/xtask/src/main.rs`.

## Checking it without a Windows 7 machine

    python tests/windows/test_legacy_imports.py

reads the import tables of the six staged files: nothing may be imported from
a DLL Windows 7 lacks, `combase.dll` may only be delay-loaded and only for
functions `ole32.dll` of Windows 7 exports, and the driver's hook, called
directly, must answer `combase.dll` with `ole32.dll`.

The legacy programs run on the development machine too, so the other tests
take them as they take the ordinary ones:

    python tests/windows/test_sapi5_driver.py dist/windows-legacy-x64/laprd64.dll
    python tools/golden/golden.py check dist/windows-legacy-x64/laprd64.dll data/voices data/dictionary

## What is not verified

Nothing here has been run on Windows 7, 8 or 8.1 (November 2026). The import
tables say the programs can load there; they do not say that every function
taken from a DLL that exists is in that system's version of it, nor how the
programs behave. Open points for a test on those systems:

- installing with the installer, and speech through SAPI5 (Narrator, a 32-bit
  and a 64-bit SAPI5 program);
- the NVDA add-on under NVDA 2023.3.4 (the last for Windows 7 SP1 and 8;
  Python 3.7) and NVDA 2025.3.3 (the last for Windows 8.1 and for 32 bits);
- the Configurator (it has no dark mode there: the calls fail quietly);
- playback of the command-line program (`cpal` 0.16 through WASAPI); writing
  a file with `-o` does not depend on it.

The targets are tier 3: a newer nightly may break the build. Keep
`toolchain.txt` on a nightly that is known to build, and rerun the tests
above after changing it.
