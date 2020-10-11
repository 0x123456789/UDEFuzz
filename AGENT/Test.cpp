//#include <Windows.h>
//#include <SetupAPI.h>
//#include <stdio.h>
//#include <stdlib.h>
//#include "Drivers.h"
//
//#pragma comment(lib, "setupapi.lib")
//
//
//int main()
//{
//    HDEVINFO hdevinfo = SetupDiGetClassDevsW(NULL, LR"(USB\VID_0951&PID_1666)",
//        NULL, DIGCF_ALLCLASSES);
//    if (hdevinfo == INVALID_HANDLE_VALUE)
//    {
//        DWORD err = GetLastError();
//        printf("SetupDiGetClassDevs: %u\n", err);
//        return 1;
//    }
//
//    SP_DEVINFO_DATA devinfo;
//    devinfo.cbSize = sizeof(devinfo);
//    if (!SetupDiEnumDeviceInfo(hdevinfo, 0, &devinfo))
//    {
//        DWORD err = GetLastError();
//        printf("SetupDiEnumDeviceInfo: %u %d\n", err, 0);
//        return 1;
//    }
//
//    DumpDeviceDriverFiles(hdevinfo, &devinfo);
//    return 1;
//
//    if (!SetupDiBuildDriverInfoList(hdevinfo, &devinfo, SPDIT_COMPATDRIVER)) {
//        printf("error %d\n", GetLastError());
//        return 1;
//    }
//
//    SP_DRVINFO_DATA_W drvdata;
//    drvdata.cbSize = sizeof(SP_DRVINFO_DATA_W);
//    BOOL worked = SetupDiEnumDriverInfoW(hdevinfo, &devinfo, SPDIT_COMPATDRIVER,
//        0, &drvdata);
//    if (worked) {
//        printf("Driver found: description: %ws, MfgName: %ws, ProviderName: %ws\n",
//            drvdata.Description, drvdata.MfgName, drvdata.ProviderName);
//    }
//    else {
//        DWORD err = GetLastError();
//        if (err == ERROR_NO_MORE_ITEMS)
//            printf("No driver found\n");
//        else {
//            printf("SetupDiEnumDriverInfoW: %d", err);
//            return 1;
//        }
//    }
//
//    ///SP_DRVINFO_DATA driverinfo;
//    //SetupDiGetDriverInfoDetail(hdevinfo, &devinfo, &drvdata, );
//    SP_DEVINSTALL_PARAMS_W deviceInstallParams;
//    deviceInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS_W);
//    worked = SetupDiGetDeviceInstallParamsW(hdevinfo, NULL, &deviceInstallParams);
//    if (worked) {
//        printf("gg)");
//
//    }
//    else {
//        DWORD err = GetLastError();
//        if (err == ERROR_NO_MORE_ITEMS)
//            printf("No driver found\n");
//        else {
//            printf("SetupDiGetDriverInstallParamsW: %d", err);
//            return 1;
//        }
//    }
//
//
//    SP_DRVINSTALL_PARAMS driverInstallParams;
//    driverInstallParams.cbSize = sizeof(SP_DRVINSTALL_PARAMS);
//    worked = SetupDiGetDriverInstallParamsW(hdevinfo, &devinfo, &drvdata, &driverInstallParams);
//    if (worked) {
//        printf("gg)");
//
//    }
//    else {
//        DWORD err = GetLastError();
//        if (err == ERROR_NO_MORE_ITEMS)
//            printf("No driver found\n");
//        else {
//            printf("SetupDiGetDriverInstallParamsW: %d", err);
//            return 1;
//        }
//    }
//
//
//
//
//    SP_DRVINFO_DETAIL_DATA_W driverInfoDetailData;
//    driverInfoDetailData.cbSize = sizeof(SP_DRVINFO_DETAIL_DATA_W);
//    SetupDiGetDriverInfoDetailW(hdevinfo, &devinfo, &drvdata, &driverInfoDetailData, sizeof(SP_DRVINFO_DETAIL_DATA_W), NULL);
//    if (worked) {
//        printf("gg)");
//
//    }
//    else {
//        DWORD err = GetLastError();
//        if (err == ERROR_NO_MORE_ITEMS)
//            printf("No driver found\n");
//        else {
//            printf("SetupDiGetDriverInstallParamsW: %d", err);
//            return 1;
//        }
//    }
//    return 0;
//}


//#include <windows.h>
//#include <psapi.h>
//#include <tchar.h>
//#include <stdio.h>
//
//// To ensure correct resolution of symbols, add Psapi.lib to TARGETLIBS
//// and compile with -DPSAPI_VERSION=1
//
//#define ARRAY_SIZE 1024
//
//int main(void)
//{
//    LPVOID drivers[ARRAY_SIZE];
//    DWORD cbNeeded;
//    int cDrivers, i;
//
//    if (EnumDeviceDrivers(drivers, sizeof(drivers), &cbNeeded) && cbNeeded < sizeof(drivers))
//    {
//        TCHAR szDriver[ARRAY_SIZE];
//
//        cDrivers = cbNeeded / sizeof(drivers[0]);
//
//        _tprintf(TEXT("There are %d drivers:\n"), cDrivers);
//        for (i = 0; i < cDrivers; i++)
//        {
//            if (GetDeviceDriverBaseName(drivers[i], szDriver, sizeof(szDriver) / sizeof(szDriver[0])))
//            {
//                _tprintf(TEXT("%d: %s\n"), i + 1, szDriver);
//            }
//        }
//    }
//    else
//    {
//        _tprintf(TEXT("EnumDeviceDrivers failed; array size needed is %d\n"), cbNeeded / sizeof(LPVOID));
//        return 1;
//    }
//
//    return 0;
//}