/*
MIT License

Copyright (c) 2017-2019 Nefarius Software Solutions e.U. and Contributors

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/


#ifndef ViGEmClient_h__
#define ViGEmClient_h__

#ifdef __cplusplus
extern "C" {
#endif

#include "ViGEm/Common.h"

#ifdef VIGEM_DYNAMIC
#ifdef VIGEM_EXPORTS
#define VIGEM_API __declspec(dllexport)
#else
#define VIGEM_API __declspec(dllimport)
#endif
#else
#define VIGEM_API
#endif

    /**
     * \typedef enum _VIGEM_ERRORS
     *
     * \brief   Defines an alias representing the ViGEm errors.
     */
    typedef enum _VIGEM_ERRORS
    {
        VIGEM_ERROR_NONE = 0x20000000,
        VIGEM_ERROR_BUS_NOT_FOUND = 0xE0000001,
        VIGEM_ERROR_NO_FREE_SLOT = 0xE0000002,
        VIGEM_ERROR_INVALID_TARGET = 0xE0000003,
        VIGEM_ERROR_REMOVAL_FAILED = 0xE0000004,
        VIGEM_ERROR_ALREADY_CONNECTED = 0xE0000005,
        VIGEM_ERROR_TARGET_UNINITIALIZED = 0xE0000006,
        VIGEM_ERROR_TARGET_NOT_PLUGGED_IN = 0xE0000007,
        VIGEM_ERROR_BUS_VERSION_MISMATCH = 0xE0000008,
        VIGEM_ERROR_BUS_ACCESS_FAILED = 0xE0000009,
        VIGEM_ERROR_CALLBACK_ALREADY_REGISTERED = 0xE0000010,
        VIGEM_ERROR_CALLBACK_NOT_FOUND = 0xE0000011,
        VIGEM_ERROR_BUS_ALREADY_CONNECTED = 0xE0000012,
        VIGEM_ERROR_BUS_INVALID_HANDLE = 0xE0000013,
        VIGEM_ERROR_XUSB_USERINDEX_OUT_OF_RANGE = 0xE0000014,
		VIGEM_ERROR_INVALID_PARAMETER = 0xE0000015

    } VIGEM_ERROR;

    /**
     * \def VIGEM_SUCCESS(_val_) (_val_ == VIGEM_ERROR_NONE);
     *
     * \brief   A macro that defines success.
     *
     * \author  Benjamin "Nefarius" Höglinger
     * \date    28.08.2017
     *
     * \param   _val_   The VIGEM_ERROR value.
     */
#define VIGEM_SUCCESS(_val_) (_val_ == VIGEM_ERROR_NONE)

     /**
      * \typedef struct _VIGEM_CLIENT_T *PVIGEM_CLIENT
      *
      * \brief   Defines an alias representing a driver connection object.
      */
    typedef struct _VIGEM_CLIENT_T *PVIGEM_CLIENT;

    /**
     * \typedef struct _VIGEM_TARGET_T *PVIGEM_TARGET
     *
     * \brief   Defines an alias representing a target device object.
     */
    typedef struct _VIGEM_TARGET_T *PVIGEM_TARGET;

    typedef
        _Function_class_(EVT_VIGEM_TARGET_ADD_RESULT)
        VOID CALLBACK
        EVT_VIGEM_TARGET_ADD_RESULT(
            PVIGEM_CLIENT Client,
            PVIGEM_TARGET Target,
            VIGEM_ERROR Result
        );

    typedef EVT_VIGEM_TARGET_ADD_RESULT *PFN_VIGEM_TARGET_ADD_RESULT;

    typedef
        _Function_class_(EVT_VIGEM_X360_NOTIFICATION)
        VOID CALLBACK
        EVT_VIGEM_X360_NOTIFICATION(
            PVIGEM_CLIENT Client,
            PVIGEM_TARGET Target,
            UCHAR LargeMotor,
            UCHAR SmallMotor,
            UCHAR LedNumber
        );

    typedef EVT_VIGEM_X360_NOTIFICATION *PFN_VIGEM_X360_NOTIFICATION;

    typedef
        _Function_class_(EVT_VIGEM_DS4_NOTIFICATION)
        VOID CALLBACK
        EVT_VIGEM_DS4_NOTIFICATION(
            PVIGEM_CLIENT Client,
            PVIGEM_TARGET Target,
            UCHAR LargeMotor,
            UCHAR SmallMotor,
            DS4_LIGHTBAR_COLOR LightbarColor
        );

    typedef EVT_VIGEM_DS4_NOTIFICATION *PFN_VIGEM_DS4_NOTIFICATION;

    /**
     * \fn  PVIGEM_CLIENT vigem_alloc(void);
     *
     * \brief   Allocates an object representing a driver connection.
     *
     * \author  Benjamin "Nefarius" Höglinger
     * \date    28.08.2017
     *
     * \return  A new driver connection object.
     */
    VIGEM_API PVIGEM_CLIENT vigem_alloc(void);

    /**
     * \fn  void vigem_free(PVIGEM_CLIENT vigem);
     *
     * \brief   Frees up memory used by the driver connection object.
     *
     * \author  Benjamin "Nefarius" Höglinger
     * \date    28.08.2017
     *
     * \param   vigem   The driver connection object.
     */
    VIGEM_API void vigem_free(PVIGEM_CLIENT vigem);

    /**
     * \fn  VIGEM_ERROR vigem_connect(PVIGEM_CLIENT vigem);
     *
     * \brief   Initializes the driver object and establishes a connection to the emulation bus
     *          driver. Returns an error if no compatible bus device has been found.
     *
     * \author  Benjamin "Nefarius" Höglinger
     * \date    28.08.2017
     *
     * \param   vigem   The driver connection object.
     *
     * \return  A VIGEM_ERROR.
     */
    VIGEM_API VIGEM_ERROR vigem_connect(PVIGEM_CLIENT vigem);

    /**
     * \fn  void vigem_disconnect(PVIGEM_CLIENT vigem);
     *
     * \brief   Disconnects from the bus device and resets the driver object state. The driver object
     *          may be reused again after calling this function. When called, all targets which may
     *          still be connected will be destroyed automatically. Be aware, that allocated target
     *          objects won't be automatically freed, this has to be taken care of by the caller.
     *
     * \author  Benjamin "Nefarius" Höglinger
     * \date    28.08.2017
     *
     * \param   vigem   The driver connection object.
     */
    VIGEM_API void vigem_disconnect(PVIGEM_CLIENT vigem);

    /**
     * \fn  PVIGEM_TARGET vigem_target_x360_alloc(void);
     *
     * \brief   Allocates an object representing an Xbox 360 Controller device.
     *
     * \author  Benjamin "Nefarius" Höglinger
     * \date    28.08.2017
     *
     * \return  A PVIGEM_TARGET representing an Xbox 360 Controller device.
     */
    VIGEM_API PVIGEM_TARGET vigem_target_x360_alloc(void);

    /**
     * \fn  PVIGEM_TARGET vigem_target_ds4_alloc(void);
     *
     * \brief   Allocates an object representing a DualShock 4 Controller device.
     *
     * \author  Benjamin "Nefarius" Höglinger
     * \date    28.08.2017
     *
     * \return  A PVIGEM_TARGET representing a DualShock 4 Controller device.
     */
    VIGEM_API PVIGEM_TARGET vigem_target_ds4_alloc(void);

    /**
     * \fn  void vigem_target_free(PVIGEM_TARGET target);
     *
     * \brief   Frees up memory used by the target device object. This does not automatically remove
     *          the associated device from the bus, if present. If the target device doesn't get
     *          removed before this call, the device becomes orphaned until the owning process is
     *          terminated.
     *
     * \author  Benjamin "Nefarius" Höglinger
     * \date    28.08.2017
     *
     * \param   target  The target device object.
     */
    VIGEM_API void vigem_target_free(PVIGEM_TARGET target);

    /**
     * \fn  VIGEM_ERROR vigem_target_add(PVIGEM_CLIENT vigem, PVIGEM_TARGET target);
     *
     * \brief   Adds a provided target device to the bus driver, which is equal to a device plug-in
     *          event of a physical hardware device. This function blocks until the target device is
     *          in full operational mode.
     *
     * \author  Benjamin "Nefarius" Höglinger
     * \date    28.08.2017
     *
     * \param   vigem   The driver connection object.
     * \param   target  The target device object.
     *
     * \return  A VIGEM_ERROR.
     */
    VIGEM_API VIGEM_ERROR vigem_target_add(PVIGEM_CLIENT vigem, PVIGEM_TARGET target);

    /**
     * \fn  VIGEM_ERROR vigem_target_add_async(PVIGEM_CLIENT vigem, PVIGEM_TARGET target, PVIGEM_TARGET_ADD_RESULT result);
     *
     * \brief   Adds a provided target device to the bus driver, which is equal to a device plug-in
     *          event of a physical hardware device. This function immediately returns. An optional
     *          callback may be registered which gets called on error or if the target device has
     *          become fully operational.
     *
     * \author  Benjamin "Nefarius" Höglinger
     * \date    28.08.2017
     *
     * \param   vigem   The driver connection object.
     * \param   target  The target device object.
     * \param   result  An optional function getting called when the target device becomes available.
     *
     * \return  A VIGEM_ERROR.
     */
    VIGEM_API VIGEM_ERROR vigem_target_add_async(PVIGEM_CLIENT vigem, PVIGEM_TARGET target, PFN_VIGEM_TARGET_ADD_RESULT result);

    /**
     * \fn  VIGEM_ERROR vigem_target_remove(PVIGEM_CLIENT vigem, PVIGEM_TARGET target);
     *
     * \brief   Removes a provided target device from the bus driver, which is equal to a device
     *          unplug event of a physical hardware device. The target device object may be reused
     *          after this function is called. If this function is never called on target device
     *          objects, they will be removed from the bus when the owning process terminates.
     *
     * \author  Benjamin "Nefarius" Höglinger
     * \date    28.08.2017
     *
     * \param   vigem   The driver connection object.
     * \param   target  The target device object.
     *
     * \return  A VIGEM_ERROR.
     */
    VIGEM_API VIGEM_ERROR vigem_target_remove(PVIGEM_CLIENT vigem, PVIGEM_TARGET target);

    /**
     * \fn  VIGEM_ERROR vigem_target_x360_register_notification(PVIGEM_CLIENT vigem, PVIGEM_TARGET target, PVIGEM_X360_NOTIFICATION notification);
     *
     * \brief   Registers a function which gets called, when LED index or vibration state changes
     *          occur on the provided target device. This function fails if the provided target
     *          device isn't fully operational or in an erroneous state.
     *
     * \author  Benjamin "Nefarius" Höglinger
     * \date    28.08.2017
     *
     * \param   vigem           The driver connection object.
     * \param   target          The target device object.
     * \param   notification    The notification callback.
     *
     * \return  A VIGEM_ERROR.
     */
    VIGEM_API VIGEM_ERROR vigem_target_x360_register_notification(PVIGEM_CLIENT vigem, PVIGEM_TARGET target, PFN_VIGEM_X360_NOTIFICATION notification);

    /**
     * \fn  VIGEM_ERROR vigem_target_ds4_register_notification(PVIGEM_CLIENT vigem, PVIGEM_TARGET target, PVIGEM_DS4_NOTIFICATION notification);
     *
     * \brief   Registers a function which gets called, when LightBar or vibration state changes
     *          occur on the provided target device. This function fails if the provided target
     *          device isn't fully operational or in an erroneous state.
     *
     * \author  Benjamin "Nefarius" Höglinger
     * \date    28.08.2017
     *
     * \param   vigem           The driver connection object.
     * \param   target          The target device object.
     * \param   notification    The notification callback.
     *
     * \return  A VIGEM_ERROR.
     */
    VIGEM_API VIGEM_ERROR vigem_target_ds4_register_notification(PVIGEM_CLIENT vigem, PVIGEM_TARGET target, PFN_VIGEM_DS4_NOTIFICATION notification);

    /**
     * \fn  void vigem_target_x360_unregister_notification(PVIGEM_TARGET target);
     *
     * \brief   Removes a previously registered callback function from the provided target object.
     *
     * \author  Benjamin "Nefarius" Höglinger
     * \date    28.08.2017
     *
     * \param   target  The target device object.
     */
    VIGEM_API void vigem_target_x360_unregister_notification(PVIGEM_TARGET target);

    /**
     * \fn  void vigem_target_ds4_unregister_notification(PVIGEM_TARGET target);
     *
     * \brief   Removes a previously registered callback function from the provided target object.
     *
     * \author  Benjamin "Nefarius" Höglinger
     * \date    28.08.2017
     *
     * \param   target  The target device object.
     */
    VIGEM_API void vigem_target_ds4_unregister_notification(PVIGEM_TARGET target);

    /**
     * \fn  void vigem_target_set_vid(PVIGEM_TARGET target, USHORT vid);
     *
     * \brief   Overrides the default Vendor ID value with the provided one.
     *
     * \author  Benjamin "Nefarius" Höglinger
     * \date    28.08.2017
     *
     * \param   target  The target device object.
     * \param   vid     The Vendor ID to set.
     */
    VIGEM_API void vigem_target_set_vid(PVIGEM_TARGET target, USHORT vid);

    /**
     * \fn  void vigem_target_set_pid(PVIGEM_TARGET target, USHORT pid);
     *
     * \brief   Overrides the default Product ID value with the provided one.
     *
     * \author  Benjamin "Nefarius" Höglinger
     * \date    28.08.2017
     *
     * \param   target  The target device object.
     * \param   pid     The Product ID to set.
     */
    VIGEM_API void vigem_target_set_pid(PVIGEM_TARGET target, USHORT pid);

    /**
     * \fn  USHORT vigem_target_get_vid(PVIGEM_TARGET target);
     *
     * \brief   Returns the Vendor ID of the provided target device object.
     *
     * \author  Benjamin "Nefarius" Höglinger
     * \date    28.08.2017
     *
     * \param   target  The target device object.
     *
     * \return  The Vendor ID.
     */
    VIGEM_API USHORT vigem_target_get_vid(PVIGEM_TARGET target);

    /**
     * \fn  USHORT vigem_target_get_pid(PVIGEM_TARGET target);
     *
     * \brief   Returns the Product ID of the provided target device object.
     *
     * \author  Benjamin "Nefarius" Höglinger
     * \date    28.08.2017
     *
     * \param   target  The target device object.
     *
     * \return  The Product ID.
     */
    VIGEM_API USHORT vigem_target_get_pid(PVIGEM_TARGET target);

    /**
     * \fn  VIGEM_ERROR vigem_target_x360_update(PVIGEM_CLIENT vigem, PVIGEM_TARGET target, XUSB_REPORT report);
     *
     * \brief   Sends a state report to the provided target device.
     *
     * \author  Benjamin "Nefarius" Höglinger
     * \date    28.08.2017
     *
     * \param   vigem   The driver connection object.
     * \param   target  The target device object.
     * \param   report  The report to send to the target device.
     *
     * \return  A VIGEM_ERROR.
     */
    VIGEM_API VIGEM_ERROR vigem_target_x360_update(PVIGEM_CLIENT vigem, PVIGEM_TARGET target, XUSB_REPORT report);

    /**
     * \fn  VIGEM_ERROR vigem_target_ds4_update(PVIGEM_CLIENT vigem, PVIGEM_TARGET target, DS4_REPORT report);
     *
     * \brief   Sends a state report to the provided target device.
     *
     * \author  Benjamin "Nefarius" Höglinger
     * \date    28.08.2017
     *
     * \param   vigem   The driver connection object.
     * \param   target  The target device object.
     * \param   report  The report to send to the target device.
     *
     * \return  A VIGEM_ERROR.
     */
    VIGEM_API VIGEM_ERROR vigem_target_ds4_update(PVIGEM_CLIENT vigem, PVIGEM_TARGET target, DS4_REPORT report);

    /**
     * \fn  ULONG vigem_target_get_index(PVIGEM_TARGET target);
     *
     * \brief   Returns the internal index (serial number) the bus driver assigned to the provided
     *          target device object. Note that this value is specific to the inner workings of the
     *          bus driver, it does not reflect related values like player index or device arrival
     *          order experienced by other APIs. It may be used to identify the target device object
     *          for its lifetime. This value becomes invalid once the target device is removed from
     *          the bus and may change on the next addition of the device.
     *
     * \author  Benjamin "Nefarius" Höglinger
     * \date    28.08.2017
     *
     * \param   target  The target device object.
     *
     * \return  The internally used index of the target device.
     */
    VIGEM_API ULONG vigem_target_get_index(PVIGEM_TARGET target);

    /**
     * \fn  VIGEM_TARGET_TYPE vigem_target_get_type(PVIGEM_TARGET target);
     *
     * \brief   Returns the type of the provided target device object.
     *
     * \author  Benjamin "Nefarius" Höglinger
     * \date    28.08.2017
     *
     * \param   target  The target device object.
     *
     * \return  A VIGEM_TARGET_TYPE.
     */
    VIGEM_API VIGEM_TARGET_TYPE vigem_target_get_type(PVIGEM_TARGET target);

    /**
     * \fn  BOOL vigem_target_is_attached(PVIGEM_TARGET target);
     *
     * \brief   Returns TRUE if the provided target device object is currently attached to the bus,
     *          FALSE otherwise.
     *
     * \author  Benjamin "Nefarius" Höglinger
     * \date    30.08.2017
     *
     * \param   target  The target device object.
     *
     * \return  TRUE if device is attached to the bus, FALSE otherwise.
     */
    VIGEM_API BOOL vigem_target_is_attached(PVIGEM_TARGET target);

    /**
     * \fn  VIGEM_API VIGEM_ERROR vigem_target_x360_get_user_index(PVIGEM_CLIENT vigem, PVIGEM_TARGET target, PULONG index);
     *
     * \brief   Returns the user index of the emulated Xenon device. This value correspondents to the
     *          (zero-based) index number representing the player number via LED present on a
     *          physical controller and is compatible to the dwUserIndex propery of the XInput* APIs.
     *
     * \author  Benjamin "Nefarius" Höglinger
     * \date    10.05.2018
     *
     * \param   vigem   The driver connection object.
     * \param   target  The target device object.
     * \param   index   The (zero-based) user index of the Xenon device.
     *
     * \return  A VIGEM_ERROR.
     */
    VIGEM_API VIGEM_ERROR vigem_target_x360_get_user_index(PVIGEM_CLIENT vigem, PVIGEM_TARGET target, PULONG index);

#ifdef __cplusplus
}
#endif

#endif // ViGEmClient_h__
