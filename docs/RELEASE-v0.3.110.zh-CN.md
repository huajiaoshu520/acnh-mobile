# 0.3.110 · 构建 119

- 同步 NX 官方图元 ID 管线修复及依赖。相关片元程序使用完整 Metal 管线，避免错误的图元 ID。
- 同步运行时预构建和离线管线的排除规则，重新生成匹配的着色器反射缓存。
- 保留竖屏副屏、自定义按键、画面适配、输入法、联机和外置数据流程。已有兼容 `.data` 可继续使用。

LiveContainer + SideStore 用户需安装发布附件的联机宿主并允许本地网络；相同修补版无需重复更新。直接侧载游戏无需宿主。

# English

- Integrates the official NX primitive-ID pipeline fix and its reflection and whole-pipeline dependencies. Affected fragment shaders use complete Metal pipelines to preserve correct primitive IDs.
- Updates runtime prebuild and offline archive exclusions and regenerates matching shader reflection caches.
- Preserves portrait companion pages, custom controls, adaptive presentation, native input, LAN and external data. Compatible existing `.data` packages remain reusable.

LiveContainer + SideStore users need the attached LAN-enabled host and local-network permission. The same patched host does not need reinstalling. Direct sideloading does not require this host.

Device-specific behavior remains subject to physical-device verification.
