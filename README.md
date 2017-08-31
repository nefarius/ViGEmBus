# ViGEm Bus Driver

Currently supports emulation of the following USB gamepads:
- [Microsoft Xbox 360 Controller](https://en.wikipedia.org/wiki/Xbox_360_controller)
- [Sony DualShock 4 Controller](https://en.wikipedia.org/wiki/DualShock#DualShock_4)
- [Microsoft Xbox One Controller](https://en.wikipedia.org/wiki/Xbox_One_Controller)
  - Experimental; not ready for stable release yet

## Necessary preparations for Windows 7
Before installing the bus driver on Windows 7 (x86 or x64) the following 3rd party software has to be installed:
 * [Xbox 360 Accessories Software 1.2](https://www.microsoft.com/accessories/en-us/products/gaming/xbox-360-controller-for-windows/52a-00004#techspecs-connect) (contains the missing device drivers)
 * [Microsoft Security Advisory 3033929 Update](https://technet.microsoft.com/en-us/library/security/3033929) has to be installed to support the drivers signature. Download links:
   * [Security Update for Windows 7 (KB3033929)](https://www.microsoft.com/en-us/download/details.aspx?id=46078)
   * [Security Update for Windows 7 for x64-based Systems (KB3033929)](https://www.microsoft.com/en-us/download/details.aspx?id=46148)

## Manual Installation
```
devcon.exe install ViGEmBus.inf Root\ViGEmBus
```

## Manual Removal
```
devcon.exe remove Root\ViGEmBus
```
