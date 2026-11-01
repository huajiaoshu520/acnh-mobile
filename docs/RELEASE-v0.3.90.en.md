# ACNH Mobile 0.3.90 · Build 99

- Fix extraction of damaged NSPs exhausting the hash cache and crashing; list damaged files and retain the existing failure policy.
- Mark damaged files in RomFS images and identify them in runtime logs. Clean images retain compatibility.
- Initialize the completed warmup queue when an item arrives, preserving libraries completed before warmup starts.

Retain external .data and separate aoc delivery; existing data remains reusable. Mac/iOS builds and relevant regressions passed; phone behavior still needs device verification.

Only the LiveContainer + SideStore LAN host IPA is attached. Allow local network access when using it; the same patched host needs no reinstall. Direct sideloads need no host. Private game IPA and data are not public assets.
