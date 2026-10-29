# v0.3.69 · Build 78

- Integrate upstream alpha testing and its separate shader libraries.
- Integrate texture-copy storage mirroring, pending-readback tracking and hashed CPU-write tracking to preserve copied content.
- Correct sparse texture layouts, small mip tails and texture reads from guest threads.
- Correct RGB565/RGB5A1/RGB5 channel order across uploads, copies and readbacks.
- Correct compressed texture first use, minimal storage layouts and incompatible texture placeholders, including the required upstream prerequisites.

Retains the current LAN implementation, iPhone/iPad display modes, controllers, text input, photo saving and one-click build flow. Player synchronization still requires physical-device testing.

When running through LiveContainer + SideStore, install the LAN-capable host attached to this release and allow Local Network access. No host update is needed if you already use the identical patched build. Direct sideloading of the game does not require this host.

Public assets contain the game-free shell, LiveContainer + SideStore host, audits and checksums, without game data or keys. Actual phone behavior still requires device testing.
