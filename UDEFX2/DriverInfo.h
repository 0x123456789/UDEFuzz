#pragma once

#include <ntddk.h>
#include <wdf.h>
#include <ntstrsafe.h>
#include <windef.h>

EXTERN_C_START

NTSYSAPI
NTSTATUS
NTAPI
ZwOpenDirectoryObject(
    OUT PHANDLE DirectoryHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES
    ObjectAttributes
);

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryDirectoryObject(
    IN HANDLE  DirectoryHandle,
    OUT PVOID  Buffer,
    IN ULONG   BufferLength,
    IN BOOLEAN ReturnSingleEntry,
    IN BOOLEAN RestartScan,
    IN OUT PULONG  Context,
    OUT PULONG ReturnLength OPTIONAL
);

NTSYSAPI
NTSTATUS
NTAPI
ObOpenObjectByName(
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN POBJECT_TYPE ObjectType OPTIONAL,
    IN KPROCESSOR_MODE AccessMode,
    IN OUT PACCESS_STATE AccessState OPTIONAL,
    IN ACCESS_MASK DesiredAccess OPTIONAL,
    IN OUT PVOID ParseContext OPTIONAL,
    OUT PHANDLE Handle);


typedef struct _DIRECTORY_BASIC_INFORMATION
{
    UNICODE_STRING ObjectName;
    UNICODE_STRING ObjectTypeName;
} DIRECTORY_BASIC_INFORMATION, * PDIRECTORY_BASIC_INFORMATION;


typedef struct _MODULE_ENTRY {
    LIST_ENTRY link;        // Flink, Blink
    BYTE unknown1[16];
    DWORD imageBase;
    PVOID  entryPoint;
    DWORD imageSize;
    UNICODE_STRING drvPath;
    UNICODE_STRING drvName;
    //...
} MODULE_ENTRY, * PMODULE_ENTRY;

typedef struct _DRIVERLISTENTRY {
    LIST_ENTRY lEntry;
    DWORD dwBase;
    DWORD dwEnd;
    PVOID  dwEntryPoint;
    char szDrvName[1024];
} DRIVERLISTENTRY, * PDRIVERLISTENTRY;


NTSTATUS GetDriverByDriverObjectScan();

EXTERN_C_END