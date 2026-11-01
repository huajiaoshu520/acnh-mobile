# ACNH Mobile 0.3.87 · Build 96

- Adopt image-write preservation, layered clears, concurrent pipeline warmup and GPU-wait temporary cleanup fixes.
- Fix audio command validation/event handles, stream closure after cloud-save metadata write failures, directory allocation/deletion and save filesystem errors.
- Synchronize present caps and variable-refresh/latency decisions; fix Mac HID fallback, cursor hiding on game input, active-window display wakefulness and Metal HUD hiding.
- Apply the iOS performance HUD setting before the first Metal device, retaining localized device messages.
- Adopt merged NSP/XCI discovery, offline archive GPU selection and disk checks, using Xcode 27.2 and the matching Metal shader chain.
- One-click builds accept DLC beside the game or under dlc/DLC, deliver separate aoc resources, and extract a real RomFS directory for .data preparation. Existing compatible .data and the delivered aoc remain reusable.

Retain ACNH controller layouts/editor, native input, Photos, personalization, LAN, iCloud folder selection and existing iPhone/iPad/Duo display adapters. Mac/iOS builds, graphics/service regressions and an independent-save Mac visual check through 1,800 frames passed. Phones, physical controllers and cross-device cloud sync still need device testing.

LiveContainer + SideStore users need the attached LAN host and local network permission. The same patched host needs no reinstall. Direct sideloads need no host. Public assets contain only the game-free shell, audit/checksums and the LAN host with its checksum; no game content or keys.
