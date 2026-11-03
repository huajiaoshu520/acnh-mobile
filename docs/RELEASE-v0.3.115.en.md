# v0.3.115 (124)

- Fix initial uploads of CPU-written 3D textures and repeated texture descriptor retirement.
- Check sparse-texture CPU writes once per submission and reuse tracked memory spans.
- Include every 3D texture depth slice in frame captures.
- Preserve game focus around native Mac text input and consume the system Fn/Globe + F fullscreen shortcut.
- Remove exFAT/AppleDouble sidecars during Mac packaging and handle special characters in watchdog sample paths without a shell.

Companion interactions, adaptive display, controls, native input, photo export, local networking and cloud saves remain available. IPA and data remain separate; compatible existing data packages can be reused.

Mac compilation and an isolated run, texture/GPU/dirty-memory regressions, packaging and watchdog tests passed, together with iOS compilation and package validation. Mac execution is not physical iPhone/iPad validation.

The only public attachment is the LiveContainer + SideStore LAN host. Install it and allow Local Network access when using this host. The same patched host does not need updating again. Directly sideloaded games do not require it.
