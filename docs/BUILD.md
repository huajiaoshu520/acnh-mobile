# Private local build

Use an Apple Silicon Mac, Xcode 26 with Metal tools, XcodeGen and Python 3.10+ with FontTools. All game inputs must be your own. Neither the repository nor GitHub Actions obtains game files or keys.

```sh
python3 -m venv .venv
. .venv/bin/activate
python -m pip install -r requirements.txt
./setup.sh check
./setup.sh build \
  --patched /path/to/owned/patched \
  --romfs /path/to/owned/RomFS \
  --shaders /path/to/owned/shaders/ios \
  --font /path/to/owned/font.ttf \
  --work /path/to/private/work \
  --output /path/to/private/ACNHMobile.ipa
```

`--patched` contains official rewritten `main`, `subsdk0`, `sdk` images and their fixups/gates/cuts/obs tables. Supported 3.0.3 build IDs are recorded in `dm/games/01006F8002326000/game.tsv`. Use rewritten inputs matching this official source, not outputs from older project releases.

`--shaders` is the parent of the complete matching `b-<chain>` cache, including reflection JSON and shadow-input variants. The v0.3.8 geometry integration uses `b-ade9bce36598` with the pinned upstream tools. Rebuild older shader caches; external RomFS `.data` remains compatible. Do not mix old project shader patches, chain metadata or translation binaries with the official runtime. For fresh extraction/rewriting/translation, follow the official source procedure in the upstream [README](https://github.com/seedds/nxnative/tree/e72a06510e6dfff4a77c49fd205a82e21cad0786); per-game tools are retained under `dm/`. Translating with a different toolchain requires regenerating the chain header, Mac/iOS shaders, c1/constant-buffer tables together and rebuilding the runtime. The checked-in chain lock is provenance, not a redistributable translator binary.

RomFS is checked as a local input but stays external. Existing 3.0.3 `.data` files work. To create a new pack:

```sh
./setup.sh data --romfs /path/to/owned/RomFS --output /path/to/private/data
```

Copy all `.data` files, including `ACNH-index.data`, to `Documents/ACNHMobile`. No extraction occurs on the phone. The JSON manifest generated alongside the pack is a tooling record; the phone reads the `.data` index.

The full IPA is private and contains game-derived modules and shaders. Do not publish it. Sign it yourself; this project does not sign or install on devices.

The v0.3.14 map alpha-test build uses chain `3eca67439950`. Regenerate matching shaders and both constant-buffer tables, then rebuild the runtime. External `.data` is unchanged.

v0.3.15 uses chain `665e6ad0479d`. Generated `c1_full.tsv` and `cb_extent.tsv` are no longer committed. Raw preparation creates them automatically, including on shader-cache reuse. Prepared-input builds look for matching Mac outputs under the iOS cache parent’s `cache/` or `mac/`, or accept `--mac-shaders <parent-of-b-chain>`. Missing/stale tables are regenerated; without Mac outputs, the build stops with a preparation instruction. Generated tables are excluded from reuse keys.

### v0.3.16 Mac development mode

The one chain header is `dm/gfx/nvn/dmsh_chain.h` (currently `e90503c2d6f2`). A full shader build creates `inputs.tsv` and both matching tables. Rebuild older caches; never copy an old stamp to new outputs.

`./setup.sh mac --patched <prepared> --romfs <RomFS> --shaders <Mac-cache-parent> --font <ttf> --work <work>` builds the Mac host and writes `<work>/run-mac.command`; add `--run` to launch it. Saves stay under `<work>/run/save`. Raw preparation is available with `python3 tools/one_click.py ipa --platform mac --game-dir <NSP-directory> --keys <prod.keys>`. Mac mode creates no IPA, iOS project or Data package; the default remains iOS.

## Standalone Mac app

`./setup.sh mac-app` packs a double-clickable Apple silicon app using the upstream launcher. Raw game input and matching prepared Mac inputs are supported. See [Mac tutorial](MAC-APP.zh-CN.md) for commands, signing, saves and logs.

Duo testing releases use Xcode 27.1 / iOS 27.1 SDK (`xcode-27` on GitHub Actions) so the Duo adaptation is compiled in. The deployment target remains iOS 26.
