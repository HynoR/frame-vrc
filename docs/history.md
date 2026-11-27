# Release History

**English** · [中文](history.zh-CN.md) · [日本語](history.ja.md)

| Version | Date | Changes |
|---|---|---|
| 0.8.1-rc1 | 2026-11-27 | Pre-release. Setup is now an 8-step guided wizard in the panel: Steam's install prompts for FEX-Emu, Proton Experimental and the EasyAntiCheat runtime open one at a time (SteamVR showed only the last one), with reopen, retry and resume. Synced [ji10me's runtime 0.3.7](https://github.com/ji10me/SteamFrame_Standalone_PCVR_VRChat): fsync/esync/ntsync off to stop VRChat freezing at login or when joining a world, and the audio fix kept on Proton Experimental's 2026-11-10 update. |
| 0.8.0 | 2026-11-09 | Initial public release. Integrates [ji10me's PCVR runtime 0.3.6](https://github.com/ji10me/SteamFrame_Standalone_PCVR_VRChat) (eye-tracked DFR, H.264 hardware video decoding, Wine audio fix, Vulkan thunks, and VRChat thread affinity); standalone headset-only installer; isolated VRChat-only patched FEX environment; one-tap switching between official and PCVR builds in the VR dashboard panel; `dfrtest` diagnostic command. |
| 0.1–0.7 | 2026-11-03 – 11-06 | Pre-release builds: initial EAC-patched FEX, DFR integration, and VR dashboard panel prototypes. |
