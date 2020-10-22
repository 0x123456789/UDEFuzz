#pragma once 

#include <initguid.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <cfgmgr32.h>

#include "..\UDEFX2\public.h"
#include "..\UDEFX2\AgentControl.h"

#pragma comment(lib, "Cfgmgr32.lib")

HANDLE OpenDevice(PWSTR deviceName) {
    return CreateFile(deviceName,
        GENERIC_WRITE | GENERIC_READ,
        FILE_SHARE_WRITE | FILE_SHARE_READ,
        NULL, // default security
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);
}

PZZWSTR GetPresentDeviceList(LPGUID interfaceGuid) {
    CONFIGRET cr = CR_SUCCESS;
    PWSTR deviceInterfaceList = NULL;
    ULONG deviceInterfaceListLength = 0;
    PWSTR nextInterface;
    HANDLE handle = INVALID_HANDLE_VALUE;

    cr = CM_Get_Device_Interface_List_Size(
        &deviceInterfaceListLength, interfaceGuid,
        NULL, CM_GET_DEVICE_INTERFACE_LIST_PRESENT);
    if (cr != CR_SUCCESS) {
        printf("Error 0x%x retrieving device interface list size.\n", cr);
        goto clean0;
    }

    if (deviceInterfaceListLength <= 1) {
        printf("Error: No active device interfaces found. Is the sample driver loaded?");
        goto clean0;
    }

    deviceInterfaceList = (PWSTR)malloc(deviceInterfaceListLength * sizeof(WCHAR));
    if (deviceInterfaceList == NULL) {
        printf("Error allocating memory for device interface list.\n");
        goto clean0;
    }

    ZeroMemory(deviceInterfaceList, deviceInterfaceListLength * sizeof(WCHAR));

    cr = CM_Get_Device_Interface_List(
        interfaceGuid, NULL, deviceInterfaceList,
        deviceInterfaceListLength, CM_GET_DEVICE_INTERFACE_LIST_PRESENT);
    if (cr != CR_SUCCESS) {
        printf("Error 0x%x retrieving device interface list.\n", cr);
        goto clean0;
    }
    return deviceInterfaceList;

clean0:
    if (deviceInterfaceList != NULL) {
        free(deviceInterfaceList);
    }
    return (WCHAR*)UNICODE_NULL;
}


void EnumerateDevices(LPGUID interfaceGuid) {
    PZZWSTR devices = GetPresentDeviceList(interfaceGuid);

    PWSTR device = devices;
    while (*device != UNICODE_NULL) {
        wprintf(L"DeviceName = (%ls)\n", device);
        device = devices + wcslen(devices) + 1;
    }
}


void UnplugUSBDevice(LPGUID interfaceGuid) {
    DWORD bytesReturned = 0;
    PZZWSTR devices = GetPresentDeviceList(interfaceGuid);
    // unplug open first device of the list
    if (*devices != UNICODE_NULL) {
        wprintf(L"Unpluging device %ls...\n", devices);
        HANDLE handle = OpenDevice(devices);
        if (handle != INVALID_HANDLE_VALUE) {
            if (!DeviceIoControl(handle,
                IOCTL_UNPLUG_USB_DEVICE,
                NULL,                  // Ptr to InBuffer
                0,                     // Length of InBuffer
                NULL,                  // Ptr to OutBuffer
                0,                     // Length of OutBuffer
                &bytesReturned,        // BytesReturned
                0))                    // Ptr to Overlapped structure
            {                  
                wprintf(L"DeviceIoControl failed with error 0x%x\n", GetLastError());
                return;
            }
            wprintf(L"Virtual USB device unplugged and deleted successully\n");
        }
        else {
            wprintf(L"Failed to open the device, error - %d", GetLastError());
            return;
        }
    }
    else {
        return;
    }
}


void PlugInUSBDevice(LPGUID interfaceGuid, USHORT deviceCode) {
    wprintf(L"[*] Selected device code: %d\n", deviceCode);

    DWORD bytesReturned = 0;
    PZZWSTR devices = GetPresentDeviceList(interfaceGuid);
    // unplug open first device of the list
    if (*devices != UNICODE_NULL) {
        wprintf(L"Plug-in device %ls...\n", devices);
        HANDLE handle = OpenDevice(devices);
        if (handle != INVALID_HANDLE_VALUE) {
            if (!DeviceIoControl(handle,
                IOCTL_PLUG_USB_DEVICE,
                &deviceCode,           // Ptr to InBuffer
                sizeof(deviceCode),    // Length of InBuffer
                NULL,                  // Ptr to OutBuffer
                0,                     // Length of OutBuffer
                &bytesReturned,        // BytesReturned
                0))                    // Ptr to Overlapped structure
            {
                wprintf(L"DeviceIoControl failed with error 0x%x\n", GetLastError());
                return;
            }
            wprintf(L"Virtual USB device unplugged and deleted successully\n");
        }
        else {
            wprintf(L"Failed to open the device, error - %d", GetLastError());
            return;
        }
    }
    else {
        return;
    }
}


void GetDriverInfo(LPGUID interfaceGuid) {
    DWORD bytesReturned = 0;
    PZZWSTR devices = GetPresentDeviceList(interfaceGuid);
    // unplug open first device of the list
    if (*devices != UNICODE_NULL) {
        wprintf(L"Obtaining driver information %ls...\n", devices);
        HANDLE handle = OpenDevice(devices);
        if (handle != INVALID_HANDLE_VALUE) {
            if (!DeviceIoControl(handle,
                IOCTL_GET_DRIVER_INFO,
                NULL,                  // Ptr to InBuffer
                0,                     // Length of InBuffer
                NULL,                  // Ptr to OutBuffer
                0,                     // Length of OutBuffer
                &bytesReturned,        // BytesReturned
                0))                    // Ptr to Overlapped structure
            {
                wprintf(L"DeviceIoControl failed with error 0x%x\n", GetLastError());
                return;
            }
            wprintf(L"Information obtained successully\n");
        }
        else {
            wprintf(L"Failed to open the device, error - %d", GetLastError());
            return;
        }
    }
    else {
        return;
    }
}



//
//HANDLE OpenDeviceByInterfaceGUID(LPGUID InterfaceGuid) {
//    CONFIGRET cr = CR_SUCCESS;
//    PWSTR deviceInterfaceList = NULL;
//    ULONG deviceInterfaceListLength = 0;
//    PWSTR nextInterface;
//    HANDLE handle = INVALID_HANDLE_VALUE;
//
//    nextInterface = deviceInterfaceList + wcslen(deviceInterfaceList) + 1;
//    if (*nextInterface != UNICODE_NULL) {
//        printf("Warning: More than one device interface instance found. \n"
//            "Selecting first matching device.\n\n");
//        wprintf(L"NextInterface = (%lS)\n", nextInterface);
//    }
//    
//    wprintf(L"DeviceName = (%lS)\n", deviceInterfaceList);
//    handle = CreateFile(deviceInterfaceList,
//        GENERIC_WRITE | GENERIC_READ,
//        FILE_SHARE_WRITE | FILE_SHARE_READ,
//        NULL, // default security
//        OPEN_EXISTING,
//        FILE_ATTRIBUTE_NORMAL,
//        NULL);
//
//    if (handle == INVALID_HANDLE_VALUE) {
//        printf("Failed to open the device, error - %d", GetLastError()); fflush(stdout);
//    }
//    else {
//        printf("Opened the device successfully.\n"); fflush(stdout);
//    }
//
//clean0:
//    if (deviceInterfaceList != NULL) {
//        free(deviceInterfaceList);
//    }
//    return handle;
//}
//


//void UnplugUSBDevice() {
//    HANDLE          deviceHandle;
//    DWORD           code;
//    ULONG           index = 0;
//    DEVICE_INTR_FLAGS  flagsState = 0;
//
//    printf("About to open device\n"); fflush(stdout);
//
//    deviceHandle = OpenDeviceByInterfaceGUID((LPGUID)&GUID_DEVINTERFACE_UDE_BACKCHANNEL);
//
//    if (deviceHandle == INVALID_HANDLE_VALUE) {
//
//        printf("Unable to find virtual controller device!\n"); fflush(stdout);
//
//        return FALSE;
//
//    }
//}


void 
WriteTextTo(LPCGUID interfaceGuid)
{
    HANDLE deviceHandle;
    DWORD  nBytesWrite = 0;
    BOOL   success;
    const char* pText = "0x12345";

    PZZWSTR devices = GetPresentDeviceList(interfaceGuid);
    // unplug open first device of the list
    if (*devices != UNICODE_NULL) {

        deviceHandle = OpenDevice(devices);
        
        if (deviceHandle == INVALID_HANDLE_VALUE) {
            wprintf(L"Unable to find device!\n");
            fflush(stdout);
            return;
        }

        wprintf(L"Device open, will write...\n"); fflush(stdout);

        success = WriteFile(deviceHandle, pText, (DWORD)(strlen(pText) + 1), &nBytesWrite, NULL);
        if (!success) {
            wprintf(L"WriteFile failed - error %d\n", GetLastError());
        }
        else {
            printf("WriteFile SUCCESS, text=%s, bytes=%d\n", pText, nBytesWrite);
        }
        CloseHandle(deviceHandle);
        return;
    }
    return;
}


USHORT ToDeviceCode(WCHAR number) {
    if (number == L'\0') {
        return 0;
    }

    USHORT n = number - L'0';
    if (n > 9) {
        wprintf(L"Bad device code: %d. Should be from 0 to 9", n);
        exit(-1);
    }
    return n;
}


DEFINE_GUID(GUID_DEVINTERFACE_HOSTUDE,
    0x5ad9d323, 0xc478, 0x4ad2, 0xb6, 0x39, 0x9d, 0x2e, 0xdd, 0xbd, 0x3b, 0x2c);

int wmain(int argc, wchar_t* argv[]) {
    // using this guid we can communicate with our driver via CreateFile for getting handle
    LPGUID deviceGUID = (LPGUID)&GUID_DEVINTERFACE_UDE_BACKCHANNEL;

    for (size_t optind = 1; optind < argc && argv[optind][0] == '-'; optind++) {
        switch (argv[optind][1]) {
        case L'e': EnumerateDevices(deviceGUID); break;
        case L'u': UnplugUSBDevice(deviceGUID); break;
        case L'p': PlugInUSBDevice(deviceGUID, ToDeviceCode(argv[optind][2])); break;
        case L'i': GetDriverInfo(deviceGUID); break;
        case L'w': WriteTextTo((LPGUID)&GUID_DEVINTERFACE_HOSTUDE); break;

            /*case 'l': mode = LINE_MODE; break;
            case 'w': mode = WORD_MODE; break;*/
        default:
            wprintf(L"Usage: %s [-eupi]\n", argv[0]);
            break;
        }
    }
}