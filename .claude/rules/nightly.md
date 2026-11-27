---
paths:
  - ".github/workflows/**"
  - "tools/nightly/**"
  - "scripts/upload-testflight.sh"
  - "scripts/testflight_distribute.py"
  - "src/platform/android/app/src/nightly/**"
---
# Nightly builds

Since 2026-11-24. Testers get a build of every platform each night from a download site of its own. GitHub Releases keep only the stable versions. The owner chose the parts:
- Cloudflare R2 for the site.
- A notarized macOS disk image, and an iPhone build on TestFlight for the testers they already have.
- A separate Android app, "Laprdus Nightly", signed with their own keystore.
- Dev and beta add-ons for the NVDA add-on store.
- `dev` stays non-default and nothing nightly goes to `main`.

The owner's setup guide is outside the repository: `C:\Users\hrvoj\OneDrive\Documents\Laprdus nightly builds - setup guide.txt`, plain text and friendly. Keep it in step when the secrets or the steps change.

**Not yet run end to end** (2026-11-24). The page generator, the version switches of the add-on and the installer, the Linux nightly mode, the nightly APK and the TestFlight script's token were each run on the Windows machine.

## What starts a night

GitHub runs a `schedule` (and the "Run workflow" button) only for workflows on the repository's default branch, `main`. Nothing nightly goes there, and `dev` stays non-default (the owner's decision). So:

- **The alarm clock is the private repository `hkatic/laprdus-nightly-trigger`.** Its one workflow, `start-nightly.yml`, runs at 02:00 UTC and on its own "Run workflow" button (input `force`). It puts the tag `nightly/<YYYYMMDD-HHMMSS>[-force]` on `dev`'s newest commit through the API (`git/refs`), with the secret `LAPRDUS_TAG_TOKEN`: a fine-grained token on `hkatic/laprdus` only, Contents read and write. Without the secret it warns and starts nothing.
- **Why private:** GitHub switches off the schedules of a public repository after 60 days without activity.
- **Why a tag:** a tag pushed with a personal token starts workflows, unlike the `GITHUB_TOKEN`'s own events, and a tag push runs the workflow of the tagged commit. So `nightly.yml` lives on `dev` only.
- **A nightly by hand:** "Run workflow" in the helper repository, or `git push origin <commit>:refs/tags/nightly/<anything>[-force]` with one's own credentials.

## How a night goes (`.github/workflows/nightly.yml`, `on: push: tags: ["nightly/*"]`)

- **`check`:**
  - Refuses a tag whose commit is not on `dev`.
  - Reads `${NIGHTLY_BASE_URL}/latest.json` and stops when it names the same commit, unless the tag ends in `-force`. So there is no nightly without new commits.
  - Without `NIGHTLY_BASE_URL`, only a forced nightly builds. There is no site to publish on, and a full build failing at `publish` every night would only send failure emails.
  - Outputs the versions (below), the number, and the changes since the last nightly (`<short sha>\t<subject>`, at most 100 lines) for the page and TestFlight.
- **`build`** calls `build.yml` and **`linux`** calls `linux-packages.yml`, with the inputs `nightly`, `addon-version`, `number` and `changes`. Every test of a push runs, and nothing is published unless all of them pass. The `linux` job has `contents: write` only because GitHub checks every job of a called workflow, including the release job, which runs on `v*` tags only.
- **`publish`:**
  - Collects the files into `night/` and writes the site with `tools/nightly/publish.py`.
  - Uploads to R2 with `aws s3`: the night's folder first, the page and the lists last. It deletes the nights beyond the last 14.
  - Puts the add-on into `nvda/` for the store (below).
  - Reads `latest.json` back and checks the SHA-256 of every file it lists.
  - Writes the run's summary: the site, the TestFlight build, and the two links for the store's form.
- **`remove-tag`** (always): once the night has gone through, or had nothing to build, it deletes the run's tag and every older `nightly/*` tag. The names hold their time, so they sort by it, and a newer tag of a nightly still waiting its turn is left alone. A night that failed keeps its tag: a re-run (`gh run rerun --failed`) checks the commit out by the tag's name, and the first test nightly (2026-11-24) could not be re-run because its tag was gone. Deleted with the `GITHUB_TOKEN`, a tag starts nothing.
- **Concurrency.** The group is `nightly`, never cancelled. In the called workflows the groups hold `github.workflow` (the caller's name there) and the ref, so pushes and nightlies never cancel each other.
- **`LAPRDUS_RELEASE_TAG`** of `linux-packages.yml` is set for `refs/tags/v*` only: a nightly tag would fail `source.sh`'s check that the tag names the version.

## Versions

A nightly is a **pre-release of the version `Cargo.toml` names**: `<version>-nightly.<YYYYMMDD>`, e.g. `2.0.0-nightly.20261124`. Its **number** is `git rev-list --count HEAD`, which only grows. The **NVDA add-on's version is the date**, `YYYY.M.D` without leading zeros (`2026.11.24`), because the NVDA add-on store takes only plain numbers, each used once across all channels (the owner's choice).

| Platform | How the version is given | What the nightly gets |
|---|---|---|
| Windows installer | `ISCC /DAppVersion=<version> /DAddonVersion=<add-on version>` (both `#ifndef` in `laprdus_sapi5.iss`, `AddonVersion` defaulting to `AppVersion`); Git Bash needs `MSYS_NO_PATHCONV=1`, or it turns `/D...` into paths | `Laprdus_SAPI5_Setup_<version>.exe`, the same AppId, so it installs over the stable version. It carries `laprdus-<add-on version>.nvda-addon`. Unsigned: SmartScreen shows "More info, Run anyway" |
| NVDA add-on | `scons version=<add-on version>`, no channel: the store's form chooses it | `laprdus-2026.11.24.nvda-addon`. `sconstruct` makes the manifest and the manuals again when the version or channel changes (`buildVersion`, an `env.Value`). Before 2026-11-24 a version given on the command line renamed the file but left the manifest at 2.0.0 |
| Android | build type `nightly` (`app/build.gradle.kts`): `initWith(release)`, `applicationIdSuffix = ".nightly"`, `-PlaprdusNightlyVersion` / `-PlaprdusNightlyNumber` through `androidComponents.onVariants`, the key from `LAPRDUS_NIGHTLY_KEYSTORE*` (the debug key without them; JKS or PKCS12, both load); the names in `src/nightly/res/values{,-hr,-sr,-sl}` | `com.hrvojekatic.laprdus.nightly`, "Laprdus TTS Nightly" / speech engine "Laprdus Nightly", signed with the owner's keystore. Google Play re-signs the store's app, so no other APK can update it. The nightly is a second app and TTS engine with its own settings, and each night updates the last. The workflow checks with `apksigner` that the APK is not debug-signed, and with `aapt2` its package, version code and name |
| Linux | `LAPRDUS_NIGHTLY=<YYYYMMDD>` for `installers/linux/ci/source.sh`, which changes only the tarball's copies of `debian/changelog`, the spec and the PKGBUILD | deb `2.0.0-0~nightly20261124`, rpm `2.0.0-0.nightly20261124`, Arch `2.0.0-0.20261124`: below the release `2.0.0-1` and above the night before. `build.sh` checks this in the build containers with `dpkg --compare-versions`, `rpmdev-vercmp` and `vercmp`. `publish` renames the binary tarball |
| macOS | `build-macos-release.sh --api-key/--api-key-id/--api-issuer --build-number --version-suffix` | `Laprdus-<version>-macOS.dmg` (notarized, stapled) and `laprdus-<version>-macos-universal.tar.xz` |
| iPhone | `scripts/upload-testflight.sh --build-number`, then `scripts/testflight_distribute.py` | TestFlight build `2.0.0 (41.<number>)` for the groups of `TESTFLIGHT_GROUPS`. `CFBundleShortVersionString` must stay numeric, so only the build number marks a nightly |

**After a stable release, bump the version** (`Cargo.toml` and every file `source.sh` compares with it, `buildVars.py`, the `.iss`, Gradle, Xcode's `MARKETING_VERSION`) to the next one, so the nightlies are pre-releases of that version.

**Since 2026-11-27 the nightlies come from `dev`, which is 2.1.0, while 2.0 is finished on `beta`** ("Branches" in `CLAUDE.md`). For 2.1.0 the bump was made early, when `beta` branched off, so the nightlies are pre-releases of 2.1.0 and carry the 2.0 fixes once `beta` is merged into `dev`. `nightly.yml` still builds only commits of `dev`.

**Apple build numbers:** a nightly's is `<CURRENT_PROJECT_VERSION>.<number>`. When `CURRENT_PROJECT_VERSION` is raised for a release (41 → 42), upload the release before the next nightly. Otherwise the nightly's 42.612 reaches App Store Connect first, and it refuses the release's lower 42. When a version has been released on the App Store, App Store Connect closes it for new builds, and `MARKETING_VERSION` must go up.

## TestFlight: the same testers, no new link

`scripts/testflight_distribute.py` uses the Python standard library only, with `openssl` signing the ES256 token. It works through the App Store Connect API:
1. Finds the app by bundle id and waits until the build is processed (`VALID`, up to 75 minutes).
2. Declares no non-exempt encryption if the build has no answer. The app's Info.plist already declares it: `INFOPLIST_KEY_ITSAppUsesNonExemptEncryption = NO` in both configurations of the app target. The owner approved this on 2026-11-24, and it is true: the app's only network use is three links opened in the browser.
3. Sets "What to Test" from the night's changes, and turns on the testers' notification.
4. Adds the build to every group the repository variable `TESTFLIGHT_GROUPS` names (comma-separated). An internal group with access to all builds is left alone.
5. Submits the build to Beta App Review when a group is external.

The testers keep their invitations and public links, and the TestFlight app shows each new build. Without `TESTFLIGHT_GROUPS` the build is uploaded and a warning says only internal groups with automatic distribution get it. `--dry-run` reads but sends no change; with `--api-key-id DRYRUN` it only makes a token.

## NVDA add-on store (dev and beta)

- **Submission is by hand.** NV Access asks that store submissions be made by filling in their issue form (`nvaccess/addon-datastore`, template `registerAddon.yml`) by hand. Their In-Process post of 10 March 2026: submitting "via CLI or do it for you" by agents "currently doesn't work". So the nightly automates everything up to the form.
- **The form's links.** The run's summary has two links that open the form filled in for the dev and for the beta channel: `download-url`, `source-url`, `publisher`, `channel`, `license-name`, `license-url`. The owner checks it and presses Submit. Any nightly can go to dev or to beta, but only to one of them: a version is used once across all channels.
- **The download address must stay valid** (the store records the SHA-256). `publish` puts the add-on into `nvda/laprdus-<add-on version>.nvda-addon` and never replaces a version already there; a forced second run on the same day has the same version. Files in `nvda/` go after 14 days, unless the store has that version (`addons/laprdus/<version>.json` in `nvaccess/addon-datastore`).
- **Approval and checks.** The first submission of an add-on is approved by NV Access, which can take up to two weeks. Laprdus was not in the store on 2026-11-24. VirusTotal scans every submission, and a hit on the unsigned DLLs or EXEs keeps it pending until NV Access looks.
- **The license is GPL v3** (`buildVars.py`, as `LICENSE`; the template's "GPL 2" was wrong until 2026-11-24).

## The site

```
index.html       the page: newest night first, a heading per platform, links with size and SHA-256,
                 what each file is for and how to install it, the changes since the last nightly,
                 fixed links, the earlier nights; English, no scripts, for screen readers first
latest.json      {version, date, commit, number, testflight, files: [{platform, kind, arch, name, size, sha256, url}]}
nightlies.json   the 14 nights kept (publish.py reads it back the next night)
<YYYY-MM-DD>/    the night's files and SHA256SUMS
latest/          the newest files under names without the version (Laprdus-nightly-setup.exe, ...), Cache-Control no-cache
nvda/            the add-ons for the NVDA add-on store (above)
```

`publish.py` knows each file by its name (`PLATFORMS`) and refuses a file it does not know, so a new kind of file needs a pattern, a note and a label there. A second run on the same day replaces that day. Test it without the site: `python -I tools/nightly/publish.py --files <dir> --out <dir> --version ... --date ... --commit ... --number ...`. A night is about 150 MB, so 14 nights take about 2 GB of R2's free 10 GB. Downloads from R2 cost nothing.

## Secrets and variables (hkatic/laprdus, Settings > Secrets and variables > Actions)

Each platform's nightly step runs only when its secrets are set. Without them it writes a warning, and the nightly goes out without that platform. The site's own settings are required.

| Name | Kind | For |
|---|---|---|
| `R2_ACCOUNT_ID`, `R2_ACCESS_KEY_ID`, `R2_SECRET_ACCESS_KEY` | secrets | the upload (required) |
| `NIGHTLY_BASE_URL` | variable | the site's address, e.g. `https://nightly.hrvojekatic.com` (required) |
| `NIGHTLY_BUCKET` | variable | the bucket, if not `laprdus-nightly` |
| `ANDROID_KEYSTORE_B64`, `ANDROID_KEYSTORE_PASSWORD`, `ANDROID_KEY_ALIAS`, `ANDROID_KEY_PASSWORD` | secrets | the nightly APK: the owner's own JKS keystore, in base64 |
| `APPLE_DEVELOPER_ID_P12_B64`, `APPLE_DEVELOPER_ID_P12_PASSWORD` | secrets | codesign of the macOS CLI, library and disk image |
| `ASC_KEY_ID`, `ASC_ISSUER_ID`, `ASC_KEY_P8_B64` | secrets | App Store Connect API key (Admin): Xcode's cloud signing, profiles, notarization, the TestFlight upload and distribution |
| `TESTFLIGHT_GROUPS` | variable | the TestFlight groups that get each nightly, comma-separated |
| `LAPRDUS_TAG_TOKEN` | secret of `hkatic/laprdus-nightly-trigger` | the alarm clock's token |

The Mac's own notarization profile (`notarytool store-credentials laprdus`) lives in that Mac's keychain and cannot leave it. On GitHub, the API key notarizes instead.
