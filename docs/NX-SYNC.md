# NX generic synchronization / 通用同步

Pinned official source: `742c9fb83c3719674720ede99da6c1bc997904da`.

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

Build 95 / 构建95：同步官方全屏就绪等待、iOS DLC查找与替换传输、Vorbis Flush恢复、音频回调诊断迁移、零扩展跳转表识别、轮询测试和基准管线归档传递。未纳入其他游戏profile说明。

Build 96 / 构建96：采用 Xcode 27.2 与对应 Metal 着色器链；同步纹理 image 写入保护、数组/立方体多层清屏、并发管线预热、等待线程临时对象释放、音频命令验证和独立事件句柄、云存档错误后的流关闭、目录分配/递归删除/APFS 错误提示、呈现帧率及可变刷新判断、Mac HID 手柄回退/指针隐藏/显示唤醒/HUD、合并 NSP/XCI 内容识别与构建空间检查。保留 DLC 子目录与外置交付；私人 .data 准备明确提取真实目录。设备能力检查在 Play 后、创建游戏 Metal 视图前执行，保留系统语言提示。其他游戏 profile 与资源不纳入。

Build 98 / 构建98：同步官方尾调用栈帧证明和 Heaps PAK 着色器缓存提取工具，未纳入其他游戏 profile 或资源。动森21项改写产物与上一版逐字节一致。构建97已采用官方dad38db6云存档文件夹选择器。

Build 99 / 构建99：在742c9fb8基线上选取cd859c38损坏NSP提取/标记/日志修复，以及f634de70的预热完成队列初始化修复。其余Switch 2架构、无keys目录打包与后台库预取未合入，等待新功能确认；未将扫描到的cd859c38全量树视为已合入。

Build 101 / 构建101：检查cd859c38至372a8d8a0404bd0d0bdc9928a674b0d9c0104b5e的7提交，选择同步音频释放通知竞态、guest内存解除映射实际归还、Mac启动页立即关闭、BGRA/BGRX影片输出、Bundle 2范围/解压长度校验，以及UE区域二次设置参数的改写修复和对应测试。未纳入其他游戏profile/资源，未将整个官方树视为已同步；保留build100自适应装修菜单修复。未修复自适应室外快跑的边缘模型弹出问题。

Build 108 / 构建108：在退回build104之后，检查372a8d8a0404至c646d43d567f的18提交，按local/tests/nx108/applied.json选择13提交的适用文件与代码段；补入7313的profile_guess.depth_signal依赖。包括影片、线程/FS、缓存/提取/改写退出、音频事件边界和通用NVN查询/缓冲/渲染通路修复。c852仅同步nvn_mtl（排除依赖新链的dispatch first_store）、相应GPU测试及音频设备事件边界；未同步其NVN2 API、SDK接口、反射/翻译器/着色器链和其他游戏profile。未将18次扫描或13次选取称为整树合入。保留104自适应修复与现有官方alpha；

Build 118 / 构建118：审阅 c646d43d567f 至 de23ae82ad35453417ed4eda942d27d07e5d74df，实际选取 39448be6ae65, 54ab7b1059ea, 1ae435d5dbe3, 22458aa14d6d, e5620b1a05a5, 4a2a81a64f02, dac4a433b3b0, 6136418bcd56, e58e4467d37e, 73f4a48b3bb7, 50675d513ce8, de23ae82ad35。按适用文件合入，非整树同步。primitive-ID 预制管线修复依赖新的反射与整管线实现，测试编译发现缺少 reads_prim_id 后撤下；新翻译链、其他游戏支持及 AV1 回退未纳入。沿用匹配的 1edea0a2efd4 链。包含本地109–117的自适应界面、输入法、竖屏副屏和按键编辑器改动；未宣称修复室外快跑的边缘模型弹出。

Build 118 后续复核到 7d260eef62a8：追加 3104c0af621c 的提前密钥校验（保留本项目递归 DLC 扫描）与 7d260eef62a8 的深度平面编码线程复制。合计14项。primitive-ID 前置实现已在官方0a759de7864b中找到，但需匹配反射生成链；本次仍不声明合入该项。

Build 119 / 构建119：补入官方 fefb16d18757 的 primitive-ID 运行时预构建与离线管线排除规则，并按依赖选取 0a759de7864b 的反射标记生成/解析及整管线构建路径。读取 primitive ID 的片元程序使用完整管线，避免从开放颜色状态的基础管线特化后 ID 错误为零。仅选择这些依赖，不代表同步 0a759de7864b 整个提交。配套反射缓存重新生成，链更新为 55b6fe4e578d；保留现有翻译器版本与动森专属功能。

Build 124 / 构建124：检查至 50363b14ef9b19d0e8ade35d78812bdc8e7f8b96，按适用文件合入 fa681f94065e（watchdog 采样路径）、6a16b7d3c52d（CPU 写入 3D 纹理初次上传）、cd7f38032968（纹理描述符单次释放）、5a07ff2ce1ee（Mac 输入框焦点）、998896aca3bc（Mac 打包 AppleDouble 清理）、b3b38f4bf381（Mac 系统全屏快捷键）、229fc22cd8f5（稀疏纹理每次提交检查）、50363b14ef9b（3D 捕获深度切片）。非整树同步；配套着色器链仍为55b6fe4e578d。保留123副屏功能。


## 0.3.119 / 128

Checked upstream through 75e5ddd8046dc70f740780a14d336dc2f2cf1d7b; this is a reviewed subset, not a whole-tree merge. Imported 1b94fab2, 5e62e207, e2ab03aa, 65cc2af4, b1c20372, 6ddeda92, db8acc5c, de159bcb, 4e5582eb, 8fc76577, 79774e76 and 733dff9d. Existing matching shader chain remains unchanged. Generic CI profile fixtures are pinned to the reviewed revision in scratch storage and are not shipped as other game profiles. Includes local landscape/controller/adaptive-layout corrections from builds 125–127 and compressed packaging.

## 0.3.121 / 130

Reviewed upstream through 13da1c52e6578947b913c9a5bb904311734f298a. Imported the macOS DualShock 4 routing fix and regression tests from 02b0b662a603, and applied the renderer allocation checks from a27d684b1e64 to the matching tables in this tree. The paired shader chain remains 55b6fe4e578d.

## 0.3.122 / 131

Reviewed upstream through 975bb4c14456d145eb3d969814534eecdd996ce9. Imported the Mac app replacement fallback from 5061b2a174 and adapted the case-insensitive input scan from 7dc00e4d2b to preserve ACNH's DLC directory handling. The paired shader chain remains 55b6fe4e578d.

## 0.3.125 / 148

Reviewed NXNative through `865cc35fad70`. Synchronized the Mac offline pipeline prebuild worker selection from `72d005717b20` and its regression test. The game shader chain remains `55b6fe4e578d`.

## 0.3.126 / 149

Reviewed NXNative through `671c68e139ec`. Synchronized NVN event storage queries (`69cb9b6effee`), compatible cube and array texture views (`c83c48f2b47c`), and reuse of RomFS images after a volume remount (`a2d39a26306c`). Adapted the SDK audio lock lookup from `5219060397d2` to ACNH's single-renderer path while preserving the fallback. The shader chain remains `55b6fe4e578d`.

## 0.3.129 / 152

Reviewed NXNative through `b2d46b86d03f`. Synchronized Metal 4 specialization descriptor ownership (`88c7e03f51e3`), texture lookup ordering during memory pool destruction (`c229bd7d52e3`), and queue-owned render target descriptions (`48d0b372437d`). Adapted extracted RomFS Zstandard header checks from `6bbdbbe25511` to the existing NSP build path. The matching shader chain remains `55b6fe4e578d`.

## 0.3.133 / 156

Reviewed NXNative through `d4c11e2b8472`. Adapted stale texture view invalidation (`0e4480d263ad`) and deferred uniform address bounds (`f09f721c763d`) to the existing renderer. Synchronized the queue-owned render target check (`362f77257b1e`) and graphics test host stubs (`060aef96ddfa`). The matching shader chain remains `55b6fe4e578d`.

## 0.3.134 / 157

Reviewed NXNative through `bd979ced3ae6`. Synchronized kernel synchronization wait ownership and wake recording (`db206a46045f`), fixed-refresh fullscreen pacing (`490859ea7d6a`), caller tool path precedence (`2bd920a9ef17`), and test paths containing spaces (`12ddb1fb75f0`). Adapted shader-code address checks without GPU residency (`f25fa9562ee9`) to the existing memory pool table. The matching shader chain remains `55b6fe4e578d`.

Build 159 / 构建159：检查 NX bd979ced3ae6 至 62f6a52c4fa9。适配 a00f03bb3f72 的 Mac 原始 HID 设备去重，按 I/O Registry ID 合并重复发现，保留本项目的 GameController 优先路由。独立移植 ea0c98df35e2 的每队列渲染开关状态，保留现有命令调度和计数器实现。纳入已验证的本地通用横屏键位修正，保留 NX 默认位置并仅在实际遮挡时避让。配套着色器链继续使用 55b6fe4e578d。

Build 160 / 构建160：按 NX 通用横屏坐标修正左摇杆与方向键重叠。保留系统遮挡检测，圆形按键仅在实际碰到保留区域时避让。通用横屏中仅调整大小的自定义记录沿用默认位置，已有自定义坐标继续生效。
