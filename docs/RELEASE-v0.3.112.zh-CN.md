# 0.3.112 · 构建 121

- 启动页版本下方自动检测版本，通过 jsDelivr 查询正式发布索引；本地版本大于或等于正式版显示已是最新版，较旧显示有更新，失败显示检测失败。
- 竖屏副屏手机页接入参考项目的普通按键导航，点击照相机可请求上屏打开游戏相机；不启用背包写入。
- 下屏使用原有主题加载页面，隐藏启动初期误报的版本提示；按 A 提示及点击入口依据上屏标题提示的实际可见状态启用。
- 保留构建 119 的官方管线修复和构建 120 的下屏背景修复。沿用外置数据。
- 本地测试包；真实 iPad／Duo 触控与时序待验证。CDN 索引需发布代码后上线。

# English

- Automatically checks for updates on launcher entry using a jsDelivr-hosted formal-release index, numeric version comparison and a failure status.
- Enables the reference companion's standard-button navigation for NookPhone actions such as opening the in-game camera. Inventory writes remain disabled.
- Uses themed lower-screen loading pages and suppresses transient startup version warnings and enables Press A only when the upper title prompt is visible.
- Retains build 119's official pipeline fixes and build 120's companion backdrop fixes. Existing external data remains compatible.
- Local test build; physical iPad/Duo touch and timing remain unverified. The CDN index becomes available after the source is published.
