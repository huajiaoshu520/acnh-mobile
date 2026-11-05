
## 0.3.123 (138)

- Fix persistent audio crackling that can occur after adjusting system volume on iOS. The audio buffer adapts to the system callback size.
- Improve audio recovery when returning to the game. The output prebuffers before playback resumes, and device underruns are tracked separately from the silent background sink.

The release includes the LiveContainer + SideStore LAN host IPA. Allow local network access when using this host. You can keep the same patched host if it is already installed. Direct sideloading requires no host.
