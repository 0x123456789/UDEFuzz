#pragma once 

#include <initguid.h>
#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "Devices.h"


#ifdef RELEASE
#include "http.h"
#endif // RELEASE



#pragma comment(lib, "Cfgmgr32.lib")



void EnumerateDevices(LPGUID interfaceGuid) {
    PZZWSTR devices = GetPresentDeviceList(interfaceGuid);

    PWSTR device = devices;
    while (*device != UNICODE_NULL) {
        wprintf(L"DeviceName = (%ls)\n", device);
        device = device + wcslen(device) + 1;
    }
}




//
//void GetDriverInfo(LPGUID interfaceGuid) {
//    DWORD bytesReturned = 0;
//    PZZWSTR devices = GetPresentDeviceList(interfaceGuid);
//    // unplug open first device of the list
//    if (*devices != UNICODE_NULL) {
//        wprintf(L"Obtaining driver information %ls...\n", devices);
//        HANDLE handle = OpenDevice(devices);
//        if (handle != INVALID_HANDLE_VALUE) {
//            if (!DeviceIoControl(handle,
//                IOCTL_GET_DRIVER_INFO,
//                NULL,                  // Ptr to InBuffer
//                0,                     // Length of InBuffer
//                NULL,                  // Ptr to OutBuffer
//                0,                     // Length of OutBuffer
//                &bytesReturned,        // BytesReturned
//                0))                    // Ptr to Overlapped structure
//            {
//                wprintf(L"DeviceIoControl failed with error 0x%x\n", GetLastError());
//                return;
//            }
//            wprintf(L"Information obtained successully\n");
//        }
//        else {
//            wprintf(L"Failed to open the device, error - %d", GetLastError());
//            return;
//        }
//    }
//    else {
//        return;
//    }
//}



DEFINE_GUID(GUID_DEVINTERFACE_HOSTUDE,
    0x5ad9d323, 0xc478, 0x4ad2, 0xb6, 0x39, 0x9d, 0x2e, 0xdd, 0xbd, 0x3b, 0x2c);

#include <Usbiodef.h>

int wmain(int argc, wchar_t* argv[]) {

    HANDLE handle;
    // using this guid we can communicate with our driver via CreateFile for getting handle
    LPGUID deviceGUID = (LPGUID)&GUID_DEVINTERFACE_UDE_BACKCHANNEL;

    for (size_t optind = 1; optind < argc && argv[optind][0] == '-'; optind++) {
        switch (argv[optind][1]) {
        case L'e': EnumerateDevices(deviceGUID); break;
        case L'u': UnplugUSBDevice(); break;
        case L'p': 
            handle = RunUSBDeviceCheckerThread(ToDeviceCode(argv[optind][2]));
            if (handle == NULL) {
                wprintf(L"Create thread error: %d", GetLastError());
            }
            else {
                WaitForSingleObject(handle, INFINITE);
            }
            break;
        //case L'f': GetDriverInfo(deviceGUID); break;
        case L'i': GenerateInterrupt(deviceGUID); break;

            /*case 'l': mode = LINE_MODE; break;
            case 'w': mode = WORD_MODE; break;*/
        default:
            wprintf(L"Usage: %s [-eupi]\n", argv[0]);
            break;
        }
    }
}