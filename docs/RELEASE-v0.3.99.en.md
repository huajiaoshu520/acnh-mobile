# v0.3.99 (108)

Updates the restored build 104 while preserving existing UI, controller layouts, external data, DLC, LAN and cloud saves.

Official fixes included in this update:
- H.264 presentation order when loop timestamps restart, and Vorbis output lengths at long/short window changes.
- File-service requests release same-core thread handoffs to avoid save-worker ordering races. Existing transactional commit and handle validation remain intact.
- Growing graphics query tables prevent dropped conditional-rendering results. Running buffer updates, same-target pass continuation, depth read/write boundaries and large-target render areas are handled more accurately.
- Repeated unimplemented-NVN logs and the SDK alias for audio-output device notifications.
- Content-based offline pipeline reuse, shader extraction cache identity, provisional-profile relocation, validated standard gzip extraction, Unity 6000.3 ComputeShader layouts, and depth-mode detection from the game's actual calls.
- Rewriter worker signal handling and pool shutdown races that could hang small jobs.


This release also includes the previously completed local fixes for audio-release notification races, returning unmapped memory pages, closing the Mac launcher after Play, BGRA/BGRX movie colors and Bundle 2 bounds; adaptive renovation-menu ghosting, background coverage, gradients, reveal transitions and loading decoration layout. Corner HUD repositioning is outside this release.

The existing matching shader chain and separate IPA/data workflow are preserved. Translator and compute-initialization changes requiring a different shader chain are outside this scoped update. No new device runtime support is claimed.

LiveContainer + SideStore users should install the attached LAN-enabled host and allow Local Network access. The same patched host does not need reinstalling. Directly sideloaded games do not require this host.

Compilation and local Mac tests do not establish iPhone/iPad validation. Check results are recorded locally.
