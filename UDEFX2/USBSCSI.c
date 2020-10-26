
#include "Driver.h"
#include "Device.h"
#include "usbdevice.h"

#include "USBSCSI.h"
#include "USBSCSI.tmh"

INQUIRY DefaultInquiry() {
    INQUIRY inquiry;

    inquiry.DeviceType = DIRECT_ACCESS_DEVICE;
    inquiry.DeviceTypeQualifier = DEVICE_CONNECTED;
    inquiry.RemovableMedia = 0x01;          // Removable
    inquiry.Versions = 0x02;                // Compliance to ANSI X3.131:1994
    inquiry.ResponseDataFormat = 0x02;
    inquiry.AdditionalLength = 0x1F;
    memcpy(inquiry.VendorId,  "ABCD", sizeof("ABCD"));
    memcpy(inquiry.ProductId, "ABCD", sizeof("ABCD"));
    memcpy(inquiry.ProductRevisionLevel, "123", sizeof("123"));
    return inquiry;
}

READ_CAPACITY_DATA DefaultCapacity() {
    READ_CAPACITY_DATA capacity;
    capacity.LogicalBlockAddress = 0xFF6F7800;
    capacity.BytesPerBlock = 0x00020000;
    return capacity;
}

MODE_PARAMETER_HEADER DefaultHeader() {
    MODE_PARAMETER_HEADER header;
    header.ModeDataLength = 0x00;
    header.MediumType = 0x00;
    header.DeviceSpecificParameter = 0x00;
    header.BlockDescriptorLength = 0x00;
    return header;
}

COMMAND_STATUS_WRAPPER CommandStatus(UINT32 Tag, UINT8 Status) {
    COMMAND_STATUS_WRAPPER csw;
    csw.dCSWSignature = 0x53425355;
    csw.dCSWTag = Tag;
    csw.dCSWDataResidue = 0x00;
    csw.bCSWStatus = Status;
    return csw;
}

NTSTATUS
SCSIInit(
    _Inout_ PSCSI pSCSI
)
{
    return WdfSpinLockCreate(WDF_NO_OBJECT_ATTRIBUTES, &(pSCSI->qsync));
}


VOID
SCSIDestroy(
    _Inout_ PSCSI pSCSI
) {
    if (pSCSI->qsync == NULL) {
        return; // init has not even started
    }
    WdfObjectDelete(pSCSI->qsync);
    pSCSI->qsync = NULL;
}


VOID
SCSIWrite(
    _Inout_ PSCSI pSCSI,
    _In_ PCOMMAND_BLOCK_WRAPPER cbw
)
{
    WdfSpinLockAcquire(pSCSI->qsync);
    memcpy(&pSCSI->cbw, cbw, sizeof(COMMAND_BLOCK_WRAPPER));
    pSCSI->Handled = FALSE;
    WdfSpinLockRelease(pSCSI->qsync);
}


// see: https://cxem.net/mc/mc433.php

BOOLEAN SCSIHandleBulkInResponse(

    _Inout_ PSCSI pSCSI,
    _Inout_ PUCHAR ResponseBuffer,
    _In_ ULONG ResponseBufferLen,
    _Out_ PULONG ResponseLen
) 
{ 
    PCDB cdb = NULL;
    
    WdfSpinLockAcquire(pSCSI->qsync);

    LogInfo(TRACE_DEVICE, "[SCSI] OPCODE, Handled: %d, %d", pSCSI->cbw.CBWCB[0], pSCSI->Handled);

    if (!pSCSI->Handled) {

        pSCSI->Handled = TRUE;

        cdb = (PCDB)pSCSI->cbw.CBWCB;

        switch (pSCSI->cbw.CBWCB[0])
        {
        case SCSIOP_INQUIRY:

            // control block can be 6, 10, 12, 32 and variable size length
            // for now we support blocks with 6 byte size only

            if (pSCSI->cbw.bCBWCBLength == 0x06) {
                
                // if EVPD bit equal to 0, the device returns a standard response to INQUIRY;
                // if 1 - then the host requests specific information, which can be determined by the PAGE CODE field.
                // we only reply with success if EVPD and CMDDT equals to 0, else we will reply with error
                
                if (cdb->CDB6INQUIRY3.EnableVitalProductData == 0x00) {

                    if (cdb->CDB6INQUIRY3.AllocationLength == sizeof(INQUIRY)) {
                        // if don't we have enouth space to save our response
                        if (ResponseBufferLen < sizeof(INQUIRY)) {
                            LogError(TRACE_DEVICE, "[ERROR] Can't copy response to buffer: ResponseBufferLen < sizeof(INQUIRY)");
                            goto notSupported;
                        }
                        // if all is ok copy response into buffer
                        // and return number of copyed bytes
                        INQUIRY inquiry = DefaultInquiry();
                        memcpy(ResponseBuffer, &inquiry, sizeof(INQUIRY));
                        (*ResponseLen) = sizeof(INQUIRY);

                        /*LogInfo(TRACE_DEVICE, "IN: %d, %d, %d, %d",
                            ResponseBuffer[0], ResponseBuffer[1], ResponseBuffer[2], ResponseBuffer[3]);*/

                    }
                    else {
                        // if expected another struct in answer
                        LogError(TRACE_DEVICE, "[ERROR] Another response expected: AllocationLength != sizeof(INQUIRY)");

                    }
                }
                else {
                    // as we said earlier, requests for specific information are not supported
                    LogWarning(TRACE_DEVICE, "[WARNING] Get unsupported request with non-zero (EVPD, CMDDT): (%d, %d).",
                        cdb->CDB6INQUIRY3.EnableVitalProductData, cdb->CDB6INQUIRY3.CommandSupportData);
                    goto notSupported;
                }
            }
            else {
                // unsupported command block length
                LogWarning(TRACE_DEVICE, "[WARNING] Get unsupported request with bCBWCBLength: %d",
                    pSCSI->cbw.bCBWCBLength);
                goto notSupported;
            }

            break;
        
        case SCSIOP_READ_CAPACITY:
            // if don't we have enouth space to save our response
            if (ResponseBufferLen < sizeof(READ_CAPACITY_DATA)) {
                LogError(TRACE_DEVICE, "[ERROR] Can't copy response to buffer: ResponseBufferLen < sizeof(READ_CAPACITY_DATA)");
                goto notSupported;
            }

            READ_CAPACITY_DATA capacity = DefaultCapacity();
            memcpy(ResponseBuffer, &capacity, sizeof(READ_CAPACITY_DATA));
            (*ResponseLen) = sizeof(READ_CAPACITY_DATA);
            break;

        case SCSIOP_MODE_SENSE:
            // if don't we have enouth space to save our response
            if (ResponseBufferLen < sizeof(MODE_PARAMETER_HEADER)) {
                LogError(TRACE_DEVICE, "[ERROR] Can't copy response to buffer: ResponseBufferLen < sizeof(MODE_PARAMETER_HEADER)");
                goto notSupported;
            }

            MODE_PARAMETER_HEADER header = DefaultHeader();
            memcpy(ResponseBuffer, &header, sizeof(MODE_PARAMETER_HEADER));
            (*ResponseLen) = sizeof(MODE_PARAMETER_HEADER);
            break;

        case SCSIOP_TEST_UNIT_READY:
            // if don't we have enouth space to save our response
            if (ResponseBufferLen < sizeof(COMMAND_STATUS_WRAPPER)) {
                LogError(TRACE_DEVICE, "[ERROR] Can't copy response to buffer: ResponseBufferLen < sizeof(COMMAND_STATUS_WRAPPER)");
                goto notSupported;
            }

            COMMAND_STATUS_WRAPPER csw = CommandStatus(pSCSI->cbw.dCBWTag, 0x00);
            memcpy(ResponseBuffer, &csw, sizeof(COMMAND_STATUS_WRAPPER));
            (*ResponseLen) = sizeof(COMMAND_STATUS_WRAPPER);
            break;

        // case SCSIOP_READ_FORMATTED_CAPACITY:
            
        default:
            goto notSupported;

        }
    }
    else {
        // if request already handled, we need send status
        if (ResponseBufferLen < sizeof(COMMAND_STATUS_WRAPPER)) {
            LogError(TRACE_DEVICE, "[ERROR] Can't copy response to buffer: ResponseBufferLen < sizeof(COMMAND_STATUS_WRAPPER)");
            goto notSupported;
        }

        COMMAND_STATUS_WRAPPER csw = CommandStatus(pSCSI->cbw.dCBWTag, 0x00);
        memcpy(ResponseBuffer, &csw, sizeof(COMMAND_STATUS_WRAPPER));
        (*ResponseLen) = sizeof(COMMAND_STATUS_WRAPPER);

    }

    WdfSpinLockRelease(pSCSI->qsync);
    return TRUE;

notSupported:
    
    if (ResponseBufferLen >= sizeof(COMMAND_STATUS_WRAPPER)) {
        // command 
        COMMAND_STATUS_WRAPPER csw;
        csw.dCSWSignature = 0x53425355;
        csw.dCSWTag = pSCSI->cbw.dCBWTag;
        csw.dCSWDataResidue = pSCSI->cbw.dCBWDataTransferLength;
        csw.bCSWStatus = 0x01;

        memcpy(ResponseBuffer, &csw, sizeof(COMMAND_STATUS_WRAPPER));
        (*ResponseLen) = sizeof(COMMAND_STATUS_WRAPPER);
    }

    WdfSpinLockRelease(pSCSI->qsync);
    return FALSE;
}


// https://habr.com/ru/company/selectel/blog/478684/

BOOLEAN IsSCSIRequest(
    _In_ PUCHAR buffer,
    _In_ ULONG bufSize
) 
{
    PCOMMAND_BLOCK_WRAPPER cbw = NULL;

    LogInfo(TRACE_DEVICE, "[SCSI] Buf Size: %d", bufSize);

    if (bufSize == sizeof(COMMAND_BLOCK_WRAPPER)) {
        cbw = (PCOMMAND_BLOCK_WRAPPER)buffer;
        UINT32 signature = *(UINT32*)cbw->dCBWSignature;

        LogInfo(TRACE_DEVICE, "[SCSI] Signature bytes: %x %x %x %x",
            cbw->dCBWSignature[0], cbw->dCBWSignature[1],
            cbw->dCBWSignature[2], cbw->dCBWSignature[3]);

        LogInfo(TRACE_DEVICE, "[SCSI] Signature: get, want: %d, %d", signature, 0x43425355);
        
        if (signature == 0x43425355) {
            return TRUE;
        }
    }
    return FALSE;
}
