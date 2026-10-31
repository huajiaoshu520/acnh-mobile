# NX generic synchronization / 通用同步

Pinned official source: `f11018a1f3125ec3046c30faae6147956a1a7c60`.

通用运行核心、图形层、着色器翻译、影片解码、震动后端及诊断工具以此官方版本为基线。动森控制器与编辑器、画面适配、LAN、系统输入、相册、外置数据、云存档文件夹入口及设备提示保留为项目适配。Mac/iOS 使用同一通用核心。

- MetalFX：启动设置中的画面放大选项；默认关闭，设备不支持时按官方路径回退。
- 温控：采用官方温控及恢复逻辑，仅在官方帧率控制适用的路径生效；不额外修改动森的游戏帧率。
- NSZ/XCZ：本地一键工具在 Mac 上解压到私有缓存，再进行原有资源处理。原始输入保持不变。
- 震动：同步官方后端与强度/阈值设置；具体手柄和平台能力仍以系统支持为准。
- 离线管线：一键工具支持 `--gpu g18p`，或本地配置 `pso_gpus`；省略时采用官方 profile 默认值。归档必须与随包着色器及 GPU 架构匹配。
- GPU 分析：同步官方计数、时序及捕获工具，诊断默认不启用。

**IPA 与 `.data` 分离**：IPA 保留运行程序、字体与转换后的着色器，不嵌入 RomFS、DLC 或 keys。兼容的已有 `.data` 直接复用。

The generic runtime, graphics and shader tools, movie decoding, rumble backends and diagnostics follow the pinned official source. ACNH-specific controls/editor, presentation, LAN, native input, photo export, external data, cloud-folder picker and device messages remain integration adapters. Optional features retain official defaults and applicability. Phone-only behavior still needs device testing.

The IPA and game data stay separate. Compatible existing external `.data` packages are reused; game resources and keys are not distributed with the public IPA.
