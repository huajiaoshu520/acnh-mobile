# v0.3.109 (118)

This release includes the previously completed local UI and portrait changes.

- Adaptive-mode corner HUD and loading hints follow the extended viewport. Interior arrangement controls, text-entry screens, bottom guides and nested backgrounds receive additional coverage, including animated position changes.
- Native keyboard text submission and input state after reconnection are corrected.
- Portrait iPad and hinge-capable Duo use an upper full-game view and a lower six-page island companion, with refreshed lifecycle and update handling.
- The custom controller editor follows device orientation. Portrait editing uses the lower screen beneath a game preview. iPad and Duo portrait layouts are stored separately; drafts survive rotation and reset affects only the current layout.

Fourteen official fixes are included:
- Empty vertex streams no longer block geometry draw runs.
- Safe texture references after memory-pool destruction.
- Stripped Unity 2020.3 ComputeShader layout recognition.
- Three rewriter fixes for register-write identity, overwritten jump-table indices/base registers, and indices changed after bounds checks.
- Bounded SARC member-name scanning.
- Exact file reads with ReadOption.
- Shared depth storage and channel swizzling for red-channel texture views.
- SDK filesystem unmount releases its mount-table entry.
- Odd presentation intervals use a 60 Hz display callback.
- NFC library state before initialization.
- Encode-thread recording of depth-plane copies with blit options.
- Early package-key checks with actionable missing-key and firmware messages.

Separate IPA/data packaging, dedicated controls, DLC, local networking, photo export and cloud-save workflows remain supported. Compatible existing data packages can be reused.

LiveContainer + SideStore users should install the attached LAN host and allow Local Network access. The same patched host does not need updating again. Directly sideloaded games do not require it.

Local regressions and compilation do not establish physical-device validation. iPad/Duo rotation, hinge updates and touch behavior still need device confirmation.
