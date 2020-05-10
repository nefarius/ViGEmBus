#include "EmulationTargetPDO.hpp"
#include "CRTCPP.hpp"


BOOLEAN USB_BUSIFFN ViGEm::Bus::Core::EmulationTargetPDO::UsbIsDeviceHighSpeed(IN PVOID BusContext)
{
	UNREFERENCED_PARAMETER(BusContext);

	return TRUE;
}

NTSTATUS USB_BUSIFFN ViGEm::Bus::Core::EmulationTargetPDO::UsbQueryBusInformation(IN PVOID BusContext, IN ULONG Level,
                                                                                  IN OUT PVOID BusInformationBuffer,
                                                                                  IN OUT PULONG BusInformationBufferLength,
                                                                                  OUT PULONG BusInformationActualLength)
{
	UNREFERENCED_PARAMETER(BusContext);
	UNREFERENCED_PARAMETER(Level);
	UNREFERENCED_PARAMETER(BusInformationBuffer);
	UNREFERENCED_PARAMETER(BusInformationBufferLength);
	UNREFERENCED_PARAMETER(BusInformationActualLength);

	return STATUS_UNSUCCESSFUL;
}

NTSTATUS USB_BUSIFFN ViGEm::Bus::Core::EmulationTargetPDO::UsbSubmitIsoOutUrb(IN PVOID BusContext, IN PURB Urb)
{
	UNREFERENCED_PARAMETER(BusContext);
	UNREFERENCED_PARAMETER(Urb);

	return STATUS_UNSUCCESSFUL;
}

NTSTATUS USB_BUSIFFN ViGEm::Bus::Core::EmulationTargetPDO::UsbQueryBusTime(IN PVOID BusContext, IN OUT PULONG CurrentUsbFrame)
{
	UNREFERENCED_PARAMETER(BusContext);
	UNREFERENCED_PARAMETER(CurrentUsbFrame);

	return STATUS_UNSUCCESSFUL;
}

VOID USB_BUSIFFN ViGEm::Bus::Core::EmulationTargetPDO::UsbGetUSBDIVersion(IN PVOID BusContext,
                                                                          IN OUT PUSBD_VERSION_INFORMATION VersionInformation,
                                                                          IN OUT PULONG HcdCapabilities)
{
	UNREFERENCED_PARAMETER(BusContext);

	if (VersionInformation != nullptr)
	{
		VersionInformation->USBDI_Version = 0x500; /* Usbport */
		VersionInformation->Supported_USB_Version = 0x200; /* USB 2.0 */
	}

	if (HcdCapabilities != nullptr)
	{
		*HcdCapabilities = 0;
	}
}

ViGEm::Bus::Core::EmulationTargetPDO::EmulationTargetPDO(USHORT VID, USHORT PID): VendorId(VID), ProductId(PID)
{
}
