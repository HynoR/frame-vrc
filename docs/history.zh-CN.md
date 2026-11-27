# 版本历史

[English](history.md) · **中文** · [日本語](history.ja.md)

| 版本 | 日期 | 更新内容 |
|---|---|---|
| 0.8.1-rc1 | 2026-11-27 | 预发布版。配置改为面板里的 8 步向导：FEX-Emu、Proton Experimental、EasyAntiCheat 运行时的 Steam 安装窗口改为逐个打开（之前 SteamVR 只显示最后一个），支持重新打开安装窗口、重试和中断后继续。同步 [ji10me 的运行方案 0.3.7](https://github.com/ji10me/SteamFrame_Standalone_PCVR_VRChat)：关闭 fsync/esync/ntsync，修复 VRChat 在登录或进入世界时卡住；Proton Experimental 在 2026-11-10 更新后音频修复继续生效。 |
| 0.8.0 | 2026-11-09 | 首个正式版。集成 [ji10me 的 PCVR 运行方案 0.3.6](https://github.com/ji10me/SteamFrame_Standalone_PCVR_VRChat)（眼动注视点渲染 DFR、H.264 硬件视频解码、Wine 音频修复、Vulkan thunk 适配、VRChat 线程亲和性优化）；支持头显本地免 PC 安装；提供 VRChat 专属私有 FEX 环境；支持通过 VR 面板一键切换官方版与 PCVR 版；新增 `dfrtest` 调试命令。 |
| 0.1–0.7 | 2026-11-03 – 11-06 | 早期预览版：验证 EAC 补丁版 FEX、DFR 与 VR 面板原型。 |
