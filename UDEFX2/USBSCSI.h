#pragma once

#include <ntddk.h>
#include <wdf.h>
#include <scsi.h>

#include "trace.h"

#pragma warning(disable:4201) // nonstandard extension used : nameless struct/union
#pragma warning(disable:4214) // nonstandard extension used : bit field types other than int


#define CDB_LEN 16

EXTERN_C_START

// Queue to store SCSI commands which are comming in bulk-out communication.
// We need reply for them then bulk-in communication will be issured.

// For now we store only command descriptor block (CDB) with mandatory type of request:
// * TEST UNIT READY
// * REQUEST SENSE
// * INQUIRY 

#pragma pack(push, com_bl, 1)
typedef struct _COMMAND_BLOCK_WRAPPER {
    UINT8 dCBWSignature[4];
    UINT32 dCBWTag;
    UINT32 dCBWDataTransferLength;
    UINT8 bmCBWFlags;
    UINT8 bCBWLUN;
    UINT8 bCBWCBLength;
    UINT8 CBWCB[CDB_LEN];
} COMMAND_BLOCK_WRAPPER, * PCOMMAND_BLOCK_WRAPPER;
#pragma pack(pop, com_bl)

#pragma pack(push, com_st, 1)
typedef struct _COMMAND_STATUS_WRAPPER {
    UINT32 dCSWSignature;
    UINT32 dCSWTag;
    UINT32 dCSWDataResidue;
    UINT8 bCSWStatus;
} COMMAND_STATUS_WRAPPER, * PCOMMAND_STATUS_WRAPPER;
#pragma pack(pop, com_st)


#pragma pack(push, inquiry, 1)
typedef struct _INQUIRY {
    UCHAR DeviceType : 5;
    UCHAR DeviceTypeQualifier : 3;
    UCHAR DeviceTypeModifier : 7;
    UCHAR RemovableMedia : 1;
    union {
        UCHAR Versions;
        struct {
            UCHAR ANSIVersion : 3;
            UCHAR ECMAVersion : 3;
            UCHAR ISOVersion : 2;
        };
    };
    UCHAR ResponseDataFormat : 4;
    UCHAR HiSupport : 1;
    UCHAR NormACA : 1;
    UCHAR TerminateTask : 1;
    UCHAR AERC : 1;
    UCHAR AdditionalLength;
    union {
        UCHAR Reserved;
        struct {
            UCHAR PROTECT : 1;
            UCHAR Reserved_1 : 2;
            UCHAR ThirdPartyCoppy : 1;
            UCHAR TPGS : 2;
            UCHAR ACC : 1;
            UCHAR SCCS : 1;
        };
    };
    UCHAR Addr16 : 1;               // defined only for SIP devices.
    UCHAR Addr32 : 1;               // defined only for SIP devices.
    UCHAR AckReqQ : 1;               // defined only for SIP devices.
    UCHAR MediumChanger : 1;
    UCHAR MultiPort : 1;
    UCHAR ReservedBit2 : 1;
    UCHAR EnclosureServices : 1;
    UCHAR ReservedBit3 : 1;
    UCHAR SoftReset : 1;
    UCHAR CommandQueue : 1;
    UCHAR TransferDisable : 1;      // defined only for SIP devices.
    UCHAR LinkedCommands : 1;
    UCHAR Synchronous : 1;          // defined only for SIP devices.
    UCHAR Wide16Bit : 1;            // defined only for SIP devices.
    UCHAR Wide32Bit : 1;            // defined only for SIP devices.
    UCHAR RelativeAddressing : 1;
    UCHAR VendorId[8];
    UCHAR ProductId[16];
    UCHAR ProductRevisionLevel[4];
} INQUIRY, * PINQUIRY;
#pragma pack(pop, inquiry)


#pragma pack(push, read_10, 1)
typedef struct _READ10 {
    UCHAR bOpcode;
    UCHAR bFlags;
    UINT32 LogicalBlockAddr;
    UINT16 TransferLength;
    UCHAR GroupNumber;
    UCHAR Control;
} READ10, * PREAD10;
#pragma pack(pop, read_10)

#pragma pack(push, write_10, 1)
typedef struct _WRITE10 {
    UCHAR bOpcode;
    UCHAR bFlags;
    UINT32 LogicalBlockAddr;
    UINT16 TransferLength;
    UCHAR GroupNumber;
    UCHAR Control;
} WRITE10, * PWRITE10;
#pragma pack(pop, write_10)




typedef struct _SCSI {
    COMMAND_BLOCK_WRAPPER cbw;
    BOOLEAN Handled;
    WDFSPINLOCK qsync;
} SCSI, * PSCSI;


NTSTATUS
SCSIInit(
    _Inout_ PSCSI pSCSI
);

VOID
SCSIDestroy(
    _Inout_ PSCSI pSCSI
);

VOID
SCSIWrite(
    _Inout_ PSCSI pSCSI,
    _In_ PCOMMAND_BLOCK_WRAPPER cbw
);

BOOLEAN IsSCSIRequest(
    _In_ PUCHAR buffer,
    _In_ ULONG bufSize
);


BOOLEAN SCSIHandleBulkInResponse(
    _Inout_ PSCSI pSCSI,
    _Inout_ PUCHAR ResponseBuffer,
    _In_ ULONG ResponseBufferLen,
    _Out_ PULONG ResponseLen
);

EXTERN_C_END