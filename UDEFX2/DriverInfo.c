/*++

Module Name:

    DriverInfo.c

Abstract:

    This file contains functions to access DRIVER_OBJECT of target USB driver and get info about supported IRP.

Environment:

    Kernel-mode Driver Framework

--*/


#include "DriverInfo.h"

/*++
* @method: IsDummyModuleEntry2
*
* @description: Checks if a kernel module entry is valid or not
*
* @input: PMODULE_ENTRY pModuleToChk
*
* @output: BOOLEAN
*
*--*/
BOOLEAN IsDummyModuleEntry2(PMODULE_ENTRY pModuleToChk)
{
    BOOLEAN bDummy = FALSE;
    __try
    {
        if (MmIsAddressValid(pModuleToChk))
        {
            if ((0 == pModuleToChk->drvPath.Length) ||
                (0 == pModuleToChk->imageSize) ||
                (0 == pModuleToChk->imageBase))
            {
                bDummy = TRUE;
            }
        }
        else
        {
            bDummy = TRUE;
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        bDummy = TRUE;
        DbgPrint("Exception caught in IsDummyModuleEntry2()");
    }
    return bDummy;
}

__declspec(dllimport) POBJECT_TYPE* IoDriverObjectType;

/*++
* @method: GetDriverByDriverObjectScan
*
* @description: Gets loaded drivers by browsing \Driver\ directory in Object Manager
*
* @input: None
*
* @output: NTSTATUS
*
*--*/


NTSTATUS GetDriverByDriverObjectScan()
{
    NTSTATUS retVal = STATUS_UNSUCCESSFUL;
    __try
    {
        HANDLE hObjMgr = NULL;
        OBJECT_ATTRIBUTES objAttr;
        UNICODE_STRING usDriverObj;
        
        // Open Object Manager
        RtlInitUnicodeString(&usDriverObj, L"\\Driver");
        InitializeObjectAttributes(&objAttr,
            &usDriverObj,
            OBJ_CASE_INSENSITIVE,
            NULL, NULL);

      
        // Otherwise, use ZwOpenDirectoryObject
        retVal = ZwOpenDirectoryObject(&hObjMgr, DIRECTORY_QUERY | DIRECTORY_TRAVERSE, &objAttr);

        if (STATUS_SUCCESS == retVal)
        {
            DRIVERLISTENTRY drvEntry;
            HANDLE hObject = NULL;
            PVOID pObject = NULL;
            PDRIVER_OBJECT pDrvObj = NULL;
            PMODULE_ENTRY pModEntry = NULL;
            char szBuffer[1024];
            WCHAR wszObjName[1024];
            PDIRECTORY_BASIC_INFORMATION pDirBasicInfo = NULL;
            ULONG actualLen = 0;
            ULONG curPos = 0;

            while (1)
            {
                // Get one object
                RtlZeroMemory(szBuffer, 1024);

               
                // Otherwise, use ZwQueryDirectoryObject
                retVal = ZwQueryDirectoryObject(hObjMgr,
                    szBuffer,
                    1024,
                    TRUE,
                    FALSE,
                    &curPos,
                    &actualLen);
                

                if (STATUS_SUCCESS == retVal)
                {
                    // Extract the driver object name
                    pDirBasicInfo = (PDIRECTORY_BASIC_INFORMATION)szBuffer;
                    if (MmIsAddressValid(pDirBasicInfo) && MmIsAddressValid(pDirBasicInfo->ObjectName.Buffer))
                    {
                        // Construct name
                        RtlZeroMemory(wszObjName, (sizeof(WCHAR) * 1024));
                        RtlStringCchCopyW(wszObjName, 1024, L"\\Driver\\");
                        RtlStringCchCatW(wszObjName, 1024, pDirBasicInfo->ObjectName.Buffer);
                        RtlInitUnicodeString(&usDriverObj, wszObjName);
                        InitializeObjectAttributes(&objAttr,
                            &usDriverObj,
                            OBJ_CASE_INSENSITIVE,
                            NULL, NULL);

                        // Open object
                        retVal = ObOpenObjectByName(&objAttr,
                            *IoDriverObjectType,
                            KernelMode,
                            NULL,
                            GENERIC_READ,
                            NULL,
                            &hObject);

                        if (STATUS_SUCCESS == retVal)
                        {
                            // may be it's better to use ObReferenceObjectByName 
                            // https://stackoverflow.com/questions/45743841/getting-windows-nt-kernel-object-by-name-from-within-a-kernel-driver


                            // Get object from handle
                            retVal = ObReferenceObjectByHandle(hObject,
                                GENERIC_READ,
                                NULL,
                                KernelMode,
                                &pObject,
                                NULL);
                            if (STATUS_SUCCESS == retVal)
                            {
                                if (MmIsAddressValid(pObject))
                                {
                                    // Get driver object from device object
                                    pDrvObj = (PDRIVER_OBJECT)pObject;

                                    // Get DriverSection from driver object
                                    pModEntry = (PMODULE_ENTRY)pDrvObj->DriverSection;
                                    if (!IsDummyModuleEntry2(pModEntry))
                                    {
                                        DbgPrint("IN IF\n");
                                        // Copy driver details to our list entry
                                        RtlZeroMemory(&drvEntry, sizeof(DRIVERLISTENTRY));
                                        drvEntry.dwBase = pModEntry->imageBase;
                                        drvEntry.dwEnd = pModEntry->imageBase + pModEntry->imageSize;
                                        drvEntry.dwEntryPoint = pModEntry->entryPoint;
                                        RtlStringCchPrintfA(drvEntry.szDrvName, 1024, "%S", pModEntry->drvPath.Buffer);
                                        DbgPrint("%s\n", drvEntry.szDrvName);
                                        DbgPrint("OUT IF\n");
                                        // Add it to our list
                                        //retVal = AddListEntry(eDrvList, &drvEntry, TRUE);
                                    }
                                    else if (pDrvObj->DriverName.Length > 0)
                                    {
                                        __try
                                        {
                                            DbgPrint("IN ELSE");
                                            // Copy driver details to our list entry
                                            RtlZeroMemory(&drvEntry, sizeof(DRIVERLISTENTRY));
                                            drvEntry.dwEntryPoint = (PVOID)(pDrvObj->DriverInit);
                                            RtlStringCchPrintfA(drvEntry.szDrvName, 1024, "%S", pDrvObj->DriverName.Buffer);
                                            DbgPrint("%s\n", drvEntry.szDrvName);
                                            DbgPrint("%p\n", pDrvObj->MajorFunction);
                                            DbgPrint("%p\n", pDrvObj->MajorFunction[IRP_MJ_CREATE]);
                                            DbgPrint("%p\n", pDrvObj->MajorFunction[IRP_MJ_PNP]);
                                            DbgPrint("%p\n", pDrvObj->MajorFunction[IRP_MJ_DEVICE_CONTROL]);

                                            if (pDrvObj->MajorFunction[IRP_MJ_PNP]) {
                                                DbgPrint("gg --------> !! %p\n", pDrvObj->MajorFunction[IRP_MJ_PNP]);
                                            }

                                           /* if (MmIsAddressValid(pDrvObj->MajorFunction[IRP_HOLD_DEVICE_QUEUE]))
                                            {
                                                DbgPrint("%p\n", pDrvObj->MajorFunction[IRP_HOLD_DEVICE_QUEUE]);
                                            }
                                            else {
                                                DbgPrint("invalid\n");
                                            }*/
                                            DbgPrint("OUT ELSE\n");
                                            // Add it to our list
                                            //retVal = AddListEntry(eDrvList, &drvEntry, TRUE);
                                        }
                                        __except (EXCEPTION_EXECUTE_HANDLER)
                                        {
                                            DbgPrint("Exception gg !!!!!\n");
                                        }
                                    }
                                }

                                // Dereference the device object
                                ObDereferenceObject(pObject);
                                pObject = NULL;
                            }
                            else
                            {
                                DbgPrint("GetDriverByDriverObjectScan: ObReferenceObjectByHandle for %S, failed: 0x%x",
                                    usDriverObj.Buffer, retVal);
                            }
                            ZwClose(hObject);
                        }
                        else
                        {
                            DbgPrint("GetDriverByDriverObjectScan: ObOpenObjectByName for %S, failed: 0x%x",
                                usDriverObj.Buffer, retVal);
                        }
                    }
                }
                else
                {
                    DbgPrint("GetDriverByDriverObjectScan: ZwQueryDirectoryObject failed: 0x%x", retVal);
                    break;
                }
            }
            ZwClose(hObjMgr);
        }
        else
        {
            DbgPrint("GetDriverByDriverObjectScan: ZwOpenDirectoryObject for %S failed: 0x%x",
                usDriverObj.Buffer, retVal);
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        retVal = STATUS_UNSUCCESSFUL;
        DbgPrint("Exception caught in GetDriverByDriverObjectScan()");
    }

    return retVal;
}