# ACNH Mobile 0.3.86 · Build 95

Updates NXNative to `5474c6f34789a41770868564fbf4e9e32fc57584`, including the local build 94 changes and this build 95 update.

## Changes

- Correct NVN buffer offset/size fields, render attachment capacity and combined depth/stencil state.
- Adopt Metal 4 default function constants, concurrent fallback and precompiled host Metal shaders while retaining Duo secondary display.
- Wait for Mac window activation and Space readiness before requesting startup fullscreen.
- Find external iOS DLC in Documents first and replace complete DLC sets during transfer to prevent stale content.
- Restore Vorbis movie audio after seeks/loops and move diagnostic logging off the real-time audio callback.
- Improve parallel rewriting, guarded zero-extended jump-table recognition, stale shader-cache reporting, polling regression tests and benchmark pipeline-archive handoff.
- Hide pipeline prebuild when Metal 4 is disabled. Preserve ACNH controls/editor, native input, Photos, LAN, cloud saves and presentation adapters.

The IPA and external `.data` remain separate. Compatible existing data packages can be reused. Existing adaptive iPhone, iPad and Duo support is retained.

## Validation and installation

Mac/iOS compilation, source audit, relevant GPU/audio/movie/tool regressions and startup checks with copied saves passed. Phone behavior, physical controllers, AirPlay and real cross-device iCloud sync still require device testing. This does not add older-device or iOS 18 support. Optional offline pipeline archives are not bundled by default.

Public assets contain only the game-free shell IPA, audit/checksums and the LiveContainer + SideStore LAN host IPA with its own SHA256. The shell cannot run the game by itself.

**LiveContainer + SideStore users:** install the attached LAN-enabled host and allow local network access. No reinstall is needed if the same patched host is already installed. Directly sideloaded game apps do not require this host.
