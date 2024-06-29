
#include <windows.h>
#include <tchar.h>

#include <setupapi.h>
#include <initguid.h>

#include <stdio.h>
#include <cfgmgr32.h>

#include "Devices.h"

#include <vector>
#include <iostream>
#include <fstream>
#include <time.h>
#include <ctime>

// This is the GUID for the USB device class
DEFINE_GUID(GUID_DEVINTERFACE_USB_DEVICE, 
	0xA5DCBF10L, 0x6530, 0x11D2, 0x90, 0x1F, 0x00, 0xC0, 0x4F, 0xB9, 0x51, 0xED);
// (A5DCBF10-6530-11D2-901F-00C04FB951ED)

struct PARAMS {
    UINT32 DefaultCount;
    UINT8 DeviceCode;
};



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


HANDLE OpenDevice(PWSTR deviceName) {
    return CreateFile(deviceName,
        GENERIC_WRITE | GENERIC_READ,
        FILE_SHARE_WRITE | FILE_SHARE_READ,
        NULL, // default security
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);
}


void UnplugUSBDevice() {
    DWORD bytesReturned = 0;
    PZZWSTR devices = GetPresentDeviceList((LPGUID)&GUID_DEVINTERFACE_UDE_BACKCHANNEL);

    PWSTR device = devices;
   
    // unpluging all devices in loop
    while (*device != UNICODE_NULL) {
        wprintf(L"Unpluging device %s...\n", device);
        HANDLE handle = OpenDevice(device);
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
            }
            wprintf(L"Virtual USB device unplugged and deleted successully\n");
        }
        else {
            wprintf(L"Failed to open the device, error - %d", GetLastError());
        }
        // go to next
        device = device + wcslen(device) + 1;
    }
}


void PlugInUSBDevice(HANDLE controllerHandle, USHORT deviceCode, UINT64 seed, BOOLEAN fuzzDesc, BOOLEAN savePV, BOOLEAN onlyDesc) {
    DWORD bytesReturned = 0;
    wprintf(L"[*] Selected device code: %d\n", deviceCode);
    // getting fuzzing config for plugged-in device
    FUZZING_CONTEXT context;
    context.Seed = seed;
    context.FuzzDescriptor = fuzzDesc;
    context.SavePV = savePV;
    context.OnlyDesc = onlyDesc;

    // for simplicity modes have same values as device codes
    context.Mode = (USB_MODE)deviceCode;
    if (!DeviceIoControl(controllerHandle,
        IOCTL_PLUG_USB_DEVICE,
        &context,                   // Ptr to InBuffer
        sizeof(FUZZING_CONTEXT),    // Length of InBuffer
        NULL,                       // Ptr to OutBuffer
        0,                          // Length of OutBuffer
        &bytesReturned,             // BytesReturned
        0))                         // Ptr to Overlapped structure
    {
        wprintf(L"DeviceIoControl failed with error 0x%x\n", GetLastError());
        return;
    }
    wprintf(L"Virtual USB device plugged successully\n");
}


void PlugInUSBDevice(LPGUID interfaceGuid, USHORT deviceCode) {
    wprintf(L"[*] Selected device code: %d\n", deviceCode);

    HANDLE handle = NULL;
    DWORD bytesReturned = 0;
    PZZWSTR devices = GetPresentDeviceList(interfaceGuid);
    // unplug open first device of the list
    if (*devices != UNICODE_NULL) {
        wprintf(L"Plug-in device %ls...\n", devices);
        handle = OpenDevice(devices);
        if (handle != INVALID_HANDLE_VALUE) {

            // getting fuzzing config for plugged-in device
            FUZZING_CONTEXT context;
            // just magic constant :)
            context.Seed = 112233;
            context.FuzzDescriptor = TRUE;
            context.SavePV = TRUE;
            context.OnlyDesc = FALSE;

            // for simplicity modes have same values as device codes
            context.Mode = (USB_MODE)deviceCode;

            if (!DeviceIoControl(handle,
                IOCTL_PLUG_USB_DEVICE,
                &context,                   // Ptr to InBuffer
                sizeof(FUZZING_CONTEXT),    // Length of InBuffer
                NULL,                       // Ptr to OutBuffer
                0,                          // Length of OutBuffer
                &bytesReturned,             // BytesReturned
                0))                         // Ptr to Overlapped structure
            {
                wprintf(L"DeviceIoControl failed with error 0x%x\n", GetLastError());
                return;
            }
            wprintf(L"Virtual USB device plugged successully\n");
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



void GenerateInterrupt(HANDLE controllerHandle) {
    DEVICE_INTR_FLAGS f = 0xaa;
    DWORD bytesReturned = 0;

    if (!DeviceIoControl(controllerHandle,
        IOCTL_UDEFX2_GENERATE_INTERRUPT,
        &f,                         // Ptr to InBuffer
        sizeof(DEVICE_INTR_FLAGS),  // Length of InBuffer
        NULL,                       // Ptr to OutBuffer
        0,                          // Length of OutBuffer
        &bytesReturned,             // BytesReturned
        0))                         // Ptr to Overlapped structure
    {
        wprintf(L"DeviceIoControl failed with error 0x%x\n", GetLastError());
        return;
    }
    // wprintf(L"GenerateInterrupt successully\n");
}

void GenerateInterrupt(LPGUID interfaceGuid) {
    HANDLE handle = NULL;
    DWORD bytesReturned = 0;
    PZZWSTR devices = GetPresentDeviceList(interfaceGuid);
    // unplug open first device of the list
    if (*devices != UNICODE_NULL) {
        wprintf(L"GenerateInterrupt device %ls...\n", devices);
        handle = OpenDevice(devices);
        if (handle != INVALID_HANDLE_VALUE) {
            DEVICE_INTR_FLAGS f = 0xaa;

            if (!DeviceIoControl(handle,
                IOCTL_UDEFX2_GENERATE_INTERRUPT,
                &f,                         // Ptr to InBuffer
                sizeof(DEVICE_INTR_FLAGS),  // Length of InBuffer
                NULL,                       // Ptr to OutBuffer
                0,                          // Length of OutBuffer
                &bytesReturned,             // BytesReturned
                0))                         // Ptr to Overlapped structure
            {
                wprintf(L"DeviceIoControl failed with error 0x%x\n", GetLastError());
                return;
            }
            wprintf(L"GenerateInterrupt successully\n");
        }
        else {
            wprintf(L"Failed to open GenerateInterrupt, error - %d", GetLastError());
            return;
        }
    }
    else {
        return;
    }
}


// VIDPID format should be like this: VID_10CF&PID_8090 (because this format is used in system)
BOOL IsDeviceConnected(LPGUID interfaceGuid, WCHAR* VIDPID) {
    PZZWSTR devices = GetPresentDeviceList(interfaceGuid);

    PWSTR device = devices;
    while (*device != UNICODE_NULL) {
        if (NULL != wcsstr(device, VIDPID)) {
            return TRUE;
        }
        device = device + wcslen(device) + 1;
    }
    return FALSE;

}

UINT32 GetCount(PZZWSTR list) {
    PWSTR elem = list;
    UINT32 count = 0;

    while (*elem != UNICODE_NULL) {
        count++;
        elem = elem + wcslen(elem) + 1;
    }
    return count;
}

DWORD WINAPI CheckUSBDevice(void* data) {

    LPGUID deviceGUID = (LPGUID)&GUID_DEVINTERFACE_UDE_BACKCHANNEL;

    PARAMS params = *(PARAMS*)data;

    while (TRUE) {
        // get current connected and working USB devices
        PZZWSTR devices = GetPresentDeviceList((LPGUID)&GUID_DEVINTERFACE_USB_DEVICE);
        // count of current connected and working USB devices
        UINT32 currentCount = GetCount(devices);
        // if current count == DefaultCount, when our device is not working,
        // so we disable it and attach again
        if (currentCount == params.DefaultCount) {
            wprintf(L"[Checker] USB device not working, recreating device...\n");
            UnplugUSBDevice();
            PlugInUSBDevice(deviceGUID, params.DeviceCode);
        }
        // do checking every second 
        Sleep(1000);
    }
    return 0;
}


HANDLE RunUSBDeviceCheckerThread(UINT8 deviceCode) {
    // get current connected and working USB devices
    PZZWSTR devices = GetPresentDeviceList((LPGUID)&GUID_DEVINTERFACE_USB_DEVICE);
    // count of current connected and working USB devices
    UINT32 defaultCount = GetCount(devices);
    // we will connect our device, so if device is connected count should be defaultCount + 1
   
    PARAMS params;
    params.DefaultCount = defaultCount;
    params.DeviceCode = deviceCode;

    wprintf(L"Creating thread...\n");
	return CreateThread(NULL, 0, CheckUSBDevice, &params, 0, NULL);
}


std::vector<HANDLE> GetDeviceHandles(PZZWSTR devices) {
    std::vector<HANDLE> v;
    PWSTR device = devices;
   
    while (*device != UNICODE_NULL) {
        HANDLE h = OpenDevice(device);
        if (h != INVALID_HANDLE_VALUE) {
            v.push_back(h);
        }
        else {
            wprintf(L"Failed to open device, error - %d\n", GetLastError());
        }
        device = device + wcslen(device) + 1;
    }
    return v;
}



const std::string currentDateTime() {
    time_t     now = time(0);
    struct tm  tstruct;
    char       buf[80];
    tstruct = *localtime(&now);
    // Visit http://en.cppreference.com/w/cpp/chrono/c/strftime
    // for more information about date/time format
    strftime(buf, sizeof(buf), "%Y-%m-%d.%X", &tstruct);

    return buf;
}


void WriteToSeedLog(UINT64 seed) {
    std::ofstream file;
    file.open("seed_log.txt", std::ios::out | std::ios::app);
    file << currentDateTime() << " " << seed << std::endl;
    file.close();
}

// Fuzzing descriptors using S2E coverage on kernel mode
// This mode should use only one controller because only one S2E plugin work
void DescriptorFuzzMode(UINT8 deviceCode) {
    std::srand(std::time(nullptr));
    // get current connected and working USB devices
    PZZWSTR devices = GetPresentDeviceList((LPGUID)&GUID_DEVINTERFACE_UDE_BACKCHANNEL);
    std::vector<HANDLE> handles = GetDeviceHandles(devices);

    if (handles.size() == 0) {
        wprintf(L"No controllers found\n");
        return;
    }

    while (TRUE) {
        UINT64 seed = std::rand();
        WriteToSeedLog(seed);
        // this will unplug USB device from all controllers
        UnplugUSBDevice();

        PlugInUSBDevice(handles[0], deviceCode, seed, TRUE, TRUE, TRUE);
        // wait for enumeration and getting fuzzing
        Sleep(100);
    }
}
void AutoFuzzMode(BOOLEAN fuzzDesc, BOOLEAN savePV, USHORT deviceCode) {
    std::srand(std::time(nullptr));
    // get current connected and working USB devices
    PZZWSTR devices = GetPresentDeviceList((LPGUID)&GUID_DEVINTERFACE_UDE_BACKCHANNEL);
    std::vector<HANDLE> handles = GetDeviceHandles(devices);

    if (handles.size() == 0) {
        wprintf(L"No controllers found\n");
        return;
    }

    // infinity fuzz 
    while (TRUE) {
        UINT64 seed = std::rand();
        WriteToSeedLog(seed);
        // this will unplug USB device from all controllers
        UnplugUSBDevice();
        // each controller device will plug different USB device
        for (int i = 0; i < handles.size(); i++) {
            if (deviceCode != 0) {
                PlugInUSBDevice(handles[i], deviceCode, seed, fuzzDesc, savePV, FALSE);
                continue;
            }

            // Plug again (i+1 because mode 0 not for fuzzing)
            PlugInUSBDevice(handles[i], i + 1, seed, fuzzDesc, savePV, FALSE);
        }
        // sleep while enumeration
        Sleep(700);
        
        // repeatedly send signal for all
        for (int k = 0; k < 50; k++) {
            for (int i = 0; i < handles.size(); i++) {
                // trying to send interrupts
                GenerateInterrupt(handles[i]);
            }
            Sleep(2);
        }
    }
}