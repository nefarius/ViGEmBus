# ViGEm Client Native SDK

Developer SDK for communication with `ViGEmBus`.

[![Build status](https://ci.appveyor.com/api/projects/status/k806d3m2egjr0j56?svg=true)](https://ci.appveyor.com/project/nefarius/vigemclient) [![Discord](https://img.shields.io/discord/346756263763378176.svg)](https://discord.vigem.org)  [![Website](https://img.shields.io/website-up-down-green-red/https/vigem.org.svg?label=ViGEm.org)](https://vigem.org/) [![PayPal Donate](https://img.shields.io/badge/paypal-donate-blue.svg)](<https://paypal.me/NefariusMaximus>) [![Support on Patreon](https://img.shields.io/badge/patreon-donate-orange.svg)](<https://www.patreon.com/nefarius>) [![GitHub followers](https://img.shields.io/github/followers/nefarius.svg?style=social&label=Follow)](https://github.com/nefarius) [![Twitter Follow](https://img.shields.io/twitter/follow/nefariusmaximus.svg?style=social&label=Follow)](https://twitter.com/nefariusmaximus)

## About

TBD

## How to build

### Dependencies

- [Boost.Asio](https://www.boost.org/doc/libs/1_66_0/doc/html/boost_asio.html)

### Prerequisites

- Visual Studio **2017** ([Community Edition](https://www.visualstudio.com/thank-you-downloading-visual-studio/?sku=Community&rel=15) is just fine)
- [.NET Core SDK 2.1](https://www.microsoft.com/net/download/dotnet-core/2.1) (or greater, required for building only)
- [Vcpkg Quick Start]
  - `.\vcpkg.exe install boost-asio:x86-windows-static boost-asio:x64-windows-static`

You can either build directly within Visual Studio or in PowerShell by running the build script:

```PowerShell
.\build.ps1
```

## Contribute

### Bugs & Features

Found a bug and want it fixed? Open a detailed issue on the [GitHub issue tracker](../../issues)!

Have an idea for a new feature? [Check out the project board](https://projects.vigem.org/public/board/27281599595f5fbe5f915884fb9ca2de92726e74173f1ac434300b2d40af), maybe it's already on there! If not, let's have a chat about your request on [Discord](https://discord.vigem.org) or the [community forums](https://forums.vigem.org).

### Questions & Support

Please respect that the GitHub issue tracker isn't a helpdesk. We offer a [Discord server](https://discord.vigem.org) and [forums](https://forums.vigem.org), where you're welcome to check out and engage in discussions!
