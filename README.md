# ACNH Mobile

Current version: **0.1.21 (build 34)**. Download the two game-free editions from [Releases](https://github.com/huajiaoshu520/acnh-mobile/releases).

- **ACNHMobile-iOS26**: mainline edition, minimum iOS 26.
- **ACNHMobile-iOS18-compat**: Metal 3 compatibility edition, minimum iOS 18.

Both editions retain the intended graphics quality and features. The installation minimum does not mean every supported device has been tested. Public IPA downloads are game-free shells and require locally prepared owned runtime files.

This version includes a sampler lifetime fix verified on an Apple M4 with Metal API Validation. Shovel discoloration/dark ground edges and sustained phone heating/frame drops remain unresolved. The long-duration 4/32 MiB upload comparison is incomplete; no stability or cooling advantage is claimed for 32 MiB.

Experimental native compatibility layer for Animal Crossing: New Horizons **3.0.3** on iPhone / iPad, with an Apple Silicon Mac development host. Based on the open-source D2R Mobile compatibility layer, with ACNH-specific graphics, input, storage and system-service support.

****Public downloads contain source/tools and a game-free runtime shell. No game program, playable game assets, translated game shaders, keys or saves are distributed. The bundled app-identification icon is the sole artwork exception.**** Users prepare all game-derived runtime files locally from their own files. A locally generated IPA embeds the user's game modules and translated shaders; it is not part of the source-only release.

[简体中文（完整说明）](README.zh-CN.md) · [Build](docs/BUILD.md) · [Game files](docs/GAME_FILES.md) · [Release contents](docs/DERIVED.md)

Local preparation, 2026-10-22. This branch is the iOS18-compat Metal 3 edition, with an iOS 18 installation minimum; this does not mean every device has been tested. This is experimental; complete gameplay and all device configurations have not been verified.

Current changes include direct `.data` access, save compatibility, local mailbox support, map alpha tests, vertex position invariance, native text input, touch controls, 30 fps/720p defaults, host timezone handling and bounded iPhone haptic feedback. Phone haptic feel still needs device verification.

Latest fixes (2026-10-20): local screenshot service for shutter crashes, and a targeted workaround for yellow shoreline splash prepasses. Mac shoreline regression captured 300 frames with no yellow artifacts versus 34 affected frames in the baseline. iOS Release compilation passed; device verification is still required. See the [maintenance log](docs/CHANGELOG.zh-CN.md).

GitHub shell builds and local IPA builds use the bundled project icon automatically; no `DM_ICON` setting is required.

## Download or build a game-free shell

Open **Actions → Build game-free iOS shell → Run workflow**. The artifact contains both unsigned, game-free IPA editions, an audit report and `SHA256SUMS.txt`. Version tags (`v*`) publish one formal Release only after both builds and audits pass. The first main push at 0.1.21/build34 also starts that publication automatically; an existing version is never overwritten. Manual runs with both tag fields empty produce artifacts only.

The shell is **not directly playable or installable**. Use your own game files on an Apple Silicon Mac to prepare the program and shaders, then assemble and sign a private IPA. Existing `.data` resources stay external and are read without extraction. See [Shell guide](docs/SHELL.md) for the exact steps and current verification limits.

## Build locally

On an Apple Silicon Mac, install Xcode with Metal tools, XcodeGen, .NET 10, spirv-cross, zstd and Python 3.10+, then:

```sh
python3 -m venv .venv
. .venv/bin/activate
python -m pip install -r requirements.txt
./setup.sh check
```

Follow [BUILD](docs/BUILD.md) to prepare your own decrypted ExeFS/RomFS, translate shaders and build a private unsigned IPA. The tools do not obtain game files or keys. Publish only audited sources and game-free shells, not locally generated game-containing builds.

Credits: [D2R Mobile](https://github.com/jhaizhou-ops/d2r-mobile), the MIT upstream compatibility layer, and the projects credited in [THIRD_PARTY_NOTICES](THIRD_PARTY_NOTICES.md). Preserve [LICENSE](LICENSE) and all third-party licenses. This project is not affiliated with Nintendo.
