# v0.3.84 · Build 93

- Synchronize the shared runtime, graphics layer and tools with NXNative f11018a1f312 while preserving ACNH-specific integrations.
- Add optional MetalFX upscaling (off by default), official thermal frame limiting and recovery, additional rumble backends and DS4 output handling.
- Connect NSZ/XCZ decompression to the local build tool, and support GPU-matched offline Metal pipeline archive creation and optional packaging. Synchronize GPU analysis and diagnostics; diagnostics remain off by default.
- Synchronize official texture upload, copy and readback paths, resource residency, memory protection, shader translation, movie decoding and related fixes.
- Explain unsupported BC texture capabilities before launch. This message does not add support for unsupported devices.
- Fix controller settings: remove the accidentally displayed condition text and show the customization target and position/size editor only when Custom is selected.
- Preserve dedicated/generic/custom controls, amiibo file loading, native text input, photo export, LAN, iCloud folder sync, adaptive iPhone/iPad presentation and Duo test adaptations.

IPA and external .data remain separate; compatible existing data packages can be reused. Public shells contain no game resources, complete private game package or keys.

Mac/iOS compilation, relevant tool and service regressions, necessary GPU checks, shader compilation for both platforms and a saved-game startup test passed. Mac testing is not phone validation; AirPlay, physical controllers and real cross-device iCloud still require device testing. Offline pipelines are optional build inputs and are not bundled by default for every device.

LiveContainer + SideStore users must install the attached LAN host and allow local network access. The same patched host does not need reinstalling. Direct game sideloads do not require this host.

Public assets: game-free shell IPA, LiveContainer + SideStore host IPA, audit report and SHA256 checksums.
