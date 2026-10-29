# v0.3.74 · Build 83

- Add optional iCloud Drive save sharing. Choose the same folder in the iPhone and Mac launchers. Saves are checked at launch and uploaded after commits. Disabled until configured; replaced saves are backed up first. If both devices changed, the upstream rule keeps the newer save. Wait for synchronization before continuing on the other device.
- Integrate the upstream render-pass retry when texture argument binding ends the encoder, avoiding draws on an ended encoder.
- Bound compressed game-title inflation and handle short input.
- Require Capstone 5 explicitly in the rewriter.
- Correct adaptive-mode held-item positioning, item-name balloon proportions and storage action balloons covering the selected item.

Preserves LAN, iPhone/iPad display modes, controllers, text input, photos, Duo test support and one-click builds. This release does not add an A16 BC texture fallback.

Save-sync core regressions and complete Mac/iOS builds passed. Real iCloud Drive synchronization between an iPhone and Mac still needs device validation, as do phone rendering and LAN behavior.

LiveContainer + SideStore users must install the attached LAN-enabled host and allow local network access. Users already running this same patched host do not need to reinstall it. Directly sideloaded games do not require the host.

Public attachments contain only the game-free shell, the LiveContainer + SideStore host, audit and checksums. No game data or keys are included.
