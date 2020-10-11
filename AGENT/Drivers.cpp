#include <Windows.h>
#include <SetupAPI.h>
#include <stdio.h>
#include <stdlib.h>
#include "Drivers.h"

#pragma comment(lib, "setupapi.lib")


BOOL FindCurrentDriver(_In_ HDEVINFO Devs, _In_ PSP_DEVINFO_DATA DevInfo, _In_ PSP_DRVINFO_DATA DriverInfoData)
/*++
Routine Description:
    Find the driver that is associated with the current device
    We can do this either the quick way (available in WinXP)
    or the long way that works in Win2k.
Arguments:
    Devs    )_ uniquely identify device
    DevInfo )
Return Value:
    TRUE if we managed to determine and select current driver
--*/
{
    SP_DEVINSTALL_PARAMS deviceInstallParams;
    WCHAR SectionName[LINE_LEN];
    WCHAR DrvDescription[LINE_LEN];
    WCHAR MfgName[LINE_LEN];
    WCHAR ProviderName[LINE_LEN];
    HKEY hKey = NULL;
    DWORD RegDataLength;
    DWORD RegDataType;
    DWORD c;
    BOOL match = FALSE;
    long regerr;

    ZeroMemory(&deviceInstallParams, sizeof(deviceInstallParams));
    deviceInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);

    if (!SetupDiGetDeviceInstallParams(Devs, DevInfo, &deviceInstallParams)) {
        return FALSE;
    }

#ifdef DI_FLAGSEX_INSTALLEDDRIVER
    //
    // Set the flags that tell SetupDiBuildDriverInfoList to just put the
    // currently installed driver node in the list, and that it should allow
    // excluded drivers. This flag introduced in WinXP.
    //
    deviceInstallParams.FlagsEx |= (DI_FLAGSEX_INSTALLEDDRIVER | DI_FLAGSEX_ALLOWEXCLUDEDDRVS);

    if (SetupDiSetDeviceInstallParams(Devs, DevInfo, &deviceInstallParams)) {
        //
        // we were able to specify this flag, so proceed the easy way
        // we should get a list of no more than 1 driver
        //
        if (!SetupDiBuildDriverInfoList(Devs, DevInfo, SPDIT_CLASSDRIVER)) {
            return FALSE;
        }
        if (!SetupDiEnumDriverInfo(Devs, DevInfo, SPDIT_CLASSDRIVER,
            0, DriverInfoData)) {
            return FALSE;
        }
        //
        // we've selected the current driver
        //
        return TRUE;
    }
    deviceInstallParams.FlagsEx &= ~(DI_FLAGSEX_INSTALLEDDRIVER | DI_FLAGSEX_ALLOWEXCLUDEDDRVS);
#endif
    //  see long way in: https://github.com/microsoft/Windows-driver-samples/blob/master/setup/devcon/dump.cpp
    wprintf(L"[-] Can't set DI_FLAGSEX_INSTALLEDDRIVER and DI_FLAGSEX_ALLOWEXCLUDEDDRVS flags. \
        Please use way to find USB drivers via registry (long). \n");
    return FALSE;
}


UINT CALLBACK DumpDeviceDriversCallback(_In_ PVOID Context, _In_ UINT Notification, _In_ UINT_PTR Param1, _In_ UINT_PTR Param2)
/*++
Routine Description:
    if Context provided, Simply count
    otherwise dump files indented 2
Arguments:
    Context      - DWORD Count
    Notification - SPFILENOTIFY_QUEUESCAN
    Param1       - scan
Return Value:
    none
--*/
{
    LPDWORD count = (LPDWORD)Context;
    LPTSTR file = (LPTSTR)Param1;

    UNREFERENCED_PARAMETER(Notification);
    UNREFERENCED_PARAMETER(Param2);

    if (count) {
        count[0]++;
    }
    else {
        wprintf(L"%s\n", file);
      /*  Padding(2);
        _tprintf(TEXT("%s\n"), file);*/
    }

    return NO_ERROR;
}



BOOL DumpDeviceDriverFiles(_In_ HDEVINFO Devs, _In_ PSP_DEVINFO_DATA DevInfo)
/*++
Routine Description:
    Dump information about what files were installed for driver package
    <tab>Installed using OEM123.INF section [abc.NT]
    <tab><tab>file...
Arguments:
    Devs    )_ uniquely identify device
    DevInfo )
Return Value:
    none
--*/
{
    //
    // do this by 'searching' for the current driver
    // mimmicing a copy-only install to our own file queue
    // and then parsing that file queue
    //
    SP_DEVINSTALL_PARAMS deviceInstallParams;
    SP_DRVINFO_DATA driverInfoData;
    SP_DRVINFO_DETAIL_DATA driverInfoDetail;
    HSPFILEQ queueHandle = INVALID_HANDLE_VALUE;
    DWORD count;
    DWORD scanResult;
    BOOL success = FALSE;

    ZeroMemory(&driverInfoData, sizeof(driverInfoData));
    driverInfoData.cbSize = sizeof(driverInfoData);

    if (!FindCurrentDriver(Devs, DevInfo, &driverInfoData)) {
        return FALSE;
    }

    //
    // get useful driver information
    //
    driverInfoDetail.cbSize = sizeof(SP_DRVINFO_DETAIL_DATA);
    if (!SetupDiGetDriverInfoDetail(Devs, DevInfo, &driverInfoData, &driverInfoDetail, sizeof(SP_DRVINFO_DETAIL_DATA), NULL) &&
        GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
        //
        // no information about driver or section
        //
        wprintf(L"[-] SetupDiGetDriverInfoDetail error: %d\n", GetLastError());
        goto final;
    }
    if (!driverInfoDetail.InfFileName[0] || !driverInfoDetail.SectionName[0]) {
        wprintf(L"[-] Can't find INF file or section name\n");
        goto final;
    }

    //
    // pretend to do the file-copy part of a driver install
    // to determine what files are used
    // the specified driver must be selected as the active driver
    //
    if (!SetupDiSetSelectedDriver(Devs, DevInfo, &driverInfoData)) {
        wprintf(L"[-] SetupDiSetSelectedDriver error: %d\n", GetLastError());
        goto final;
    }

    //
    // create a file queue so we can look at this queue later
    //
    queueHandle = SetupOpenFileQueue();

    if (queueHandle == (HSPFILEQ)INVALID_HANDLE_VALUE) {
        wprintf(L"[-] Can't create file queue\n");
        goto final;
    }

    //
    // modify flags to indicate we're providing our own queue
    //
    ZeroMemory(&deviceInstallParams, sizeof(deviceInstallParams));
    deviceInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
    if (!SetupDiGetDeviceInstallParams(Devs, DevInfo, &deviceInstallParams)) {
        wprintf(L"[-] SetupDiGetDeviceInstallParams error: %d\n", GetLastError());
        goto final;
    }

    //
    // we want to add the files to the file queue, not install them!
    //
    deviceInstallParams.FileQueue = queueHandle;
    deviceInstallParams.Flags |= DI_NOVCP;

    if (!SetupDiSetDeviceInstallParams(Devs, DevInfo, &deviceInstallParams)) {
        wprintf(L"[-] SetupDiSetDeviceInstallParams error: %d\n", GetLastError());
        goto final;
    }

    //
    // now fill queue with files that are to be installed
    // this involves all class/co-installers
    //
    if (!SetupDiCallClassInstaller(DIF_INSTALLDEVICEFILES, Devs, DevInfo)) {
        wprintf(L"[-] SetupDiCallClassInstaller error: %d\n", GetLastError());
        goto final;
    }

    //
    // we now have a list of delete/rename/copy files
    // iterate the copy queue twice - 1st time to get # of files
    // 2nd time to get files
    // (WinXP has API to get # of files, but we want this to work
    // on Win2k too)
    //

    count = 0;
    scanResult = 0;
    //
    // call once to count
    //
    SetupScanFileQueue(queueHandle, SPQ_SCAN_USE_CALLBACK, NULL, DumpDeviceDriversCallback, &count, &scanResult);
    //
    // call again to dump the files
    //
    SetupScanFileQueue(queueHandle, SPQ_SCAN_USE_CALLBACK, NULL, DumpDeviceDriversCallback, NULL, &scanResult);

    success = TRUE;

final:

    SetupDiDestroyDriverInfoList(Devs, DevInfo, SPDIT_CLASSDRIVER);

    if (queueHandle != (HSPFILEQ)INVALID_HANDLE_VALUE) {
        SetupCloseFileQueue(queueHandle);
    }

    if (!success) {
        wprintf(L"[-] SetupDiCallClassInstaller error: %d\n", GetLastError());
    }

    return success;

}