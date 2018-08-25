# ViGEm user-mode client library
This static library provides the gateway to the bus drivers functionalities. It offers:
 * Searching and connecting to a library-compatible bus device on the system
 * Attaching and removing a (artificially limited) number of virtual devices
 * Feeding the emulated devices (aka providing input state changes) 

## Dependencies
In addition to this library you'll need to link against `setupapi.lib`.