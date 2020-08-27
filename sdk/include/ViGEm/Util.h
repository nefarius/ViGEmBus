#pragma once

#include "ViGEm/Common.h"
#include <limits.h>

VOID FORCEINLINE XUSB_TO_DS4_REPORT(
    _Out_ PXUSB_REPORT Input,
    _Out_ PDS4_REPORT Output
)
{
    if (Input->wButtons & XUSB_GAMEPAD_BACK) Output->wButtons |= DS4_BUTTON_SHARE;
    if (Input->wButtons & XUSB_GAMEPAD_START) Output->wButtons |= DS4_BUTTON_OPTIONS;
    if (Input->wButtons & XUSB_GAMEPAD_LEFT_THUMB) Output->wButtons |= DS4_BUTTON_THUMB_LEFT;
    if (Input->wButtons & XUSB_GAMEPAD_RIGHT_THUMB) Output->wButtons |= DS4_BUTTON_THUMB_RIGHT;
    if (Input->wButtons & XUSB_GAMEPAD_LEFT_SHOULDER) Output->wButtons |= DS4_BUTTON_SHOULDER_LEFT;
    if (Input->wButtons & XUSB_GAMEPAD_RIGHT_SHOULDER) Output->wButtons |= DS4_BUTTON_SHOULDER_RIGHT;
    if (Input->wButtons & XUSB_GAMEPAD_GUIDE) Output->bSpecial |= DS4_SPECIAL_BUTTON_PS;
    if (Input->wButtons & XUSB_GAMEPAD_A) Output->wButtons |= DS4_BUTTON_CROSS;
    if (Input->wButtons & XUSB_GAMEPAD_B) Output->wButtons |= DS4_BUTTON_CIRCLE;
    if (Input->wButtons & XUSB_GAMEPAD_X) Output->wButtons |= DS4_BUTTON_SQUARE;
    if (Input->wButtons & XUSB_GAMEPAD_Y) Output->wButtons |= DS4_BUTTON_TRIANGLE;

    Output->bTriggerL = Input->bLeftTrigger;
    Output->bTriggerR = Input->bRightTrigger;

    if (Input->bLeftTrigger > 0)Output->wButtons |= DS4_BUTTON_TRIGGER_LEFT;
    if (Input->bRightTrigger > 0)Output->wButtons |= DS4_BUTTON_TRIGGER_RIGHT;

    if (Input->wButtons & XUSB_GAMEPAD_DPAD_UP) DS4_SET_DPAD(Output, DS4_BUTTON_DPAD_NORTH);
    if (Input->wButtons & XUSB_GAMEPAD_DPAD_RIGHT) DS4_SET_DPAD(Output, DS4_BUTTON_DPAD_EAST);
    if (Input->wButtons & XUSB_GAMEPAD_DPAD_DOWN) DS4_SET_DPAD(Output, DS4_BUTTON_DPAD_SOUTH);
    if (Input->wButtons & XUSB_GAMEPAD_DPAD_LEFT) DS4_SET_DPAD(Output, DS4_BUTTON_DPAD_WEST);

    if (Input->wButtons & XUSB_GAMEPAD_DPAD_UP
        && Input->wButtons & XUSB_GAMEPAD_DPAD_RIGHT) DS4_SET_DPAD(Output, DS4_BUTTON_DPAD_NORTHEAST);
    if (Input->wButtons & XUSB_GAMEPAD_DPAD_RIGHT
        && Input->wButtons & XUSB_GAMEPAD_DPAD_DOWN) DS4_SET_DPAD(Output, DS4_BUTTON_DPAD_SOUTHEAST);
    if (Input->wButtons & XUSB_GAMEPAD_DPAD_DOWN
        && Input->wButtons & XUSB_GAMEPAD_DPAD_LEFT) DS4_SET_DPAD(Output, DS4_BUTTON_DPAD_SOUTHWEST);
    if (Input->wButtons & XUSB_GAMEPAD_DPAD_LEFT
        && Input->wButtons & XUSB_GAMEPAD_DPAD_UP) DS4_SET_DPAD(Output, DS4_BUTTON_DPAD_NORTHWEST);

    Output->bThumbLX = ((Input->sThumbLX + ((USHRT_MAX / 2) + 1)) / 257);
    Output->bThumbLY = (-(Input->sThumbLY + ((USHRT_MAX / 2) - 1)) / 257);
    Output->bThumbLY = (Output->bThumbLY == 0) ? 0xFF : Output->bThumbLY;
    Output->bThumbRX = ((Input->sThumbRX + ((USHRT_MAX / 2) + 1)) / 257);
    Output->bThumbRY = (-(Input->sThumbRY + ((USHRT_MAX / 2) + 1)) / 257);
    Output->bThumbRY = (Output->bThumbRY == 0) ? 0xFF : Output->bThumbRY;
}

