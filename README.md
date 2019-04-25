# ViGEm Bus Driver

Windows kernel-mode driver emulating well-known USB game controllers.

[![Build status](https://ci.appveyor.com/api/projects/status/rv74ufluwib52dq2?svg=true)](https://ci.appveyor.com/project/nefarius/vigembus) [![Discord](https://img.shields.io/discord/346756263763378176.svg)](https://discord.gg/QTJpBX5)  [![Website](https://img.shields.io/website-up-down-green-red/https/vigem.org.svg?label=ViGEm.org)](https://vigem.org/) [![PayPal Donate](https://img.shields.io/badge/paypal-donate-blue.svg)](<https://paypal.me/NefariusMaximus>) [![Support on Patreon](https://img.shields.io/badge/patreon-donate-orange.svg)](<https://www.patreon.com/nefarius>) [![GitHub followers](https://img.shields.io/github/followers/nefarius.svg?style=social&label=Follow)](https://github.com/nefarius) [![Twitter Follow](https://img.shields.io/twitter/follow/nefariusmaximus.svg?style=social&label=Follow)](https://twitter.com/nefariusmaximus)

## About

**Disclaimer:** this project is for software developers. To make it do something useful you'll also need a [feeder application](<https://docs.vigem.org/#!vigem-feeder.md>).  

The `ViGEmBus` driver and `ViGEmClient` libraries represent the core of the Virtual Gamepad Emulation Framework (or `ViGEm` , for short). `ViGEm` aims for a 100% accurate [emulation](<https://en.wikipedia.org/wiki/Emulator>) of well-known gaming peripherals as pure software-based devices at kernel level. As it mimics "the real thing" games and other processes require no additional modification whatsoever to detect `ViGEm`-based devices (no Proxy-DLLs or API-Hooking) and simply work out of the box. While the (now obsolete) [Scarlett.Crush Productions Virtual Bus Driver](<https://github.com/nefarius/ScpVBus>) is the spiritual father of this project, `ViGEm` has been designed and written from the ground up utilizing Microsoft's [Kernel-Mode Driver Framework](https://en.wikipedia.org/wiki/Kernel-Mode_Driver_Framework).

### Emulated devices

Currently supports emulation of the following USB Gamepads:

- [Microsoft Xbox 360 Controller](https://en.wikipedia.org/wiki/Xbox_360_controller)
- [Sony DualShock 4 Controller](https://en.wikipedia.org/wiki/DualShock#DualShock_4)

## Use cases

A few examples of the most common use cases for `ViGEm` are:

- You have an unsupported input device you'd like to use within games without modifying said game.
- You want the freedom to use a different controller of your choice in [PS4 Remote Play](<https://remoteplay.dl.playstation.net/remoteplay/>).
- You encountered a game not compatible with [x360ce](<https://www.x360ce.com/>).
- You want to extend the reach of your input device (like send traffic to a different machine over a network).
- You want to test/benchmark your game and need a replay mechanism for your user inputs.
- You want to work around player slot assignment order issues in `XInput`.

## Supported Systems

The driver is built for Windows 7/8/8.1/10 (x86 and amd64).

## How to build

### Prerequisites

- Visual Studio **2017** ([Community Edition](https://www.visualstudio.com/thank-you-downloading-visual-studio/?sku=Community&rel=15) is just fine)
- [WDK for Windows 10, version 1803](https://developer.microsoft.com/en-us/windows/hardware/windows-driver-kit)
- [.NET Core SDK 2.1](https://www.microsoft.com/net/download/dotnet-core/2.1) (or greater, required for building only)

You can either build directly within Visual Studio or in PowerShell by running the build script:

```PowerShell
.\build.ps1 -configuration release
```

Do bear in mind that you'll need to **sign** the driver to use it without [test mode](<https://technet.microsoft.com/en-us/ff553484(v=vs.96)>).

## Contribute

### Bugs & Features

Found a bug and want it fixed? Open a detailed issue on the [GitHub issue tracker](../../issues)!

Have an idea for a new feature? [Check out the project board](https://projects.vigem.org/public/board/27281599595f5fbe5f915884fb9ca2de92726e74173f1ac434300b2d40af), maybe it's already on there! If not, let's have a chat about your request on [Discord](https://discord.vigem.org) or the [community forums](https://forums.vigem.org).

### Questions & Support

Please respect that the GitHub issue tracker isn't a helpdesk. We offer a [Discord server](https://discord.vigem.org) and [forums](https://forums.vigem.org), where you're welcome to check out and engage in discussions!

## Installation

To grab the latest signed binaries for use or redistribution [follow the installation instructions](<https://docs.vigem.org/#!vigem-bus-driver-installation.md>).

## Sponsors

Sponsors listed here have helped the project flourish by either financial support or by gifting licenses:

- [3dRudder](https://www.3drudder.com/)
- [Parsec](https://parsecgaming.com/)
- [Rainway, Inc](https://rainway.io/)
- [JetBrains](https://www.jetbrains.com/resharper/)

## Known users of ViGEm

A brief listing of projects/companies/vendors known to build upon the powers of ViGEm. This list is non-exhaustive, if you'd like to see your project included, contact us!

- [3dRudder](https://www.3drudder.com/)
- [Parsec](https://parsecgaming.com/)
- [GloSC](https://github.com/Alia5/GloSC)
- [UCR](https://github.com/Snoothy/UCR)
- [InputMapper](https://inputmapper.com/)
- [Oculus VR, LLC.](https://www.oculus.com/)
- [Rainway, Inc](https://rainway.io/)
- [WiimoteHook](https://forum.cemu.info/showthread.php/140-WiimoteHook-Nintendo-Wii-Remote-with-Motion-Rumble-and-Nunchuk-support)
- [XJoy](https://github.com/sam0x17/XJoy)
- [HP](https://www8.hp.com/us/en/campaigns/gamingpcs/overview.html)
- [DS4Windows](https://ryochan7.github.io/ds4windows-site/)
- [XOutput](https://github.com/csutorasa/XOutput)
