//! `build`, `install` and `dist`: what a Linux package, `make install` or a
//! release tarball would do, for Linux and macOS.
//!
//! `build` compiles what `install` copies, so a package can build as the
//! user and install into its staging directory without compiling again.
//!
//! The installed tree, under `--destdir` and `--prefix`:
//!
//! - `bin/laprdus`, the command-line program (the engine is linked into it);
//! - `lib/liblaprdus.so.<version>` with the links `liblaprdus.so.<major>`
//!   and `liblaprdus.so` (Linux) or `lib/liblaprdus.dylib` (macOS): the C
//!   API for programs written in other languages, Cargo's
//!   `liblaprdus_capi` under the name the library has always had;
//! - `include/laprdus/laprdus_api.h`;
//! - `share/laprdus/`: the packed voices and the dictionaries, side by side,
//!   which is where the programs look for them;
//! - Linux: `lib/speech-dispatcher-modules/sd_laprdus` and
//!   `/etc/speech-dispatcher/modules/laprdus.conf`.
//!
//! Adding the module to `speechd.conf` and restarting Speech Dispatcher is
//! left to the packages' and the tarball's own scripts.

use std::error::Error;
use std::path::{Path, PathBuf};
use std::process::Command;
use std::{env, fs};

type Outcome = Result<(), Box<dyn Error>>;

const VERSION: &str = env!("CARGO_PKG_VERSION");
const MAJOR: &str = env!("CARGO_PKG_VERSION_MAJOR");

/// The oldest macOS the programs run on: the deployment target of the app.
const MACOS_DEPLOYMENT_TARGET: &str = "13.0";
const MACOS_TRIPLES: [&str; 2] = ["aarch64-apple-darwin", "x86_64-apple-darwin"];
/// The name a program that links the library looks for it by on macOS:
/// relative to the program's run path, so the library can be put anywhere.
const MACOS_INSTALL_NAME: &str = "@rpath/liblaprdus.dylib";

const DICTIONARIES: [&str; 4] = ["internal.json", "spelling.json", "emoji.json", "emoji_sl.json"];

/// What the command line asked for.
#[derive(Default)]
pub struct Options {
    /// A staging root the whole tree is put under (a package's `$pkgdir`).
    pub destdir: Option<PathBuf>,
    /// `/usr/local` unless given.
    pub prefix: Option<PathBuf>,
    /// The library's directory when it is not `<prefix>/lib` (Debian's
    /// multiarch directory, `lib64`).
    pub libdir: Option<PathBuf>,
    /// Speech Dispatcher's module directory when it is not
    /// `<prefix>/lib/speech-dispatcher-modules`.
    pub moduledir: Option<PathBuf>,
    /// macOS: both architectures in one binary.
    pub universal: bool,
    /// macOS: the identity `codesign` signs the program and the library with.
    pub identity: Option<String>,
    /// `--locked`, `--offline`, `--frozen`: handed on to Cargo.
    pub cargo_flags: Vec<String>,
}

#[derive(Clone, Copy, PartialEq)]
enum Os {
    Linux,
    MacOs,
}

fn host() -> Result<Os, Box<dyn Error>> {
    match env::consts::OS {
        "linux" => Ok(Os::Linux),
        "macos" => Ok(Os::MacOs),
        other => Err(format!("build, install and dist are for Linux and macOS, not {other}").into()),
    }
}

fn target_dir(root: &Path) -> PathBuf {
    env::var_os("CARGO_TARGET_DIR").map_or_else(|| root.join("target"), PathBuf::from)
}

fn run(command: &mut Command) -> Outcome {
    let program = command.get_program().to_string_lossy().into_owned();
    match command.status() {
        Ok(status) if status.success() => Ok(()),
        Ok(status) => Err(format!("{program} failed ({status})").into()),
        Err(error) => Err(format!("{program} could not be started: {error}").into()),
    }
}

fn cargo(root: &Path, options: &Options, triple: Option<&str>, arguments: &[&str]) -> Outcome {
    let mut cargo = Command::new(env::var("CARGO").unwrap_or_else(|_| "cargo".into()));
    cargo.current_dir(root).arg(arguments[0]).arg("--release").args(&options.cargo_flags);
    if let Some(triple) = triple {
        cargo.args(["--target", triple]);
    }
    if env::consts::OS == "macos" && env::var_os("MACOSX_DEPLOYMENT_TARGET").is_none() {
        cargo.env("MACOSX_DEPLOYMENT_TARGET", MACOS_DEPLOYMENT_TARGET);
    }
    run(cargo.args(&arguments[1..]))
}

/// The built programs and the library, ready to be copied.
struct Built {
    cli: PathBuf,
    module: Option<PathBuf>,
    library: PathBuf,
}

/// Builds the programs and the C API library and returns where they are.
fn build(root: &Path, os: Os, options: &Options) -> Result<Built, Box<dyn Error>> {
    // The name the library is found by at run time is part of the file:
    // the soname of the versioned links on Linux, and on macOS an install
    // name relative to the run path of whatever links it. `cargo rustc`
    // gives the argument to the linker of this one library only.
    let (file, name) = match os {
        Os::Linux => ("liblaprdus_capi.so", format!("-Wl,-soname,liblaprdus.so.{MAJOR}")),
        Os::MacOs => ("liblaprdus_capi.dylib", format!("-Wl,-install_name,{MACOS_INSTALL_NAME}")),
    };
    let name = format!("link-arg={name}");
    let triples: Vec<Option<&str>> =
        if options.universal { MACOS_TRIPLES.iter().copied().map(Some).collect() } else { vec![None] };
    let mut slices = Vec::new();
    for triple in triples {
        let mut programs = vec!["build", "-p", "laprdus-cli"];
        if os == Os::Linux {
            programs.extend(["-p", "laprdus-speechd"]);
        }
        cargo(root, options, triple, &programs)?;
        cargo(
            root,
            options,
            triple,
            &["rustc", "-p", "laprdus-capi", "--lib", "--crate-type", "cdylib", "--", "-C", &name],
        )?;
        let mut folder = target_dir(root);
        folder.extend(triple);
        slices.push(folder.join("release"));
    }
    let module = (os == Os::Linux).then(|| slices[0].join("sd_laprdus"));
    if let [only] = &slices[..] {
        return Ok(Built { cli: only.join("laprdus"), module, library: only.join(file) });
    }
    // One file with both architectures.
    let universal = target_dir(root).join("universal").join("release");
    fs::create_dir_all(&universal)?;
    for (built, joined) in [("laprdus", "laprdus"), (file, "liblaprdus.dylib")] {
        let mut lipo = Command::new("lipo");
        lipo.arg("-create").args(slices.iter().map(|slice| slice.join(built)));
        run(lipo.arg("-output").arg(universal.join(joined)))?;
    }
    Ok(Built { cli: universal.join("laprdus"), module, library: universal.join("liblaprdus.dylib") })
}

/// `path` under the staging root: `/usr/bin` under `pkg` is `pkg/usr/bin`.
fn staged(destdir: Option<&Path>, path: &Path) -> PathBuf {
    match destdir {
        Some(destdir) => destdir.join(path.strip_prefix("/").unwrap_or(path)),
        None => path.to_owned(),
    }
}

#[cfg(unix)]
fn set_mode(path: &Path, mode: u32) -> Outcome {
    use std::os::unix::fs::PermissionsExt;
    Ok(fs::set_permissions(path, fs::Permissions::from_mode(mode))?)
}

#[cfg(not(unix))]
fn set_mode(_path: &Path, _mode: u32) -> Outcome {
    Ok(())
}

#[cfg(unix)]
fn link(target: &str, at: &Path) -> Outcome {
    if at.symlink_metadata().is_ok() {
        fs::remove_file(at)?;
    }
    Ok(std::os::unix::fs::symlink(target, at)?)
}

#[cfg(not(unix))]
fn link(_target: &str, _at: &Path) -> Outcome {
    Err("symbolic links are made on Linux and macOS only".into())
}

/// Copies one file with the mode an installed file has, whatever the mode
/// of the source (a checkout on a Windows drive has every file executable).
fn place(from: &Path, to: &Path, mode: u32) -> Outcome {
    let folder = to.parent().ok_or("a file has a folder")?;
    fs::create_dir_all(folder)?;
    // Not over the old file in place: a running program may have it mapped.
    if to.symlink_metadata().is_ok() {
        fs::remove_file(to)?;
    }
    fs::copy(from, to).map_err(|error| format!("{} -> {}: {error}", from.display(), to.display()))?;
    set_mode(to, mode)?;
    println!("{}", to.display());
    Ok(())
}

/// Signs a program or a library. The hardened runtime, which notarization
/// requires, is for programs only, and neither it nor the secure timestamp
/// can go with an ad hoc signature (`-`), which has no certificate.
fn sign(file: &Path, identity: &str, program: bool) -> Outcome {
    let mut codesign = Command::new("codesign");
    codesign.args(["--force", "--sign", identity]);
    if identity != "-" {
        codesign.arg("--timestamp");
        if program {
            codesign.args(["--options", "runtime"]);
        }
    }
    run(codesign.arg(file))
}

/// Fails when the library does not carry its install name: a program linked
/// against it would then look for Cargo's file name and not find it.
fn check_install_name(library: &Path) -> Outcome {
    let listed = Command::new("otool").arg("-D").arg(library).output()?;
    if String::from_utf8_lossy(&listed.stdout).lines().any(|line| line.trim() == MACOS_INSTALL_NAME) {
        Ok(())
    } else {
        Err(format!("{} does not have the install name {MACOS_INSTALL_NAME}", library.display()).into())
    }
}

fn install_as(root: &Path, os: Os, options: &Options) -> Outcome {
    let built = build(root, os, options)?;
    let destdir = options.destdir.as_deref();
    let prefix = options.prefix.clone().unwrap_or_else(|| "/usr/local".into());
    let lib = staged(destdir, &options.libdir.clone().unwrap_or_else(|| prefix.join("lib")));
    let prefix = staged(destdir, &prefix);

    let cli = prefix.join("bin/laprdus");
    place(&built.cli, &cli, 0o755)?;
    match os {
        Os::Linux => {
            let versioned = format!("liblaprdus.so.{VERSION}");
            let major = format!("liblaprdus.so.{MAJOR}");
            place(&built.library, &lib.join(&versioned), 0o755)?;
            link(&versioned, &lib.join(&major))?;
            link(&major, &lib.join("liblaprdus.so"))?;
        }
        Os::MacOs => {
            let library = lib.join("liblaprdus.dylib");
            place(&built.library, &library, 0o755)?;
            check_install_name(&library)?;
            if let Some(identity) = &options.identity {
                sign(&library, identity, false)?;
                sign(&cli, identity, true)?;
            }
        }
    }
    place(
        &root.join("src/platform/c_api/include/laprdus/laprdus_api.h"),
        &prefix.join("include/laprdus/laprdus_api.h"),
        0o644,
    )?;

    let share = prefix.join("share/laprdus");
    let mut voices: Vec<_> = fs::read_dir(root.join("data/voices"))?
        .flatten()
        .map(|entry| entry.path())
        .filter(|path| path.extension().is_some_and(|extension| extension == "bin"))
        .collect();
    voices.sort();
    for voice in voices {
        place(&voice, &share.join(voice.file_name().unwrap_or_default()), 0o644)?;
    }
    for dictionary in DICTIONARIES {
        place(&root.join("data/dictionary").join(dictionary), &share.join(dictionary), 0o644)?;
    }

    if let Some(module) = &built.module {
        let modules = match &options.moduledir {
            Some(modules) => staged(destdir, modules),
            None => prefix.join("lib/speech-dispatcher-modules"),
        };
        place(module, &modules.join("sd_laprdus"), 0o755)?;
        place(
            &root.join("src/platform/linux/speechd/laprdus.conf"),
            &staged(destdir, Path::new("/etc/speech-dispatcher/modules/laprdus.conf")),
            0o644,
        )?;
    }
    Ok(())
}

/// Builds what `install` copies, and nothing more: a package's build step.
pub fn build_only(root: &Path, options: &Options) -> Outcome {
    let built = build(root, host()?, options)?;
    for file in [Some(built.cli), built.module, Some(built.library)].into_iter().flatten() {
        println!("{}", file.display());
    }
    Ok(())
}

/// Builds and installs. The voice data must have been packed (`data`).
pub fn install(root: &Path, options: &Options) -> Outcome {
    install_as(root, host()?, options)
}

/// The release tarball: `dist/<os>-<arch>/laprdus-<version>-<os>-<arch>.tar.xz`
/// with the installed tree at its top (`bin`, `lib`, `include`, `share`, on
/// Linux `etc`), the documents and the tarball's install and removal scripts.
pub fn dist(root: &Path, options: &Options) -> Outcome {
    let os = host()?;
    let (system, arch, scripts) = match os {
        Os::Linux => ("linux", env::consts::ARCH, "installers/linux/tarball"),
        Os::MacOs => {
            let arch = match env::consts::ARCH {
                _ if options.universal => "universal",
                "aarch64" => "arm64",
                _ => "x64",
            };
            ("macos", arch, "installers/macos/tarball")
        }
    };
    let name = format!("laprdus-{VERSION}-{system}-{arch}");
    let folder = root.join("dist").join(format!("{system}-{arch}"));
    let stage = folder.join(&name);
    if stage.exists() {
        fs::remove_dir_all(&stage)?;
    }
    fs::create_dir_all(&stage)?;
    let layout = Options {
        destdir: Some(stage.clone()),
        prefix: Some("/".into()),
        libdir: None,
        moduledir: None,
        universal: options.universal,
        identity: options.identity.clone(),
        cargo_flags: options.cargo_flags.clone(),
    };
    install_as(root, os, &layout)?;

    let documents: &[&str] = match os {
        Os::Linux => &["readme.md", "LICENSE"],
        Os::MacOs => &["readme.md", "LICENSE", "docs/changelog-en.md", "docs/changelog-hr.md"],
    };
    for document in documents {
        let from = root.join(document);
        place(&from, &stage.join("share/doc/laprdus").join(from.file_name().unwrap_or_default()), 0o644)?;
    }
    for (file, mode) in [("install.sh", 0o755), ("uninstall.sh", 0o755), ("README", 0o644)] {
        let from = root.join(scripts).join(file);
        if from.is_file() {
            place(&from, &stage.join(file), mode)?;
        }
    }

    let tarball = folder.join(format!("{name}.tar.xz"));
    let mut tar = Command::new("tar");
    // Without this the tar of macOS adds a `._` file for the extended
    // attributes of every file, junk on any other system.
    tar.env("COPYFILE_DISABLE", "1").arg("-C").arg(&folder);
    if os == Os::Linux {
        // The files belong to root where they are installed, not to
        // whoever built the tarball.
        tar.args(["--owner=0", "--group=0", "--numeric-owner"]);
    }
    run(tar.arg("-cJf").arg(&tarball).arg(&name))?;
    println!("{}", tarball.display());
    Ok(())
}
