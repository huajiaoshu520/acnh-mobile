# 本地联机

在机场选择本地联机，可发现同一 Wi-Fi 内运行本项目联机实现的另一台设备。目前后端为两节点实验实现，使用 Bonjour 发现房间，通过本地 TCP/UDP 传输游戏数据，不支持与原版 Switch 或 Nintendo Online 联机。

iPhone 首次使用时需允许本地网络权限。通过 LiveContainer 运行时，宿主本体还需声明 Bonjour 服务 `_acnhldn._udp`；仅更新游戏包不能补齐宿主声明。权限或网络初始化失败时会显示原因。

本地玩家记录标识和游玩记录保存在存档路径父目录的 `ACNHLocalNetwork` 文件夹，文件为 `identity.bin`、`play-history.bin`。这些是本项目的本地记录，不是 Nintendo 账号凭据。

联机实现来自独立实验的构建 75。已有真机建房、访客加入事件和存档提交记录，用户反馈可用；仅凭单端日志尚不能确认完整双机进岛、离岛和存档一致性。

合入时保留 ACNH Mobile 的名称、原应用标识和签名配置，不使用实验版 `org.acnhmobile.lan.experimental` 标识。原有画面、地图、自定义按键和输入功能保留。

实现归属：`dm/games/01006F8002326000/network/` 保存动森的联机服务、权限提示与诊断；`sources.mk` 仅在选择该游戏时替换对应服务单元。通用 `dm/host/hle_account.c`、`hle_friends.c`、`hle_misc.c`、`hle_net.c`、`hle_offline.c` 保留原实现。网络权限声明由同目录的 `platform_info.json` 提供，其他游戏不自动获得动森的服务声明。
