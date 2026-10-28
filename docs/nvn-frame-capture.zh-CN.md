# 本地 NVN 单帧诊断

默认关闭。在私有运行环境设置 `DM_NVN_CAPTURE=120:1:/absolute/private/frame.nvncap`，捕获指定帧。
iOS 可在 Documents/env.txt 中指定应用可写的绝对路径。启用捕获会等待 GPU 并增加磁盘、内存与运行耗时，仅用于诊断。

在同一源码、游戏与着色器链上编译：

```
make -C dm/gfx/tests GAME=01006F8002326000 nvn_replay
./dm/gfx/tests/nvn_replay /absolute/private/frame.nvncap --repeat 2
```

此适配保留当前移植项目的 50 操作记录布局（24 字节 pool_t）；与上游新布局不互通，工具会校验布局并拒绝不匹配文件。尚不支持稀疏虚拟池捕获。
捕获含游戏纹理、内存与可能的存档相关内容，仅本地保存，不提交 Git、不上传或附在公开发布中。

多手柄默认独立分配玩家；需要旧式全部合并输入时可设置 `DM_PAD_MERGE=1`。实际支持人数由游戏决定。
