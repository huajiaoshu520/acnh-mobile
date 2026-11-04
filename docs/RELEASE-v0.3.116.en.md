# v0.3.116 (125)

- Keep the game and custom-control editor landscape-only on iPhone, without the upper/lower companion layout.
- Retain the portrait companion and lower-screen control editing on iPad.
- Compress shader libraries when packaging the IPA to reduce archive size. IPA and game data remain separate; compatible existing data packages can be reused.

Passed iOS compilation, layout and control regressions, archive integrity and packaged orientation checks. Physical iPhone/iPad rotation behavior still requires device validation.

The only public attachment is the LiveContainer + SideStore LAN host. Install it and allow Local Network access when using this host. The same patched host does not need updating again. Directly sideloaded games do not require it.
