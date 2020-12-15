
#include <windows.h>
#include <tchar.h>

#include <setupapi.h>
#include <initguid.h>

#include <stdio.h>

	// This is the GUID for the USB device class
	DEFINE_GUID(GUID_DEVINTERFACE_USB_DEVICE,
		0xA5DCBF10L, 0x6530, 0x11D2, 0x90, 0x1F, 0x00, 0xC0, 0x4F, 0xB9, 0x51, 0xED);
	// (A5DCBF10-6530-11D2-901F-00C04FB951ED)


// VIDPID format should be like this: vid_10cf&pid_8090 (because this format is used in system)
BOOL IsUSBDeviceConnected(WCHAR* VIDPID) {

		BOOL							 connected;

		HDEVINFO                         hDevInfo;
		SP_DEVICE_INTERFACE_DATA         DevIntfData;
		PSP_DEVICE_INTERFACE_DETAIL_DATA DevIntfDetailData;
		SP_DEVINFO_DATA                  DevData;

		DWORD dwSize, dwType, dwMemberIdx;
		HKEY hKey;
		BYTE lpData[1024];

		
		connected = FALSE;
		wprintf(L"[*] Connected USB devices:\n");

		// We will try to get device information set for all USB devices that have a
		// device interface and are currently present on the system (plugged in).
		hDevInfo = SetupDiGetClassDevs(
			&GUID_DEVINTERFACE_USB_DEVICE, NULL, 0, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);

		if (hDevInfo != INVALID_HANDLE_VALUE)
		{
			// Prepare to enumerate all device interfaces for the device information
			// set that we retrieved with SetupDiGetClassDevs(..)
			DevIntfData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
			dwMemberIdx = 0;

			// Next, we will keep calling this SetupDiEnumDeviceInterfaces(..) until this
			// function causes GetLastError() to return  ERROR_NO_MORE_ITEMS. With each
			// call the dwMemberIdx value needs to be incremented to retrieve the next
			// device interface information.

			SetupDiEnumDeviceInterfaces(hDevInfo, NULL, &GUID_DEVINTERFACE_USB_DEVICE,
				dwMemberIdx, &DevIntfData);

			while (GetLastError() != ERROR_NO_MORE_ITEMS)
			{
				// As a last step we will need to get some more details for each
				// of device interface information we are able to retrieve. This
				// device interface detail gives us the information we need to identify
				// the device (VID/PID), and decide if it's useful to us. It will also
				// provide a DEVINFO_DATA structure which we can use to know the serial
				// port name for a virtual com port.

				DevData.cbSize = sizeof(DevData);

				// Get the required buffer size. Call SetupDiGetDeviceInterfaceDetail with
				// a NULL DevIntfDetailData pointer, a DevIntfDetailDataSize
				// of zero, and a valid RequiredSize variable. In response to such a call,
				// this function returns the required buffer size at dwSize.

				SetupDiGetDeviceInterfaceDetail(
					hDevInfo, &DevIntfData, NULL, 0, &dwSize, NULL);

				// Allocate memory for the DeviceInterfaceDetail struct. Don't forget to
				// deallocate it later!
				DevIntfDetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwSize);
				DevIntfDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

				if (SetupDiGetDeviceInterfaceDetail(hDevInfo, &DevIntfData,
					DevIntfDetailData, dwSize, &dwSize, &DevData))
				{
					// Finally we can start checking if we've found a useable device,
					// by inspecting the DevIntfDetailData->DevicePath variable.
					// The DevicePath looks something like this:
					//
					// \\?\usb#vid_04d8&pid_0033#5&19f2438f&0&2#{a5dcbf10-6530-11d2-901f-00c04fb951ed}
					//

					wprintf(L"    [+] %s\n", (TCHAR*)DevIntfDetailData->DevicePath);
					if (NULL != wcsstr((TCHAR*)DevIntfDetailData->DevicePath, VIDPID)) {
						connected = TRUE;
					}
				}

				HeapFree(GetProcessHeap(), 0, DevIntfDetailData);

				// Continue looping
				SetupDiEnumDeviceInterfaces(
					hDevInfo, NULL, &GUID_DEVINTERFACE_USB_DEVICE, ++dwMemberIdx, &DevIntfData);
			}

			SetupDiDestroyDeviceInfoList(hDevInfo);
		}

		if (connected) {
			wprintf(L"\n    [+] Device with VIP = 125f, PID = c82a is connected\n");
		}
		else {
			wprintf(L"\n    [-] Device with VIP = 125f, PID = c82a is NOT connected\n");
		}

		return connected;
}

