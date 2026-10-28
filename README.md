# ACNH Mobile

**0.3.64 · build 67 — official Metal 4 edition, iOS 26+.**

This revision extends the top and bottom decorative backgrounds on ABD, Nook Miles and shopping screens in iPhone adaptive mode while retaining menu proportions. [Release notes](docs/RELEASE-v0.3.64.zh-CN.md).
A native compatibility runtime for Animal Crossing: New Horizons 3.0.3. The runtime and shader translation sources are based on [NXNative](https://github.com/seedds/nxnative) commit `e72a06510e6dfff4a77c49fd205a82e21cad0786`.

This edition uses the official Metal 4 replay path and matching shader chain. It retains ACNH's icon touch controls, tool wheel, physical controllers, R/field-triggered Chinese text input, direct external `.data` access, transactional saves, amiibo file import, Photos export and bounded haptics. Custom touch layouts can be edited from startup settings; there is no added in-game typing toolbar.

[简体中文](README.zh-CN.md) · [Shell guide](docs/SHELL.md) · [Build guide](docs/BUILD.md)

Adaptive expanded view preserves horizontal FOV and reveals more vertical scenery on landscape iPads; wide iPhones retain horizontal expansion. Existing controllers, input, external data and Duo layouts remain available. See [release notes](docs/RELEASE-v0.3.63.zh-CN.md).

## Downloads

[Releases](https://github.com/huajiaoshu520/acnh-mobile/releases) contains an **unsigned, game-free shell IPA**, audit report and SHA256 checksums. It contains no game program, game data, translated game shaders, keys, saves or fonts. The project app icon is the sole artwork exception. A shell alone cannot play the game; importing `.data` alone is insufficient.

Use an Apple Silicon Mac and your own rewritten 3.0.3 modules, complete matching official iOS shader cache, RomFS and font to generate a private IPA. Sign the entire resulting package, including Guest.framework when using shell assembly. Do not upload private, game-containing IPA files.

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

## Validation

The private embedded build and game-free shell compile in Release. The shell has been audited for an exact file allowlist, absent game sections, version/build, iOS minimum, Metal 4 runtime and checksums. Local assembly with user-owned inputs passes. Mac startup reached the airport; filesystem, save, text, amiibo and photo regressions passed. Full phone gameplay, physical-controller behavior, keyboard interaction, NFC and Photos permission behavior still need device testing. No cooling or full-game stability claim is made.

## GitHub builds

Actions → **Build game-free iOS shell** → Run workflow on `main`. Leave **publish** off for artifacts only; enable it to create the release matching `config/version.json` after successful build and audits. A `v*` tag also builds and publishes. Existing releases are never overwritten. This edition publishes one iOS 26 shell; historical releases and compatibility branches remain available.

Credits and licenses: [NXNative](https://github.com/seedds/nxnative), [D2R Mobile](https://github.com/jhaizhou-ops/d2r-mobile), [LICENSE](LICENSE), [third-party notices](THIRD_PARTY_NOTICES.md). Not affiliated with Nintendo.

## Local launchers

Download and extract the complete project, then double-click a root `.command` launcher to create an iPhone IPA, an IPA with external data, or a standalone Mac app. Choose your game folder, prod.keys and output folder in the three dialogs; no terminal commands or configuration edits are required. Dependencies are checked automatically, and generated build caches are cleaned after successful output validation. See the [local launcher guide](docs/ONE-CLICK.zh-CN.md).

## Mac app

Build a standalone `.app` with `./setup.sh mac-app`, using your own game and keys or matching prepared Mac inputs. [Chinese tutorial](docs/MAC-APP.zh-CN.md).
