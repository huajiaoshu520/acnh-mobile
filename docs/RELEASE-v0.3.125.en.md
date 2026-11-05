## English

- The macOS offline pipeline prebuild tool now sizes its worker pool using Metal’s reported compiler concurrency. The manual worker-count override remains available.
- Devices with hardware BC texture support use the GPU path. Devices without it fall back to CPU decoding. A15/A16 runtime compatibility still needs device testing.
- LiveContainer + SideStore users should install the attached LAN host and allow Local Network access. If the same patched host is already installed, it can be reused. Direct sideloads of the game do not require the host.
