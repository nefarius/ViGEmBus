# ViGEm Bus Driver

Windows kernel-mode driver emulating well-known USB game controllers.

[![Build status](https://ci.appveyor.com/api/projects/status/rv74ufluwib52dq2?svg=true)](https://ci.appveyor.com/project/nefarius/vigembus) [![GitHub All Releases](https://img.shields.io/github/downloads/ViGEm/ViGEmBus/total)](https://somsubhra.github.io/github-release-stats/?username=nefarius&repository=ViGEmBus) [![Discord](https://img.shields.io/discord/346756263763378176.svg)](https://discord.nefarius.at) [![Website](https://img.shields.io/website-up-down-green-red/https/vigem.org.svg?label=docs.nefarius.at)](https://docs.nefarius.at/) [![GitHub followers](https://img.shields.io/github/followers/nefarius.svg?style=social&label=Follow)](https://github.com/nefarius) [![Mastodon Follow](https://img.shields.io/mastodon/follow/109321120351128938?domain=https%3A%2F%2Ffosstodon.org%2F&style=social)](https://fosstodon.org/@Nefarius)

---

## 🧟 THIS PROJECT HAS BEEN RETIRED 🧟

Users of this software are encouraged to [read the end-of-life statement](https://docs.nefarius.at/projects/ViGEm/End-of-Life/). So long, cheers 🖖

---

## About

The `ViGEmBus` driver and `ViGEmClient` libraries represent the core of the Virtual Gamepad Emulation Framework (or `ViGEm` , for short). `ViGEm` aims for a 100% accurate [emulation](<https://en.wikipedia.org/wiki/Emulator>) of well-known gaming peripherals as pure software-based devices at kernel level. As it mimics "the real thing" games and other processes require no additional modification whatsoever to detect `ViGEm`-based devices (no Proxy-DLLs or API-Hooking) and simply work out of the box. While the (now obsolete) [Scarlett.Crush Productions Virtual Bus Driver](<https://github.com/nefarius/ScpVBus>) is the spiritual father of this project, `ViGEm` has been designed and written from the ground up utilizing Microsoft's [Kernel-Mode Driver Framework](https://en.wikipedia.org/wiki/Kernel-Mode_Driver_Framework).

### Emulated devices

Emulation of the following USB Gamepads is supported:

- [Microsoft Xbox 360 Controller](https://en.wikipedia.org/wiki/Xbox_360_controller)
- [Sony DualShock 4 Controller](https://en.wikipedia.org/wiki/DualShock#DualShock_4)

## Use cases

A few examples of the most common use cases for `ViGEm` are:

- You have an unsupported input device you'd like to use within games without modifying said game.
- You want the freedom to use a different controller of your choice in [PS4 Remote Play](<https://remoteplay.dl.playstation.net/remoteplay/>).
- You encountered a game not compatible with [x360ce](<https://www.x360ce.com/>) (prior to version 4.x).
- You want to extend the reach of your input device (like send traffic to a different machine over a network).
- You want to test/benchmark your game and need a replay mechanism for your user inputs.
- You want to work around player slot assignment order issues in `XInput`.

## Supported Systems

🛑 **Windows Server** might work but is **not supported** 🛑

Bug reports/support requests regarding running on a Server OS will be discarded.

### Version 1.16 and below

The driver is built for Windows 7/8.1/10 (x86 and amd64).

### Version 1.17 and above

The driver is built for Windows 10/11 only (x86, amd64 and ARM64).

## License

The ViGEm Bus Driver is licensed under the **BSD-3-Clause**, see [LICENSE](./LICENSE.md) for more information.

## How to build

### Prerequisites

- [Step 1: Install Visual Studio 2019](https://docs.microsoft.com/en-us/windows-hardware/drivers/other-wdk-downloads#step-1-install-visual-studio)
- [Step 2: Install WDK for Windows 10, version 2004](https://docs.microsoft.com/en-us/windows-hardware/drivers/other-wdk-downloads#step-2-install-the-wdk)
- [Step 3: Clone the Driver Module Framework (DMF)](https://github.com/microsoft/DMF) into the same parent directory.
  - Build the `DmfK` project with Release and Debug configurations for all architectures (x64 and Win32).

You can build directly within Visual Studio.

Do bear in mind that you'll need to **sign** the driver to use it without [test mode](https://docs.microsoft.com/en-us/windows-hardware/drivers/install/the-testsigning-boot-configuration-option#enable-or-disable-use-of-test-signed-code).

## Contribute

### Bugs & Features

Found a bug and want it fixed? Open a detailed issue on the [GitHub issue tracker](../../issues)!

Have an idea for a new feature? Let's have a chat about your request on [Discord](https://discord.nefarius.at).

### Questions & Support

Please respect that the GitHub issue tracker isn't a helpdesk. We offer a [range of support resources](https://docs.nefarius.at/Community-Support/) you're welcome to check out!

## Installation

Pre-built production-signed binaries **for Windows 10/11** are provided by `Nefarius Software Solutions e.U.` [as an all-in-one setup](../../releases/latest).

## Sponsors

Sponsors listed here have helped the project flourish by either financial support or by gifting licenses:

- [3dRudder](https://www.3drudder.com/)
- [Parsec](https://parsec.app/)
- [JetBrains](https://www.jetbrains.com/resharper/)
- [Advanced Installer](https://www.advancedinstaller.com/)
- [ICAROS](https://www.icaros.com/)

## Known users of ViGEm

A brief listing of projects/companies/vendors known to build upon the powers of ViGEm.

This list is non-exhaustive, if you'd like to see your project included, contact us!

- [3dRudder](https://www.3drudder.com/)
- [Parsec](https://parsec.app/)
- [GloSC](https://github.com/Alia5/GloSC)
- [UCR](https://github.com/Snoothy/UCR)
- [InputMapper](https://inputmapper.com/)
- [Oculus VR, LLC.](https://www.oculus.com/)
- [WiimoteHook](https://forum.cemu.info/showthread.php/140-WiimoteHook-Nintendo-Wii-Remote-with-Motion-Rumble-and-Nunchuk-support)
- [XJoy](https://github.com/sam0x17/XJoy)
- [HP](https://www8.hp.com/us/en/gaming/omen.html)
- [DS4Windows](https://ryochan7.github.io/ds4windows-site/)
- [XOutput](https://github.com/csutorasa/XOutput)
- [RdpGamepad](https://github.com/microsoft/RdpGamepad)
- [Touchmote](https://github.com/Ryochan7/Touchmote/tree/ryochan7)
- [Mi-ViGEm](https://github.com/grayver/mi-vigem)
- [BetterJoy](https://github.com/Davidobot/BetterJoy)
- [Regame](https://github.com/ksyun-kenc/liuguang)
- [NetInput](https://github.com/usertoroot/NetInput)
- [NetJoy](https://github.com/Qcent/NetJoy/)
