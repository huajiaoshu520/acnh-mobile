# v0.3.81 · Build 90

- Add upstream external TV/AirPlay output: a 16:9 game picture on the TV while touch controls remain on the phone; disconnecting restores the phone layout. Physical displays, AirPlay and LiveContainer multi-scene behavior still need device validation.
- Add Atmosphère cheat support. Mac has an upstream menu and management window. iPhone uses cheat files and env.txt configuration, has no graphical manager yet, and cannot patch signed executable pages. Cheats are disabled until configured.
- Add optional physical-input recording and frame-based replay: DM_REC=recording.txt to record and DM_KEYS=@recording.txt to replay. Off by default; use the same build and starting save.
- Fix present-snapshot texture residency and the race between memory protection and texture-pool retirement.
- Correct Nintendo Pro/Joy-Con face-button mapping and mounted vibration handles, with controller-applet focus notifications.
- Avoid an unbounded cloud-save wait if the download thread cannot be created; retain timeout handling and conflict backups.
- Integrate AudioOut large-stack calls, audio-renderer offset detection, old-SDK graphics-memory reporting, aligned buffer-texture copies and texture-handle parsing fixes.
- Include the iCloud folder-picker correction and adaptive UI exit backgrounds, text proportions, balloon and cursor positioning fixes.

Preserves LAN, special/generic/custom controllers, text input, photos, iPhone/iPad display modes, Duo test support and one-click builds.

Complete Mac/iOS builds and input, cheat, save-sync, notification, audio, UI and relevant graphics regressions passed. Mac startup is not phone validation. TV/AirPlay, physical controllers and cross-device iCloud still require device testing.

LiveContainer + SideStore users must install the attached LAN-enabled host and allow local network access. Users already running this same patched host do not need to reinstall it. Directly sideloaded games do not require the host.

Public attachments contain only the game-free shell IPA, LiveContainer + SideStore host IPA, audit and checksums. No game data or keys are included.
