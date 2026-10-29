# v0.3.67 · 构建 76

- 新增实验性本地联机，支持同一 Wi-Fi 下发现房间和连接另一台使用本项目联机实现的设备。
- 修复联机初始化、网络错误码、房间信息及本地玩家游玩记录相关问题。
- 联机实现归入动森专属补丁，一键生成 iPhone IPA 和 Mac 应用时自动带入；保留原应用名称、标识及现有画面与控制设置。

已有真机建房、访客加入及存档成功记录，完整双机进岛、离岛与存档一致性仍待进一步验证。目前为两节点实验，不支持与原版 Switch 或 Nintendo Online 联机。已有 3.0.3 `.data` 可继续使用。

## LiveContainer + SideStore 用户必读

**使用 LiveContainer + SideStore 运行游戏的用户，需要更新到本页附件 `LiveContainer-3.8.11+SideStore-ACNH-LAN-unsigned.ipa`，并允许 LiveContainer 使用本地网络，才能正常使用本地联机。**

这个宿主包已补充动森需要的 Bonjour 服务声明 `_acnhldn._udp`。仅更新动森游戏包无法补齐宿主声明；已安装相同修补版的用户无需重复更新。宿主包为未签名版本，请使用自己的 SideStore 签名配置更新安装，保留原有应用数据。

直接通过 SideStore 安装 ACNH Mobile 的用户，不需要额外安装 LiveContainer，只需允许 ACNH Mobile 使用本地网络。

公开游戏附件仅包含无游戏空壳 IPA、审计报告和 SHA256 校验文件；另附上述 LiveContainer 宿主修补包及其 SHA256 校验文件。完整游戏 IPA、游戏数据、着色器、keys、字体及存档不上传。宿主修补包由 LiveContainer + SideStore 构建包调整服务声明而来，并非 LiveContainer 官方新增版本。
