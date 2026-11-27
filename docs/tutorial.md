# Step-by-Step Tutorial

**English** · [中文](tutorial.zh-CN.md) · [日本語](tutorial.ja.md)

This guide walks through installing `frame-vrc` and running the PC version of VRChat (with EasyAntiCheat) on the Steam Frame. Setup takes about 10 minutes, plus a ~1.1 GB game update. Please read the disclaimer in the [README](../README.md) before proceeding.

## How Version Switching Works

`frame-vrc` switches between VRChat builds by configuring VRChat's Steam compatibility tool:

| Version | Steam Compatibility Tool |
|---|---|
| **Official** (Android build) | Default (Lepton) |
| **PCVR** (Windows build + DFR + H.264 hardware video) | Proton Experimental (Ji10me PCVR Edit) |

When you launch VRChat, Steam automatically updates the game to match the selected compatibility tool. The patched FEX environment is isolated to VRChat and does not affect Steam's built-in FEX-Emu or other games.

## 1. Install frame-vrc

In Konsole on the headset (or over SSH), run the following command as the normal user `steamos` (**never use `sudo`**):

```sh
curl -fsSL https://raw.githubusercontent.com/HynoR/frame-vrc/main/get.sh | bash
```

Installation takes about a minute. Once complete, open the VR panel: in the SteamVR dashboard, tap **+** (Launch Program) > **frame-vrc**.

![SteamVR dashboard: + (Launch Program) > frame-vrc](images/steamvr-launch-program.jpg)

## 2. Configure the PCVR Version

Open the **Install** page in the panel:

![Install page: Setup to PCVR Version](images/panel-install-setup.jpg)

1. Tap **Setup to PCVR Version** (green), then tap again to confirm. The **Install** page switches to the 8-step setup wizard (you can tap **View log** anytime to inspect raw output).
2. Missing Steam tools (FEX-Emu, Proton Experimental, Proton EasyAntiCheat Runtime) are requested **one at a time**. Confirm each Steam install prompt as it appears; if you closed a prompt by mistake, tap **Reopen Install Prompt**. The wizard waits automatically while Steam downloads.
3. At step 7, **Steam restarts once** if needed to register the new compatibility tool (the dashboard view will briefly disappear and resume automatically when you reopen the panel).
4. If a step fails or stops early, tap **Retry This Step** (or re-run `frame-vrc setup`) to continue from that step.

![Log page while setup runs](images/panel-log.jpg)

Once finished, if VRChat itself still needs to download or update to the PC build, the wizard shows that VRChat still needs install/update in Steam (or **patched: update and launch VRChat**).

## 3. Launch VRChat

Launch VRChat from the panel's **Home** page or from Steam. Steam will download the PC build (~1.1 GB) before starting the game. Joining your first world takes a few minutes for initial shader compilation; subsequent loads will be much faster.

![Steam updating VRChat to the PC build](images/steam-vrchat-updating.jpg)

Under **VRChat > Properties > Compatibility**, you can verify the configured compatibility tool (launch options are automatically set to `frame-vrc-run %command%`—do not edit them manually):

![Compatibility tool: Proton Experimental (Ji10me PCVR Edit)](images/compat-pcvr-version.jpg)

## 4. Panel Overview

Use the language button in the bottom-left corner to switch between English and 日本語. When VRChat is uninstalled or set to the official Android build, only **Home** and **Install** are shown; switching to the PCVR version unlocks all pages:

- **Home**: Displays memory/CPU usage, render resolution presets (`1056` / `1440` / `1920`, default `1920`), and **Launch VRChat**. Eye-tracked foveated rendering (DFR) and H.264 hardware decoding are always enabled.

  ![Home page](images/panel-home.jpg)

- **Environment**: Lists environment health checks with one-tap fix buttons for any detected issues.

  ![Environment page](images/panel-environment.jpg)

- **Advanced**: Shows installed component versions, additional resolution options, swap status, update, and uninstall controls.

  ![Advanced page](images/panel-advanced.jpg)

## 5. Switch Back to the Official Version

On the **Install** page, tap **Recover to Official Android Version** (red) and tap again to confirm. VRChat will revert to Lepton along with your previous launch options and resolution. Steam will download the Android build the next time you launch VRChat.

![Install page: Recover to Official Android Version](images/panel-install-recover.jpg)

![Compatibility tool back to Default (Lepton)](images/compat-official-lepton.jpg)

## Command-Line Interface

All panel actions are also available via the CLI:

```text
frame-vrc setup | official | status | doctor
frame-vrc resolution 1056|1440|1920
frame-vrc launch | update | uninstall [--purge]
```

See [Installation & CLI Reference](install.md) and [System Changes & Uninstallation](changes.md) for full details.
