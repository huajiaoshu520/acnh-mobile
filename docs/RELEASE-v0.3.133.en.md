## English

- Invalidate the cached view when a texture ID loses its backing texture, so the GPU cannot reuse a released texture.
- Check the address bounds of deferred uniform updates before looking up reads that cannot overlap any pending update.
- Verify queue-owned render targets by serial number and share host stubs across the NVN and graphics suspend tests.
- LiveContainer + SideStore users should install the attached LAN host and allow Local Network access. The same patched host can be reused. Direct sideloads of the game do not require the host.
