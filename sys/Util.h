/*
* Virtual Gamepad Emulation Framework - Windows kernel-mode bus driver
* Copyright (C) 2016-2018  Benjamin Höglinger-Stelzer
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#pragma once

//
// Returns the current caller process id.
// 
#define CURRENT_PROCESS_ID() ((DWORD)((DWORD_PTR)PsGetCurrentProcessId() & 0xFFFFFFFF))

#define IS_OWNER(_pdo_) (_pdo_->OwnerProcessId == CURRENT_PROCESS_ID())

//
// Represents a MAC address.
//
typedef struct _MAC_ADDRESS
{
    UCHAR Vendor0;
    UCHAR Vendor1;
    UCHAR Vendor2;
    UCHAR Nic0;
    UCHAR Nic1;
    UCHAR Nic2;
} MAC_ADDRESS, *PMAC_ADDRESS;


VOID ReverseByteArray(PUCHAR Array, INT Length);
VOID GenerateRandomMacAddress(PMAC_ADDRESS Address);
