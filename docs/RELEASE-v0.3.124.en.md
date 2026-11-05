
## 0.3.124 (145)

- Restore exclusive game audio on iOS. With background music mixing enabled, device testing reproduced crackling during villager dialogue or when changing system volume. Both scenarios sounded normal in the comparison build with mixing disabled.
- Restore the audio implementation from build 121 and remove the Game Volume slider from the launcher. The silent-mode playback setting remains available.

The release includes the LiveContainer + SideStore LAN host IPA. Allow local network access when using this host. You can keep the same patched host if it is already installed. Direct sideloading requires no host.
