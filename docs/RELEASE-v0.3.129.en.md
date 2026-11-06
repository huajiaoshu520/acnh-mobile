
## English

- Fixed ownership of Metal 4 pipeline specialization descriptors to avoid a crash before the first frame on iOS 26.0.
- Fixed a texture lookup race during memory pool destruction and kept render target descriptors alive while the GPU queue still uses them, preventing crashes when command memory is reused.
- NSP builds now check Zstandard headers in extracted RomFS files before rewriting the program and report damaged resources early.
- LiveContainer + SideStore users should install the attached LAN host and allow Local Network access. The same patched host can be reused. Direct sideloads of the game do not require the host.
