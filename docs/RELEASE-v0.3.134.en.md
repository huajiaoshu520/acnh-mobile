## English

- Fixed kernel synchronization waits. An active wait retains the original object and records its wake signal even if the signal is subsequently cleared or the handle is closed.
- Fixed 30 fps fullscreen presentation pacing on fixed-refresh displays. The display link stays at 60 Hz while the present interval paces the game at 30 fps.
- Hashing shader keys on the CPU checks the registered memory range without adding the pool to the GPU residency set.
- NSP builds prioritize tools from the caller's PATH. Graphics and sanitizer regression targets now support checkout paths with spaces.
- LiveContainer + SideStore users should install the attached LAN host and allow Local Network access. The same patched host can be reused. Direct sideloads of the game do not require the host.
