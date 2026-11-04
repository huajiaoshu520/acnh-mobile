# v0.3.119 (128)

- Keep iPhone landscape-only; correct default landscape controller placement and adaptive viewport selection on iPhone/iPad. Preserve portrait companion mode on iPad/Duo.
- Compress private IPA packages while retaining separate game data.
- Adopt official Mac Globe/Fn letter-shortcut/key-release handling and motion-input finite-value guards.
- Avoid oversized shader compiler argument lists. Batch and merge offline pipeline compilation and stream its configuration to reduce memory use.
- Repair PFS0 reads with a missing final filename terminator and recognize unframed pointer-thunk tail calls.
- Add half-precision vertex formats and unsigned byte indices; correct depth/stencil clears and render-pass labels.
- Fix movie decoder constructor aliases and initial CPU upload tracking for compressible textures.

Pipeline, GPU, motion, file-reading, rewriting and display-layout regressions passed. An isolated Mac run completed 1800 frames and closed. iOS compilation, compressed package integrity, version and checksum checks passed. Physical iPad/Duo behavior remains unverified.

Custom and standard controls, native input, photos, volume, background-audio mixing, networking and cloud saves remain available. Compatible existing data packages can be reused.

The sole public attachment is the LiveContainer + SideStore LAN host IPA. Install it and allow Local Network access when using this host. The same patched host does not need updating again. Directly sideloaded games do not require it.
