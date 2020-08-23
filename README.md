# ViGEm Client Native SDK

C/C++ developer SDK for communication with [`ViGEmBus`](https://github.com/ViGEm/ViGEmBus).

[![Build status](https://ci.appveyor.com/api/projects/status/k806d3m2egjr0j56?svg=true)](https://ci.appveyor.com/project/nefarius/vigemclient) [![Discord](https://img.shields.io/discord/346756263763378176.svg)](https://discord.vigem.org)

## About

**TL;DR:** use this if you want to create virtual game controllers from your C/C++ application 😊

The `ViGEmClient` provides a small library exposing a simple API for creating and "feeding" (periodically updating it with new input data) virtual game controllers through [`ViGEmBus`](https://github.com/ViGEm/ViGEmBus). The library takes care of discovering a compatible instance of the bus driver on the users system and abstracting away the inner workings of the emulation framework. You can use and distribute it with your project as either a static component (recommended) or a dynamic library (DLL). This library is **not** thread-safe, ensure proper synchronization in a multi-threaded environment.

## How to build

### Prerequisites

- Visual Studio **2019** ([Community Edition](https://www.visualstudio.com/thank-you-downloading-visual-studio/?sku=Community&rel=16) is just fine)
  - When linking statically, make sure to also link against `setupapi.lib`

## Contribute

### Bugs & Features

Found a bug and want it fixed? Open a detailed issue on the [GitHub issue tracker](../../issues)!

Have an idea for a new feature? Let's have a chat about your request on [Discord](https://discord.vigem.org) or the [community forums](https://forums.vigem.org).

### Questions & Support

Please respect that the GitHub issue tracker isn't a helpdesk. We offer a [Discord server](https://discord.vigem.org) and [forums](https://forums.vigem.org), where you're welcome to check out and engage in discussions!

## How to use

### Integration

Integrating this library into your project is pretty straight-forward, there are no additional 3rd party dependencies. You can either `git submodule` or `git subtree` this repository directly into your source tree or use the provided [`vcpkg`](https://github.com/microsoft/vcpkg) package manager integration [found here](https://github.com/ViGEm/ViGEmClient.vcpkg) (recommended, can be updates with ease). The library tries to handle driver compatibility internally so static linking is recommended to avoid DLL hell 😊

### API usage

For a general overview of the provided types and functions [take a look at the main include file](./include/ViGEm/Client.h).

Now, onwards to a practical example 😉 First, include some basic headers:

```cpp
//
// Windows basic types 'n' fun
//
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

//
// Optional depending on your use case
//
#include <Xinput.h>

//
// The ViGEm API
//
#include <ViGEm/Client.h>

//
// Link against SetupAPI
//
#pragma comment(lib, "setupapi.lib")
```

To initialize the API call `vigem_alloc` which gives you an opaque handle to the underlying driver:

```cpp
const auto client = vigem_alloc();

if (client == nullptr)
{
    std::cerr << "Uh, not enough memory to do that?!" << std::endl;
    return -1;
}
```

Establish connection to the driver:

```cpp
const auto retval = vigem_connect(client);

if (!VIGEM_SUCCESS(retval))
{
    std::cerr << "ViGEm Bus connection failed with error code: 0x" << std::hex << retval << std::endl;
    return -1;
}
```

👉 Note: this is an "expensive" operation, it's recommended you do this once in your project, not every frame for performance benefits.

---

With this handle we're prepared to spawn (connect) and feed (supply with periodic input updates) one or many emulated controller devices. So let's spawn an Xbox 360 controller:

```cpp
//
// Allocate handle to identify new pad
//
const auto pad = vigem_target_x360_alloc();

//
// Add client to the bus, this equals a plug-in event
//
const auto pir = vigem_target_add(client, pad);

//
// Error handling
//
if (!VIGEM_SUCCESS(pir))
{
    std::cerr << "Target plugin failed with error code: 0x" << std::hex << retval << std::endl;
    return -1;
}

XINPUT_STATE state;

//
// Grab the input from a physical X36ß pad in this example
//
XInputGetState(0, &state);

//
// The XINPUT_GAMEPAD structure is identical to the XUSB_REPORT structure
// so we can simply take it "as-is" and cast it.
//
// Call this function on every input state change e.g. in a loop polling
// another joystick or network device or thermometer or... you get the idea.
//
vigem_target_x360_update(client, pad, *reinterpret_cast<XUSB_REPORT*>(&state.Gamepad));

//
// We're done with this pad, free resources (this disconnects the virtual device)
//
vigem_target_remove(client, pad);
vigem_target_free(pad);
```

---

Alright, so we got the feeding side of things done, but what about the other direction? After all, the virtual device can receive some state changes as well (for the Xbox 360 device the LED ring can change and rumble/vibration requests can arrive) and this information is of interest for us. This is achieved by registering a notification callback like so:

```cpp
//
// Define the callback function
//
VOID CALLBACK notification(
    PVIGEM_CLIENT Client,
    PVIGEM_TARGET Target,
    UCHAR LargeMotor,
    UCHAR SmallMotor,
    UCHAR LedNumber,
    LPVOID UserData
)
{
    static int count = 1;

    std::cout.width(3);
    std::cout << count++ << " ";
    std::cout.width(3);
    std::cout << (int)LargeMotor << " ";
    std::cout.width(3);
    std::cout << (int)SmallMotor << std::endl;
}
```

Register it:

```cpp
const auto retval = vigem_target_x360_register_notification(client, pad, &notification, nullptr);

//
// Error handling
//
if (!VIGEM_SUCCESS(retval))
{
    std::cerr << "Registering for notification failed with error code: 0x" << std::hex << retval << std::endl;
    return -1;
}
```

The function `notification` will now get invoked every time a rumble request was sent to the virtual controller and can get handled accordingly. This is a blocking call and the invocation will take place in the order the underlying requests arrived.

---

Once ViGEm interaction is no longer required (e.g. the application is about to end) the acquired resources need to be freed properly:

```cpp
vigem_disconnect(client);
vigem_free(client);
```

After that the `client` handle will become invalid and must not be used again.
