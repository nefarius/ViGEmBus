# ViGEm Bus Driver

Windows kernel-mode driver emulating well-known USB game controllers. 

[![Build status](https://ci.appveyor.com/api/projects/status/rv74ufluwib52dq2?svg=true
)](https://ci.appveyor.com/project/nefarius/indicium-supra) [![Discord](https://img.shields.io/discord/346756263763378176.svg)](https://discord.gg/QTJpBX5)  [![Website](https://img.shields.io/website-up-down-green-red/https/vigem.org.svg?label=ViGEm.org)](https://vigem.org/) [![PayPal Donate](https://img.shields.io/badge/paypal-donate-blue.svg)](<https://paypal.me/NefariusMaximus>) [![Support on Patreon](https://img.shields.io/badge/patreon-donate-orange.svg)](<https://www.patreon.com/nefarius>) [![GitHub followers](https://img.shields.io/github/followers/nefarius.svg?style=social&label=Follow)](https://github.com/nefarius) [![Twitter Follow](https://img.shields.io/twitter/follow/nefariusmaximus.svg?style=social&label=Follow)](https://twitter.com/nefariusmaximus)

## About
Currently supports emulation of the following USB gamepads:
- [Microsoft Xbox 360 Controller](https://en.wikipedia.org/wiki/Xbox_360_controller)
- [Sony DualShock 4 Controller](https://en.wikipedia.org/wiki/DualShock#DualShock_4)

## Necessary preparations for Windows 7
Before installing the bus driver on Windows 7 (x86 or x64) the following 3rd party software has to be installed:
 * [Xbox 360 Accessories Software 1.2](https://www.microsoft.com/accessories/en-us/products/gaming/xbox-360-controller-for-windows/52a-00004#techspecs-connect) (contains the missing device drivers)
 * [Microsoft Security Advisory 3033929 Update](https://technet.microsoft.com/en-us/library/security/3033929) has to be installed to support the drivers signature. Download links:
   * [Security Update for Windows 7 (KB3033929)](https://www.microsoft.com/en-us/download/details.aspx?id=46078)
   * [Security Update for Windows 7 for x64-based Systems (KB3033929)](https://www.microsoft.com/en-us/download/details.aspx?id=46148)

## Installation
[Follow the installation instructions](<https://vigem.org/wiki/vigem-bus-driver-installation/>).
