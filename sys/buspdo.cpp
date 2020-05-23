/*
* Virtual Gamepad Emulation Framework - Windows kernel-mode bus driver
*
* BSD 3-Clause License
*
* Copyright (c) 2018-2020, Nefarius Software Solutions e.U. and Contributors
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
* 1. Redistributions of source code must retain the above copyright notice, this
*    list of conditions and the following disclaimer.
*
* 2. Redistributions in binary form must reproduce the above copyright notice,
*    this list of conditions and the following disclaimer in the documentation
*    and/or other materials provided with the distribution.
*
* 3. Neither the name of the copyright holder nor the names of its
*    contributors may be used to endorse or promote products derived from
*    this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
* CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


#include "Driver.h"
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
