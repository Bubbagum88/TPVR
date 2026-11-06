<div align="center">
  <img src="res/logo.png?v=2" alt="TPVR logo" width="640">
</div>

# TPVR for Quest 1

Twilight Princess in VR on the **original Oculus Quest (Quest 1)**, standalone.

This is a Quest 1 build of [TPVR](https://github.com/JoeyAW/TPVR) by **JoeyAW**, a VR mod for [Dusklight](https://twilitrealm.dev/), the Twilight Princess PC port. All the VR work is JoeyAW's. This fork only changes what the Quest 1 needs to start, load the disc and draw the game. It isn't affiliated with the Dusklight team.

> [!IMPORTANT]
> **Work in progress.** It plays, but the Quest 1's GPU struggles with this game and the frame rate drops, worst around water, mist and glow. See [Status](#status).

**No game content is included.** You need your own Twilight Princess GameCube disc image (USA, `GZ2E01`).

## What "Quest 1 support" means

Upstream TPVR ships only a Windows build. The Quest code in it was made for the Quest 3, and on a Quest 1 it doesn't start or draws nothing. This build changes:

- **The manifest lists the Quest 1** (`quest|quest2`). The Quest 1's VR runtime refuses an app whose manifest names a headset it doesn't know.
- **A 2D launcher panel** to choose the disc and allow file access, which then starts the game in VR. Started straight into VR, the setup screen can't be seen on the Quest 1.
- **Storage read access**, so the disc can be opened by its path. (Opening it through Android's file picker misread a good disc image.)
- **Workarounds for the Quest 1's GPU driver** (Adreno 540, `vulkan.msm8998.so`): a crash in `vkCmdDrawIndexed`, `arrayLength()` reading 0 for the second storage buffer (every model collapsed), and screen copies coming out as one flat colour.
- **VR > Performance > Menu Resolution** (25 to 100%, 50% by default): the menu layer is drawn at a fraction of the window size, which saved about 15 ms a frame.

The renderer fixes are patch files in `platforms/android/quest1/`, applied by the CI workflow.

## Status

Tested on one Quest 1 (firmware v50) with release **quest1-v1.0**: the game starts from the launcher, loads the disc and plays in VR.

- **Frame rate:** normal areas take about 22 ms a frame to draw. Water, mist and glow push that much higher, about 69 ms at the Ordon spring. Lowering **VR Render Resolution** helps; it only applies after restarting the game. Making those effects cheaper is the next piece of work.
- That headset is rooted; nothing in the port uses root, but it hasn't been tried on an unrooted Quest 1.

## Requirements

- **Firmware:** tested on Quest 1 firmware **v50** only. Older firmware is untested.
- **Android:** built for Android 9 and up (API 28). The Quest 1 on v50 runs Android 10.
- **Graphics:** Vulkan, through Dawn. The Quest 1's driver reports Vulkan 1.1.
- **OpenXR:** the Khronos OpenXR loader 1.1.63, built from source and bundled in the APK, uses the headset's own OpenXR runtime.

## Install

You need Developer Mode on the headset and either [SideQuest](https://sidequestvr.com/setup-howto) or `adb` (Android platform-tools) on your PC.

1. Download the APK from the [quest1-v1.0 release](https://github.com/Bubbagum88/TPVR/releases/tag/quest1-v1.0) and save it as `TPVR-quest1-v1.0.apk`.
2. Install it with SideQuest's **Install APK** button, or with adb.
3. Copy your disc image to `/sdcard/Android/data/com.joeyaw.tpvr/files/GZ2E01.iso`.
   **Uninstalling TPVR deletes this folder**, so copy the disc again after any uninstall.
4. In the headset, open **TPVR** from **App Library > Unknown Sources**. In the launcher panel choose the disc image, tap **Allow file access**, then **Play**.

### The same steps with adb

In PowerShell, in your platform-tools folder (where `adb.exe` is), with the APK and your disc image saved there as `TPVR-quest1-v1.0.apk` and `GZ2E01.iso`:

```powershell
.\adb install TPVR-quest1-v1.0.apk
.\adb shell mkdir -p /sdcard/Android/data/com.joeyaw.tpvr/files
.\adb push GZ2E01.iso /sdcard/Android/data/com.joeyaw.tpvr/files/GZ2E01.iso
```

## Known issues

- **Low frame rate**, as above.
- **Leaving the game can crash it** (pausing to the Quest menu, or the headset going to sleep): recreating the display surface fails on the Quest 1.
- **Signed with a public test key** (in this repository), so CI builds install over each other. Windows and Google have flagged the APK; this is believed to be a false positive from that key, but it hasn't been confirmed.

If something breaks, please [open an issue](https://github.com/Bubbagum88/TPVR/issues) with a log:

```powershell
.\adb logcat -d > tpvr-log.txt
```

## Controls

The controls are upstream TPVR's Quest bindings, shown below. The Quest 1's Touch controllers have the same buttons as the Quest 2's.

<div align="center">
<img src="Graphic design/TPVR controls.png" alt="TPVR controls" width="1920">
</div>

## Building

The [Quest 1 APK workflow](.github/workflows/quest1-apk.yml) builds the APK on GitHub Actions on every push to this branch (or by hand from the **Actions** tab). The APK is in the run's `tpvr-quest1-apk` artifact. For other platforms, see upstream's [build instructions](docs/building.md).

## Credits

- **JoeyAW**: [TPVR](https://github.com/JoeyAW/TPVR). Join JoeyAW's [Discord](https://discord.gg/CxQJ9PjnjA) or support the work on [Patreon](https://www.patreon.com/cw/JoeyAW). JoeyAW's statement on AI-generated code is in [upstream's README](https://github.com/JoeyAW/TPVR#statement-on-ai-generated-code).
- The [Dusklight](https://twilitrealm.dev/) team and all [Dusklight contributors](https://github.com/TwilitRealm/dusklight/graphs/contributors), the [Twilight Princess decompilation](https://github.com/zeldaret/tp) team and the GC/Wii decompilation community, the [Aurora](https://github.com/encounter/aurora) developers, and everyone else in [upstream's credits](https://github.com/JoeyAW/TPVR#credits).
- Quest 1 build: **Bubbagum88**.

This is a fan project, not affiliated with or endorsed by Nintendo. The Legend of Zelda and Twilight Princess are trademarks of Nintendo. See [LICENSE.md](LICENSE.md).

<br/>
<div align="center">
    <a href="https://github.com/encounter/aurora">
        <img src="assets/aurora-powered.png" alt="Powered by Aurora" width="800">
    </a>
</div>
