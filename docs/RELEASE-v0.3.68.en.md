# v0.3.68 · Build 77

- Ignore AppleDouble `._` sidecars when generating shader tables and prefilling the Metal cache on external drives.
- Apply upstream sampler LOD bias and invalidate argument blocks when a texture's Metal resource changes.
- Track zero-size GPU storage bindings as the rest of their buffer, avoiding unrelated uniform shadow refusals.
- Correct the clock-snapshot span boolean result and repeated audio-device notification acquisition result.
- Reactivate manually enabled motion sensors on connected physical controllers when returning from the background.

Retains the current LAN implementation, iPhone/iPad display modes, controllers, text input, photo saving and one-click build flow. Player synchronization still needs physical-device testing; this release does not claim to fix invisible visitors.

When running through LiveContainer + SideStore, install the LAN-capable host attached to this release and allow Local Network access. No host update is needed if you already use the identical patched build. Direct sideloading of the game does not require this host.

Local regressions, Metal graphics checks and iOS compilation passed. Public assets contain the game-free shell, LiveContainer + SideStore host, audits and checksums, without game data or keys.
