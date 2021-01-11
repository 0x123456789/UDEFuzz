/*++
Copyright (c) Microsoft Corporation

Module Name:

USBCom.c

Abstract:
    Implementation of functions defined in USBCom.h

--*/

#include "Misc.h"
#include "Driver.h"
#include "Device.h"
#include "usbdevice.h"
#include "USBCom.h"
#include "BackChannel.h"
#include "ucx/1.4/ucxobjects.h"
#include "USBCom.tmh"
#include "Descriptor.h"

#include "USBSCSI.h"
#include "USBHID.h"
#include "Fuzzer.h"




typedef struct _ENDPOINTQUEUE_CONTEXT {
    UDECXUSBDEVICE usbDeviceObj;
    WDFDEVICE      backChannelDevice;
} ENDPOINTQUEUE_CONTEXT, *PENDPOINTQUEUE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(ENDPOINTQUEUE_CONTEXT, GetEndpointQueueContext);


NTSTATUS
Io_AllocateContext(
    _In_ UDECXUSBDEVICE Object
)
/*++

Routine Description:

Object context allocation helper

Arguments:

Object - WDF object upon which to allocate the new context

Return value:

NTSTATUS. Could fail on allocation failure or the same context type already exists on the object

--*/
{
    NTSTATUS status;
    WDF_OBJECT_ATTRIBUTES attributes;

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, IO_CONTEXT);

    status = WdfObjectAllocateContext(Object, &attributes, NULL);

    if (!NT_SUCCESS(status)) {

        LogError(TRACE_DEVICE, "Unable to allocate new context for WDF object %p", Object);
        goto exit;
    }

exit:

    return status;
}


NTSTATUS CompleteRequestWithDescriptor(
    _In_ WDFREQUEST Request,
    _In_ DESCRIPTOR_INFO Descriptor
)
{
    PUCHAR buffer;
    ULONG length = 0;
    NTSTATUS status = UdecxUrbRetrieveBuffer(Request, &buffer, &length);

    if (!NT_SUCCESS(status)) {
        LogError(TRACE_DEVICE, "Can't get buffer for descriptor: %!STATUS!", status);
        return status;
    }

    if (length < Descriptor.Length) {
        LogError(TRACE_DEVICE, "Buffer too small");
        return STATUS_BUFFER_TOO_SMALL;
    }
    // copy report descriptor in buffer
    memcpy(buffer, Descriptor.Descriptor, Descriptor.Length);
    return STATUS_SUCCESS;
}





static VOID
IoEvtControlUrb(
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request,
    _In_ size_t OutputBufferLength,
    _In_ size_t InputBufferLength,
    _In_ ULONG IoControlCode
)
{
    WDF_USB_CONTROL_SETUP_PACKET setupPacket;
    NTSTATUS status;
    UNREFERENCED_PARAMETER(Queue);
    UNREFERENCED_PARAMETER(OutputBufferLength);
    UNREFERENCED_PARAMETER(InputBufferLength);

    PENDPOINTQUEUE_CONTEXT pEpQContext = GetEndpointQueueContext(Queue);
    WDFDEVICE backchannel = pEpQContext->backChannelDevice;
    PUDECX_BACKCHANNEL_CONTEXT pBackChannelContext = GetBackChannelContext(backchannel);

    //NT_VERIFY(IoControlCode == IOCTL_INTERNAL_USB_SUBMIT_URB);

    if (IoControlCode == IOCTL_INTERNAL_USB_SUBMIT_URB)
    {
        // These are on the control pipe.
        // We don't do anything special with these requests,
        // but this is where we would add processing for vendor-specific commands.

        status = UdecxUrbRetrieveControlSetupPacket(Request, &setupPacket);

        if (!NT_SUCCESS(status))
        {
            LogError(TRACE_DEVICE, "WdfRequest %p is not a control URB? UdecxUrbRetrieveControlSetupPacket %!STATUS!",
                Request, status);
            UdecxUrbCompleteWithNtStatus(Request, status);
            goto exit;
        }

        // if Get descriptor request (0x06)
       
        if (setupPacket.Packet.bRequest == 0x06) {
            //  descriptor type is report descriptor (0x22)
            if (setupPacket.Packet.wValue.Bytes.HiByte == 0x22) {
                LogInfo(TRACE_DEVICE, "[IoEvtControlUrb] Report descriptor is requested");
                // check if driver now really emulating HID device
                if (pBackChannelContext->FuzzingContext.Mode == HID_MOUSE_MODE ||
                    pBackChannelContext->FuzzingContext.Mode == HID_KEYBOARD_MODE) {
                    status = CompleteRequestWithDescriptor(Request, pBackChannelContext->Descriptors.Report);
                    UdecxUrbCompleteWithNtStatus(Request, status);
                    goto exit;
                }
                else {
                    LogError(TRACE_DEVICE, "report descriptor is requested, but mode not HID_MOUSE_MODE");
                }
            }
        }

        LogInfo(TRACE_DEVICE, "v44 control CODE %x, [type=%x dir=%x recip=%x] req=%x [wv = %x wi = %x wlen = %x]",
            IoControlCode,
            (int)(setupPacket.Packet.bm.Request.Type),
            (int)(setupPacket.Packet.bm.Request.Dir),
            (int)(setupPacket.Packet.bm.Request.Recipient),
            (int)(setupPacket.Packet.bRequest),
            (int)(setupPacket.Packet.wValue.Value),
            (int)(setupPacket.Packet.wIndex.Value),
            (int)(setupPacket.Packet.wLength)
        );

        UdecxUrbCompleteWithNtStatus(Request, STATUS_SUCCESS);
    }
    else
    {
        LogError(TRACE_DEVICE, "control NO submit code is %x", IoControlCode);
    }
exit:
    return;
}


static int _Test_rebound = 0;


static VOID
IoEvtBulkOutUrb(
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request,
    _In_ size_t OutputBufferLength,
    _In_ size_t InputBufferLength,
    _In_ ULONG IoControlCode
)
{
    //WDFREQUEST matchingRead;
    WDFDEVICE backchannel;
    PUDECX_BACKCHANNEL_CONTEXT pBackChannelContext;
    PENDPOINTQUEUE_CONTEXT pEpQContext;
    NTSTATUS status = STATUS_SUCCESS;
    PUCHAR transferBuffer;
    ULONG transferBufferLength = 0;
    //SIZE_T completeBytes = 0;

    PCOMMAND_BLOCK_WRAPPER cbw = NULL;

    UNREFERENCED_PARAMETER(OutputBufferLength);
    UNREFERENCED_PARAMETER(InputBufferLength);

    pEpQContext = GetEndpointQueueContext(Queue);
    backchannel = pEpQContext->backChannelDevice;
    pBackChannelContext = GetBackChannelContext(backchannel);

    if (IoControlCode != IOCTL_INTERNAL_USB_SUBMIT_URB)
    {
        LogError(TRACE_DEVICE, "WdfRequest BOUT %p Incorrect IOCTL %x, %!STATUS!",
            Request, IoControlCode, status);
        status = STATUS_INVALID_PARAMETER;
        goto exit;
    }

    status = UdecxUrbRetrieveBuffer(Request, &transferBuffer, &transferBufferLength);
    if (!NT_SUCCESS(status))
    {
        LogError(TRACE_DEVICE, "WdfRequest BOUT %p unable to retrieve buffer %!STATUS!",
            Request, status);
        goto exit;
    }
    
    if (IsSCSIRequest(transferBuffer, transferBufferLength)) {
        cbw = (PCOMMAND_BLOCK_WRAPPER)transferBuffer;
        LogInfo(TRACE_DEVICE, "[SCSI] Writing request with OPCODE %d", cbw->CBWCB[0]);
        SCSIWrite(&(pBackChannelContext->LastSCSIRequest), cbw);
        LogInfo(TRACE_DEVICE, "[SCSI] LastSCSIRequest addr: %p", &(pBackChannelContext->LastSCSIRequest));
        LogInfo(TRACE_DEVICE, "[SCSI] LastSCSIRequest handled: %d", pBackChannelContext->LastSCSIRequest.Handled);
    }
    
   

    //if (1) {
    //    PVOID gg = ExAllocatePool(PagedPool, transferBufferLength + 1);
    //    if (gg == NULL) {
    //        TraceEvents(TRACE_LEVEL_ERROR,
    //            TRACE_QUEUE,
    //            "Not enough memory to queue write, err= %!STATUS!", status);
    //        status = STATUS_INSUFFICIENT_RESOURCES;
    //    }

    //    //sizeof(COMMAND_BLOCK_WRAPPER);
    //    // copy
    //    memcpy(gg, transferBuffer, transferBufferLength);
    //    char* bb = gg;
    //    bb[transferBufferLength] = '\0';
    //    LogInfo(TRACE_DEVICE, "TRANSF BUFF bOUT: %s", bb);
    //    ExFreePool(gg);
    //}


    //// try to get us information about a request that may be waiting for this info
    //status = WRQueuePushWrite(
    //    &(pBackChannelContext->missionRequest),
    //    transferBuffer,
    //    transferBufferLength,
    //    &matchingRead);

    //if (matchingRead != NULL)
    //{
    //    PVOID rbuffer;
    //    SIZE_T rlen;

    //    // this is a back-channel read, not a USB read!
    //    status = WdfRequestRetrieveOutputBuffer(matchingRead, 1, &rbuffer, &rlen);

    //    if (!NT_SUCCESS(status))  {

    //        LogError(TRACE_DEVICE, "WdfRequest %p cannot retrieve mission completion buffer %!STATUS!",
    //            matchingRead, status);

    //    } else  {
    //        completeBytes = MINLEN(rlen, transferBufferLength);
    //        memcpy(rbuffer, transferBuffer, completeBytes);
    //    }

    //    WdfRequestCompleteWithInformation(matchingRead, status, completeBytes);

    //    LogInfo(TRACE_DEVICE, "Mission request %p completed with matching read %p", Request, matchingRead);
    //} else {
    //    LogInfo(TRACE_DEVICE, "Mission request %p enqueued", Request);
    //}



exit:
    // writes never pended, always completed
    UdecxUrbSetBytesCompleted(Request, transferBufferLength);
    UdecxUrbCompleteWithNtStatus(Request, status);
    return;
}





static VOID
IoEvtBulkInUrb(
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request,
    _In_ size_t OutputBufferLength,
    _In_ size_t InputBufferLength,
    _In_ ULONG IoControlCode
)
{
    NTSTATUS status = STATUS_SUCCESS;
    WDFDEVICE backchannel;
    PUDECX_BACKCHANNEL_CONTEXT pBackChannelContext;
    FUZZING_CONTEXT fuzzingContext;
    PENDPOINTQUEUE_CONTEXT pEpQContext;
    BOOLEAN bReady = FALSE;
    PUCHAR transferBuffer;
    ULONG transferBufferLength;
    ULONG responseLen = 0;
    SIZE_T completeBytes = 0;

    UNREFERENCED_PARAMETER(OutputBufferLength);
    UNREFERENCED_PARAMETER(InputBufferLength);

    pEpQContext = GetEndpointQueueContext(Queue);
    backchannel = pEpQContext->backChannelDevice;
    pBackChannelContext = GetBackChannelContext(backchannel);
    fuzzingContext = pBackChannelContext->FuzzingContext;

    if (IoControlCode != IOCTL_INTERNAL_USB_SUBMIT_URB)
    {
        LogError(TRACE_DEVICE, "WdfRequest BIN %p Incorrect IOCTL %x, %!STATUS!",
            Request, IoControlCode, status);
        status = STATUS_INVALID_PARAMETER;
        goto exit;
    }

    status = UdecxUrbRetrieveBuffer(Request, &transferBuffer, &transferBufferLength);
    if (!NT_SUCCESS(status))
    {
        LogError(TRACE_DEVICE, "WdfRequest BIN %p unable to retrieve buffer %!STATUS!",
            Request, status);
        goto exit;
    }

    /*LogInfo(TRACE_DEVICE, "Before: %d, %d, %d, %d",
        transferBuffer[0], transferBuffer[1], transferBuffer[2], transferBuffer[3]);*/


    if (fuzzingContext.Mode == SCSI_MODE) {
        SCSIHandleBulkInResponse(
            &pBackChannelContext->LastSCSIRequest,
            transferBuffer,
            transferBufferLength,
            &responseLen
        );
    }


    bReady = TRUE;
    completeBytes = responseLen;
    status = STATUS_SUCCESS;

    // mutating response from USB device
    if (fuzzingContext.Mode != NONE_MODE) {
        FuzzerMutate(transferBuffer, responseLen);
    }


    //WDFMEMORY  reqMemory;
    //// get memory -----------------------
    //status = WdfRequestRetrieveInputMemory(
    //    Request,
    //    &reqMemory
    //);

    //if (!NT_SUCCESS(status)) {
    //    LogError(TRACE_DEVICE, "No input buffer provided !!!! err= %!STATUS!", status);
    //}
    //else {
    //    size_t bufSize;
    //    PVOID b = WdfMemoryGetBuffer(reqMemory, &bufSize);

    //    PVOID gg = ExAllocatePool(PagedPool, bufSize + 1);
    //    if (gg == NULL) {
    //        TraceEvents(TRACE_LEVEL_ERROR,
    //            TRACE_QUEUE,
    //            "Not enough memory to queue write, err= %!STATUS!", status);
    //        status = STATUS_INSUFFICIENT_RESOURCES;
    //    }

    //    // copy
    //    memcpy(gg, b, bufSize);
    //    char* bb = gg;
    //    bb[bufSize] = '\0';
    //    LogInfo(TRACE_DEVICE, "INBUFF: %s", bb);
    //    ExFreePool(gg);
    //}

   

    //// return some random bytes for now ------------------------------

    //LARGE_INTEGER iSeed;
    //KeQuerySystemTime(&iSeed);
    //
    //for (ULONG i = 0; i < transferBufferLength; i++) {
    //    transferBuffer[i] = (UCHAR)0x41;
    //    //transferBuffer[i] = (UCHAR)RtlRandomEx(&iSeed.LowPart);
    //}

    //bReady = TRUE;
    //completeBytes = transferBufferLength;
    //status = STATUS_SUCCESS;

    // ---------------------------------------------------------------

    //// try to get us information about a request that may be waiting for this info
    //status = WRQueuePullRead(
    //    &(pBackChannelContext->missionCompletion),
    //    Request,
    //    transferBuffer,
    //    transferBufferLength,
    //    &bReady,
    //    &completeBytes);

    if (bReady)
    {
        UdecxUrbSetBytesCompleted(Request, (ULONG)completeBytes);
        UdecxUrbCompleteWithNtStatus(Request, status);
        LogInfo(TRACE_DEVICE, "Mission response %p completed with pre-existing data", Request);
    } else {
        LogInfo(TRACE_DEVICE, "Mission response %p pended", Request);
    }


exit:
    return;
}


static VOID
IoEvtCancelInterruptInUrb(
    IN WDFQUEUE Queue,
    IN WDFREQUEST  Request
)
{
    UNREFERENCED_PARAMETER(Queue);
    LogInfo(TRACE_DEVICE, "[IoEvtCancelInterruptInUrb] Canceling request %p", Request);
    UdecxUrbCompleteWithNtStatus(Request, STATUS_CANCELLED);
}


static VOID
IoCompletePendingRequest(
    _In_ UDECXUSBDEVICE Device,
    _In_ WDFREQUEST Request,
    _In_ DEVICE_INTR_FLAGS LatestStatus)
{
    UNREFERENCED_PARAMETER(LatestStatus);

    ULONG responseLen = 0;
    NTSTATUS status = STATUS_SUCCESS;
    PUCHAR transferBuffer;
    ULONG transferBufferLength;
    PUSB_CONTEXT pUsbContext;
    PUDECX_BACKCHANNEL_CONTEXT pBackChannelContext;


    pUsbContext = GetUsbDeviceContext(Device);
    pBackChannelContext = GetBackChannelContext(pUsbContext->ControllerDevice);

    status = UdecxUrbRetrieveBuffer(Request, &transferBuffer, &transferBufferLength);
    if (!NT_SUCCESS(status))
    {
        LogError(TRACE_DEVICE, "WdfRequest  %p unable to retrieve buffer %!STATUS!",
            Request, status);
        goto exit;
    }

    if (pBackChannelContext->FuzzingContext.Mode == HID_MOUSE_MODE) {
        HIDHandleMouseResponse(
            transferBuffer,
            transferBufferLength,
            &responseLen);
    }

    if (pBackChannelContext->FuzzingContext.Mode == HID_KEYBOARD_MODE) {
        HIDHandleKeyboardResponse(
            transferBuffer,
            transferBufferLength,
            &responseLen);
    }
    UdecxUrbSetBytesCompleted(Request, responseLen);

exit:
    UdecxUrbCompleteWithNtStatus(Request, status);
    return;

}



NTSTATUS
Io_RaiseInterrupt(
    _In_ UDECXUSBDEVICE    Device,
    _In_ DEVICE_INTR_FLAGS LatestStatus )
{
    PIO_CONTEXT pIoContext;
    WDFREQUEST request;
    NTSTATUS status = STATUS_SUCCESS;

    pIoContext = WdfDeviceGetIoContext(Device);
    
    status = WdfIoQueueRetrieveNextRequest( pIoContext->IntrDeferredQueue, &request);

    // no items in the queue?  it is safe to assume the device is sleeping
    if (!NT_SUCCESS(status))    {
        LogInfo(TRACE_DEVICE, "Save update and wake device as queue status was %!STATUS!", status);

        WdfSpinLockAcquire(pIoContext->IntrState.sync);
        pIoContext->IntrState.latestStatus = LatestStatus;
        if ((pIoContext->IntrState.numUnreadUpdates) < INTR_STATE_MAX_CACHED_UPDATES)
        {
            ++(pIoContext->IntrState.numUnreadUpdates);
        }
        WdfSpinLockRelease(pIoContext->IntrState.sync);

        UdecxUsbDeviceSignalWake(Device);
        LogWarning(TRACE_DEVICE, "[!] For USB 3.0 need may be need UdecxUsbDeviceSignalFunctionWake?");
        status = STATUS_SUCCESS;
    } else {
        IoCompletePendingRequest(Device, request, LatestStatus);
    }

    return status;
}



static VOID
IoEvtInterruptInUrb(
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request,
    _In_ size_t OutputBufferLength,
    _In_ size_t InputBufferLength,
    _In_ ULONG IoControlCode
)
{
    PIO_CONTEXT pIoContext;
    UDECXUSBDEVICE tgtDevice;
    NTSTATUS status = STATUS_SUCCESS;
    DEVICE_INTR_FLAGS LatestStatus = 0;
    PENDPOINTQUEUE_CONTEXT pEpQContext;

    BOOLEAN bHasData = FALSE;

    UNREFERENCED_PARAMETER(Request);
    UNREFERENCED_PARAMETER(OutputBufferLength);
    UNREFERENCED_PARAMETER(InputBufferLength);


    pEpQContext = GetEndpointQueueContext(Queue);

    tgtDevice = pEpQContext->usbDeviceObj;

    pIoContext = WdfDeviceGetIoContext(tgtDevice);


    if (IoControlCode != IOCTL_INTERNAL_USB_SUBMIT_URB)   {
        LogError(TRACE_DEVICE, "Invalid Interrupt/IN out IOCTL code %x", IoControlCode);
        status = STATUS_ACCESS_DENIED;
        goto exit;
    }

    // gate cached data we may have and clear it
    WdfSpinLockAcquire(pIoContext->IntrState.sync);
    if( pIoContext->IntrState.numUnreadUpdates > 0)
    {
        bHasData = TRUE;
        LatestStatus = pIoContext->IntrState.latestStatus;
    }
    pIoContext->IntrState.latestStatus = 0;
    pIoContext->IntrState.numUnreadUpdates = 0;
    WdfSpinLockRelease(pIoContext->IntrState.sync);


    if (bHasData)  {
        
        IoCompletePendingRequest(tgtDevice, Request, LatestStatus);

    } else {
        status = WdfRequestForwardToIoQueue(Request, pIoContext->IntrDeferredQueue);
        if (NT_SUCCESS(status)) {
            LogInfo(TRACE_DEVICE, "Request %p forwarded for later (IoControlCode: %d)", Request, IoControlCode);
        } else {
            LogError(TRACE_DEVICE, "ERROR: Unable to forward Request %p error %!STATUS!", Request, status);
            UdecxUrbCompleteWithNtStatus(Request, status);
        }
    }

exit:
    return;
}


static NTSTATUS
Io_CreateDeferredIntrQueue(
    _In_ WDFDEVICE   ControllerDevice,
    _In_ PIO_CONTEXT pIoContext )
{
    NTSTATUS status;
    WDF_IO_QUEUE_CONFIG queueConfig;

    pIoContext->IntrState.latestStatus = 0;
    pIoContext->IntrState.numUnreadUpdates = 0;

    //
    // Register a manual I/O queue for handling Interrupt Message Read Requests.
    // This queue will be used for storing Requests that need to wait for an
    // interrupt to occur before they can be completed.
    //
    WDF_IO_QUEUE_CONFIG_INIT(&queueConfig, WdfIoQueueDispatchManual);

    // when a request gets canceled, this is where we want to do the completion
    queueConfig.EvtIoCanceledOnQueue = IoEvtCancelInterruptInUrb;

    //
    // We shouldn't have to power-manage this queue, as we will manually 
    // purge it and de-queue from it whenever we get power indications.
    //
    queueConfig.PowerManaged = WdfFalse;

    status = WdfIoQueueCreate(ControllerDevice,
        &queueConfig,
        WDF_NO_OBJECT_ATTRIBUTES,
        &(pIoContext->IntrDeferredQueue)
    );

    if (!NT_SUCCESS(status)) {
        LogError(TRACE_DEVICE,
            "WdfIoQueueCreate failed 0x%x\n", status);
        goto Error;
    }


    status = WdfSpinLockCreate(WDF_NO_OBJECT_ATTRIBUTES,
        &(pIoContext->IntrState.sync));
    if (!NT_SUCCESS(status)) {
        LogError(TRACE_DEVICE,
            "WdfSpinLockCreate failed  %!STATUS!\n", status);
        goto Error;
    }

Error:
    return status;
}


NTSTATUS
Io_DeviceSlept(
    _In_ UDECXUSBDEVICE  Device
)
{
    PIO_CONTEXT pIoContext;
    pIoContext = WdfDeviceGetIoContext(Device);

    // thi will result in all current requests being canceled
    LogInfo(TRACE_DEVICE, "About to purge deferred request queue" );
    WdfIoQueuePurge(pIoContext->IntrDeferredQueue, NULL, NULL);

    return STATUS_SUCCESS;
}


NTSTATUS
Io_DeviceWokeUp(
    _In_ UDECXUSBDEVICE  Device
)
{
    PIO_CONTEXT pIoContext;
    pIoContext = WdfDeviceGetIoContext(Device);

    // thi will result in all current requests being canceled
    LogInfo(TRACE_DEVICE, "About to re-start paused deferred queue");
    WdfIoQueueStart(pIoContext->IntrDeferredQueue);

    return STATUS_SUCCESS;
}


NTSTATUS
Io_RetrieveEpQueue(
    _In_ UDECXUSBDEVICE  Device,
    _In_ UCHAR           EpAddr,
    _Out_ WDFQUEUE     * Queue
)
{
    NTSTATUS status;
    PIO_CONTEXT pIoContext;
    PUSB_CONTEXT pUsbContext;
    WDF_IO_QUEUE_CONFIG queueConfig;
    WDFDEVICE wdfController;
    WDFQUEUE *pQueueRecord = NULL;
    WDF_OBJECT_ATTRIBUTES  attributes;
    PFN_WDF_IO_QUEUE_IO_INTERNAL_DEVICE_CONTROL pIoCallback = NULL;

    status = STATUS_SUCCESS;
    pIoContext = WdfDeviceGetIoContext(Device);
    pUsbContext = GetUsbDeviceContext(Device);

    wdfController = pUsbContext->ControllerDevice;

    switch (EpAddr)
    {
    case USB_DEFAULT_ENDPOINT_ADDRESS:
        pQueueRecord = &(pIoContext->ControlQueue);
        pIoCallback = IoEvtControlUrb;
        break;

    case g_BulkOutEndpointAddress:
        pQueueRecord = &(pIoContext->BulkOutQueue);
        pIoCallback = IoEvtBulkOutUrb;
        break;

    case g_BulkInEndpointAddress:
        pQueueRecord = &(pIoContext->BulkInQueue);
        pIoCallback = IoEvtBulkInUrb;
        break;

    case g_InterruptEndpointAddress:
        status = Io_CreateDeferredIntrQueue(wdfController, pIoContext);
        pQueueRecord = &(pIoContext->InterruptUrbQueue);
        pIoCallback = IoEvtInterruptInUrb;
        break;

    default:
        LogError(TRACE_DEVICE, "Io_RetrieveEpQueue received unrecognized ep %x", EpAddr);
        status = STATUS_ILLEGAL_FUNCTION;
        goto exit;
    }


    *Queue = NULL;
    if (!NT_SUCCESS(status)) {
        goto exit;
    }

    if ( (*pQueueRecord)  == NULL) {

        PENDPOINTQUEUE_CONTEXT pEPQContext;
        WDF_IO_QUEUE_CONFIG_INIT(&queueConfig, WdfIoQueueDispatchSequential);

        //Sequential must specify this callback
        queueConfig.EvtIoInternalDeviceControl = pIoCallback;
        WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, ENDPOINTQUEUE_CONTEXT);

        status = WdfIoQueueCreate(wdfController,
            &queueConfig,
            &attributes,
            pQueueRecord);

        pEPQContext = GetEndpointQueueContext(*pQueueRecord);
        pEPQContext->usbDeviceObj      = Device;
        pEPQContext->backChannelDevice = wdfController; // this is a dirty little secret, so we contain it.

        if (!NT_SUCCESS(status)) {
            LogError(TRACE_DEVICE, "WdfIoQueueCreate failed for queue of ep %x %!STATUS!", EpAddr, status);
            goto exit;
        }
    }

    *Queue = (*pQueueRecord);

exit:

    return status;
}



VOID
Io_StopDeferredProcessing(
    _In_ UDECXUSBDEVICE  Device,
    _Out_ PIO_CONTEXT   pIoContextCopy
)
{
    PIO_CONTEXT pIoContext = WdfDeviceGetIoContext(Device);

    pIoContext->bStopping = TRUE;
    // plus this queue will no longer accept incoming requests
    WdfIoQueuePurgeSynchronously( pIoContext->IntrDeferredQueue);

    (*pIoContextCopy) = (*pIoContext);
}


VOID
Io_FreeEndpointQueues(
    _In_ PIO_CONTEXT   pIoContext
)
{

    WdfObjectDelete(pIoContext->IntrDeferredQueue);

    WdfIoQueuePurgeSynchronously(pIoContext->ControlQueue);
    WdfObjectDelete(pIoContext->ControlQueue);

    WdfIoQueuePurgeSynchronously(pIoContext->InterruptUrbQueue);
    WdfObjectDelete(pIoContext->InterruptUrbQueue);

    WdfIoQueuePurgeSynchronously(pIoContext->BulkInQueue);
    WdfObjectDelete(pIoContext->BulkInQueue);

    WdfIoQueuePurgeSynchronously(pIoContext->BulkOutQueue);
    WdfObjectDelete(pIoContext->BulkOutQueue);

}



