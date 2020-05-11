/*
* Virtual Gamepad Emulation Framework - Windows kernel-mode bus driver
*
* MIT License
*
* Copyright (c) 2016-2020 Nefarius Software Solutions e.U. and Contributors
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*/

#include "busenum.h"
#include "EmulationTargetPDO.hpp"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, Bus_EvtDeviceListCreatePdo)
#endif

EXTERN_C NTSTATUS Bus_EvtDeviceListCreatePdo(
    WDFCHILDLIST DeviceList,
    PWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER IdentificationDescription,
    PWDFDEVICE_INIT ChildInit)
{
	ViGEm::Bus::Core::PPDO_IDENTIFICATION_DESCRIPTION pDesc;

    PAGED_CODE();

    pDesc = CONTAINING_RECORD(IdentificationDescription, ViGEm::Bus::Core::PDO_IDENTIFICATION_DESCRIPTION, Header);

    return pDesc->Target->PdoCreateDevice(WdfChildListGetDevice(DeviceList), ChildInit);
}

//
// Compares two children on the bus based on their serial numbers.
// 
EXTERN_C BOOLEAN Bus_EvtChildListIdentificationDescriptionCompare(
    WDFCHILDLIST DeviceList,
    PWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER FirstIdentificationDescription,
    PWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER SecondIdentificationDescription)
{
	ViGEm::Bus::Core::PPDO_IDENTIFICATION_DESCRIPTION lhs, rhs;

    UNREFERENCED_PARAMETER(DeviceList);

    lhs = CONTAINING_RECORD(FirstIdentificationDescription,
        ViGEm::Bus::Core::PDO_IDENTIFICATION_DESCRIPTION,
        Header);
    rhs = CONTAINING_RECORD(SecondIdentificationDescription,
        ViGEm::Bus::Core::PDO_IDENTIFICATION_DESCRIPTION,
        Header);

    return (lhs->SerialNo == rhs->SerialNo) ? TRUE : FALSE;
}
