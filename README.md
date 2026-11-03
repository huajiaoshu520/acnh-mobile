# ACNH Mobile

**0.3.114 · build 123 — official Metal 4 edition, iOS 26+.**

Portrait companion now enables checked pocket moves, item actions, sorting and tool-ring interactions for the matching ACNH 3.0.3 build. Lower-screen Press A remains gated by upper-screen title readiness. Real iPad/Duo interaction verification is pending.

Startup → Personalization includes Game volume (0–100%, default 100%). Game audio mixes with background music; the silent-mode playback preference is retained.

Optional iCloud Drive save sharing is available in the iPhone and Mac launchers; choose the same folder on both devices. Replaced saves are backed up locally.

Experimental same-Wi-Fi local networking is included as an ACNH-only patch. LiveContainer + SideStore users must update to the LAN-enabled host attached to this release and allow local network access. [Release notes](docs/RELEASE-v0.3.112.zh-CN.md).
A native compatibility runtime for Animal Crossing: New Horizons 3.0.3. The runtime and shader translation sources are based on [NXNative](https://github.com/seedds/nxnative) commit `742c9fb83c3719674720ede99da6c1bc997904da`.

This edition uses the official Metal 4 replay path and matching shader chain. It retains ACNH's icon touch controls, tool wheel, physical controllers, R/field-triggered Chinese text input, direct external `.data` access, transactional saves, amiibo file import, Photos export and bounded haptics. Custom touch layouts can be edited from startup settings; there is no added in-game typing toolbar.

[简体中文](README.zh-CN.md) · [Shell guide](docs/SHELL.md) · [Build guide](docs/BUILD.md)

Adaptive expanded view preserves horizontal FOV and reveals more vertical scenery on landscape iPads; wide iPhones retain horizontal expansion. Existing controllers, input, external data and Duo layouts remain available. See [release notes](docs/RELEASE-v0.3.63.zh-CN.md).

## Downloads

[Releases](https://github.com/huajiaoshu520/acnh-mobile/releases) attaches only the **LiveContainer + SideStore LAN host IPA** from v0.3.90 onward. The game-free shell is no longer attached. It contains no game program, game data, translated game shaders, keys, saves or fonts. The project app icon is the sole artwork exception. A shell alone cannot play the game; importing `.data` alone is insufficient.

Use an Apple Silicon Mac and your own rewritten 3.0.3 modules, complete matching official iOS shader cache, RomFS and font to generate a private IPA. Sign the entire resulting package, including Guest.framework when using shell assembly. Do not upload private, game-containing IPA files.

Build 92 synchronizes the generic runtime and shader tools with the pinned NXNative baseline: MetalFX upscaling, thermal pacing, expanded controller rumble, offline Metal pipeline archives, GPU diagnostics and newer graphics/movie handling. ACNH controls, adaptive layout, LAN, photo export and cloud saves remain project adapters. Device-only behavior still requires device testing.

IPA and game data remain separate: the IPA contains the runtime and converted shaders; the existing compatible external `.data` package is reused. NSZ/XCZ conversion is supported by the local one-click tool. Optional offline pipeline targets can be selected with `--gpu g18p` (or `pso_gpus` in local configuration); omitted targets follow the official profile defaults.

## Existing build interface

```sh
./setup.sh build \
  --patched /path/to/owned/patched \
  --romfs /path/to/owned/RomFS \
  --shaders /path/to/owned/shaders/ios \
  --font /path/to/owned/font.ttf \
  --work /path/to/private/work \
  --output /path/to/private/ACNHMobile.ipa
```

The resource pack remains outside the IPA. Existing 3.0.3 `ACNH-index.data` and numbered `.data` parts remain compatible and are read directly without extraction. Place the complete set in `Documents/ACNHMobile`, or choose its folder in the app. First import verifies file hashes; later starts reuse the verified index identity.

Build 96 synchronizes texture image-write preservation, layered clears, pipeline warmup concurrency, audio events, filesystem/save error handling, Mac display/gamepad fixes, package scanning and the Xcode 27.2 shader toolchain. Existing compatible external data remains reusable; phone behavior requires device verification.

## Validation

The private embedded build and game-free shell compile in Release. The shell has been audited for an exact file allowlist, absent game sections, version/build, iOS minimum, Metal 4 runtime and checksums. Local assembly with user-owned inputs passes. Mac startup reached the airport; filesystem, save, text, amiibo and photo regressions passed. Full phone gameplay, physical-controller behavior, keyboard interaction, NFC and Photos permission behavior still need device testing. No cooling or full-game stability claim is made.

## GitHub builds

Actions → **Validate and publish ACNH Mobile** → Run workflow on `main`. Leave **publish** off for artifacts only; enable it to create the release matching `config/version.json` after successful build and audits. A `v*` tag also builds and publishes. Existing releases are never overwritten. This edition publishes one iOS 26 shell; historical releases and compatibility branches remain available.

Credits and licenses: [NXNative](https://github.com/seedds/nxnative), [D2R Mobile](https://github.com/jhaizhou-ops/d2r-mobile), [LICENSE](LICENSE), [third-party notices](THIRD_PARTY_NOTICES.md). Not affiliated with Nintendo.

## Local launchers

Download and extract the complete project, then double-click a root `.command` launcher to create an iPhone IPA, an IPA with external data, or a standalone Mac app. Choose your game folder, prod.keys and output folder in the three dialogs; no terminal commands or configuration edits are required. Dependencies are checked automatically, and generated build caches are cleaned after successful output validation. See the [local launcher guide](docs/ONE-CLICK.zh-CN.md).

## Mac app

Build a standalone `.app` with `./setup.sh mac-app`, using your own game and keys or matching prepared Mac inputs. [Chinese tutorial](docs/MAC-APP.zh-CN.md).

### External DLC in one-click builds

Place your own DLC NSPs beside the base and 3.0.3 update before running the one-click tool. It extracts DLC with the official extractor, includes DLC resources in shader preparation, and outputs an `aoc` folder beside the IPA for both iOS build modes. Copy the entire `aoc` folder to the app’s `Documents` and restart. Keep `Documents/ACNHMobile` for the existing `.data` pack; DLC is separate. Adding/removing DLC invalidates preparation caches. The output `aoc` is a complete replacement set (including an empty set when no DLC is selected), not a merge. Mac app builds include the extracted DLC. No game files or keys are published.

DLC may also be placed under `game-directory/dlc/` (case-insensitive folder name; nested folders supported). Only DLC packages belong there; unrelated build/cache folders are not scanned.

## Portrait companion (local build 121)

iPad portrait and detected foldable portrait displays keep the complete 16:9 game above a six-page read-only companion: Map, Pockets, Critterpedia, DIY, Today and NookPhone. Touch controls occupy the lower area; the bottom page tabs remain available. Returning to landscape or background pauses companion refresh. Existing external `.data` files are reused. Physical iPad and folded Duo behavior still requires device validation.

The companion is adapted from [AnimalDuo](https://github.com/ShawndYang/AnimalDuo) and Eden Duo. Their GPL-3.0-or-later notices are retained in `dm/third_party/eden_duo`; combined distributions including this companion follow GPL-3.0-or-later. Original NXNative files retain their original notices. No game assets are supplied by the companion source.
