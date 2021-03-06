/*++

Module Name:

BackChannel.c - Offers interface for talking to the driver "offline", 
                for texst pourposes

Abstract:

    Implementation of interfaces declared in BackChannel.h.

    Notice that we are taking a shortcut here, by not explicitly creating
    a WDF object for the back-channel, and using the controller context
    as the back-channel context.
    This of course doesn't work if we want the back-channel to be per-device.
    But that's okay for the purposes of the demo. No point complicating
    the back-channel for this semantic issue, when it will likely
    be removed completely by any real product.

Environment:

Kernel-mode Driver Framework

--*/

#include "driver.h"
#include "usbdevice.h"
#include "Misc.h"
#include "USBCom.h"
#include "BackChannel.h"
#include "AgentControl.h"
#include "DriverInfo.h"
#include "Descriptor.h"
#include "Fuzzer.h"

#include <ntstrsafe.h>
#include "BackChannel.tmh"

NTSTATUS SetFuzzingContext(
    _In_ WDFREQUEST Request,
    _Inout_ WDFDEVICE ctrdevice
)
{
    PVOID  buffer;
    size_t  bufSize;

    NTSTATUS status = WdfRequestRetrieveInputBuffer(
        Request,
        sizeof(FUZZING_CONTEXT),
        &buffer,
        &bufSize
    );

    if (!NT_SUCCESS(status)) {
        LogError(TRACE_DEVICE, "%!FUNC! Unable to retrieve input buffer with device code");
        return status;
    }

    if (bufSize != sizeof(FUZZING_CONTEXT)) {
        LogError(TRACE_DEVICE, "%!FUNC! sizeof(FUZZING_CONTEXT) != size of input buffer");
        return STATUS_INVALID_BUFFER_SIZE;
    }

    PUDECX_USBCONTROLLER_CONTEXT pControllerContext = GetUsbControllerContext(ctrdevice);
    memcpy(&pControllerContext->FuzzingContext, buffer, sizeof(FUZZING_CONTEXT));

    LogInfo(TRACE_DEVICE, "New fuzzing seed: %llu", pControllerContext->FuzzingContext.Seed);

    status = FuzzerInit(pControllerContext->FuzzingContext.Seed);
    if (!NT_SUCCESS(status)) {
        LogError(TRACE_DEVICE, "%!FUNC! Unable init fuzzer");
        return status;
    }
    return status;
}


NTSTATUS
BackChannelInit(
    _In_ WDFDEVICE ctrdevice
)
{
    NTSTATUS status = STATUS_SUCCESS;
    PUDECX_USBCONTROLLER_CONTEXT pControllerContext;

    pControllerContext = GetUsbControllerContext(ctrdevice);

    status = WRQueueInit(ctrdevice, &(pControllerContext->missionRequest), FALSE);
    if (!NT_SUCCESS(status)) {
        LogError(TRACE_DEVICE, "Unable to initialize mission completion, err= %!STATUS!", status);
        goto exit;
    }

    status = WRQueueInit(ctrdevice, &(pControllerContext->missionCompletion), TRUE);
    if (!NT_SUCCESS(status)) {
        LogError(TRACE_DEVICE, "Unable to initialize mission request, err= %!STATUS!", status);
        goto exit;
    }

    status = SCSIInit(&(pControllerContext->LastSCSIRequest));
    if (!NT_SUCCESS(status)) {
        LogError(TRACE_DEVICE, "Unable to initialize LastSCSIRequest, err= %!STATUS!", status);
        goto exit;
    }

exit:
    return status;
}


VOID
BackChannelDestroy(
    _In_ WDFDEVICE ctrdevice
)
{
    PUDECX_USBCONTROLLER_CONTEXT pControllerContext;

    pControllerContext = GetUsbControllerContext(ctrdevice);

    WRQueueDestroy(&(pControllerContext->missionCompletion));
    WRQueueDestroy(&(pControllerContext->missionRequest));
    SCSIDestroy(&(pControllerContext->LastSCSIRequest));
}

VOID
BackChannelEvtRead(
    WDFQUEUE   Queue,
    WDFREQUEST Request,
    size_t     Length
)
{
    WDFDEVICE controller;
    PUDECX_USBCONTROLLER_CONTEXT pControllerContext;
    NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN bReady = FALSE;
    PVOID transferBuffer;
    SIZE_T transferBufferLength;
    SIZE_T completeBytes = 0;

    UNREFERENCED_PARAMETER(Length);

    controller = WdfIoQueueGetDevice(Queue); /// WdfIoQueueGetDevice
    pControllerContext = GetUsbControllerContext(controller);

    status = WdfRequestRetrieveOutputBuffer(Request, 1, &transferBuffer, &transferBufferLength);
    if (!NT_SUCCESS(status))
    {
        LogError(TRACE_DEVICE, "BCHAN WdfRequest read %p unable to retrieve buffer %!STATUS!",
            Request, status);
        goto exit;
    }

    // try to get us information about a request that may be waiting for this info
    status = WRQueuePullRead(
        &(pControllerContext->missionRequest),
        Request,
        transferBuffer,
        transferBufferLength,
        &bReady,
        &completeBytes);

    if (bReady)
    {
        WdfRequestCompleteWithInformation(Request, status, completeBytes);
        LogInfo(TRACE_DEVICE, "BCHAN Mission request %p filed with pre-existing data", Request);
    }
    else {
        LogInfo(TRACE_DEVICE, "BCHAN Mission request %p pended", Request);
    }


exit:
    return;

}



VOID
BackChannelEvtWrite(
    WDFQUEUE Queue,
    WDFREQUEST Request,
    size_t Length
)
{
    WDFDEVICE controller;
    WDFREQUEST matchingRead;
    PUDECX_USBCONTROLLER_CONTEXT pControllerContext;
    NTSTATUS status = STATUS_SUCCESS;
    PVOID transferBuffer;
    SIZE_T transferBufferLength;
    SIZE_T completeBytes = 0;

    UNREFERENCED_PARAMETER(Length);

    controller = WdfIoQueueGetDevice(Queue); /// WdfIoQueueGetDevice
    pControllerContext = GetUsbControllerContext(controller);

    status = WdfRequestRetrieveInputBuffer(Request, 1, &transferBuffer, &transferBufferLength);
    if (!NT_SUCCESS(status))
    {
        LogError(TRACE_DEVICE, "BCHAN WdfRequest write %p unable to retrieve buffer %!STATUS!",
            Request, status);
        goto exit;
    }

    // try to get us information about a request that may be waiting for this info
    status = WRQueuePushWrite(
        &(pControllerContext->missionCompletion),
        transferBuffer,
        transferBufferLength,
        &matchingRead);

    if (matchingRead != NULL)
    {
        PUCHAR rbuffer;
        ULONG rlen;

        // this is a USB read!
        status = UdecxUrbRetrieveBuffer(matchingRead, &rbuffer, &rlen);

        if (!NT_SUCCESS(status)) {

            LogError(TRACE_DEVICE, "BCHAN WdfRequest %p cannot retrieve mission completion buffer %!STATUS!",
                matchingRead, status);
        }
        else {
            completeBytes = MINLEN(rlen, transferBufferLength);
            memcpy(rbuffer, transferBuffer, completeBytes);
        }

        UdecxUrbSetBytesCompleted(matchingRead, (ULONG)completeBytes);
        UdecxUrbCompleteWithNtStatus(matchingRead, status);

        LogInfo(TRACE_DEVICE, "BCHAN Mission completion %p delivered with matching USB read %p", Request, matchingRead);
    }
    else {
        LogInfo(TRACE_DEVICE, "BCHAN Mission completion %p enqueued", Request);
    }

exit:
    // writes never pended, always completed
    WdfRequestCompleteWithInformation(Request, status, transferBufferLength);
    return;
}




BOOLEAN
BackChannelIoctl(
    _In_ ULONG IoControlCode,
    _In_ WDFDEVICE ctrdevice,
    _In_ WDFREQUEST Request
)
{
    BOOLEAN handled = FALSE;
    NTSTATUS status;
    PDEVICE_INTR_FLAGS pflags = 0;
    size_t pblen;
    PUDECX_USBCONTROLLER_CONTEXT pControllerContext;

    pControllerContext = GetUsbControllerContext(ctrdevice);


    switch (IoControlCode)
    {
    case IOCTL_UNPLUG_USB_DEVICE:
        // if device already unplugged
        if (pControllerContext->ChildDevice == NULL) {
            status = STATUS_INVALID_DEVICE_STATE;
            goto unplug_exit;
        }


        status = Usb_Disconnect(ctrdevice);
        if (!NT_SUCCESS(status)) {
            TraceEvents(TRACE_LEVEL_ERROR,
                TRACE_DEVICE,
                "%!FUNC! Unable to disconect USB device");
        }
        else {
            Usb_Destroy(ctrdevice);
        }
        // Cleaning memory used for lock
        // FuzzerDestroy();

        unplug_exit:
        WdfRequestComplete(Request, status);
        handled = TRUE;
        break;

    case IOCTL_PLUG_USB_DEVICE:
        LogInfo(TRACE_DEVICE, "ChildDevice: 0x%p", pControllerContext->ChildDevice);
        LogInfo(TRACE_DEVICE, "ChildDeviceInit: 0x%p", pControllerContext->ChildDeviceInit);

        status = SetFuzzingContext(Request, ctrdevice);
        if (!NT_SUCCESS(status)) {
            TraceEvents(TRACE_LEVEL_ERROR,
                TRACE_DEVICE,
                "%!FUNC! Unable to set fuzzing context");
            goto gg;
        }

        
        DESCRIPTORS descriptorSet;
        DESCRIPTOR_POOL pool = GetDescriptorPool();

        // Fuzzing mode constant are the same as indexes of descriptors set
        // please also see Fuzzing.h
        switch (pControllerContext->FuzzingContext.Mode) {
        case NONE_MODE:
            descriptorSet = pool.Descriptors[DEFAULT_DESCRIPTOR_SET];
            break;
        case RESERVED_USB_30:
            descriptorSet = pool.Descriptors[KINGSTON_DESCRIPTOR_SET];
            break;
        case SCSI_MODE:
            descriptorSet = pool.Descriptors[FLASH_20_DESCRIPTOR_SET];
            break;
        case HID_MOUSE_MODE:
            descriptorSet = pool.Descriptors[HID_MOUSE_DESCRIPTOR_SET];
            break;
        case HID_KEYBOARD_MODE:
            descriptorSet = pool.Descriptors[HID_KEYBOARD_DESCRIPTOR_SET];
            break;
        case HID_JOYSTICK_MODE:
            descriptorSet = pool.Descriptors[HID_JOYSTICK_DESCRIPTOR_SET];
            break;
        default:
            TraceEvents(TRACE_LEVEL_ERROR, TRACE_DEVICE, "%!FUNC!: Unknown device code");
            status = STATUS_INVALID_PARAMETER;
            goto gg;
        }

        status = Usb_Initialize(
            ctrdevice,
            descriptorSet);

        if (!NT_SUCCESS(status)) {
            TraceEvents(TRACE_LEVEL_ERROR,
                TRACE_DEVICE,
                "%!FUNC! Unable to initialize USB device");
            goto gg;
        }
        status = Usb_CreateEndpointsAndPlugIn(ctrdevice);
        if (!NT_SUCCESS(status)) {
            TraceEvents(TRACE_LEVEL_ERROR,
                TRACE_DEVICE,
                "%!FUNC! Unable to plug-in USB device");
        }
        gg:
        WdfRequestComplete(Request, status);
        handled = TRUE;
        break;


    case IOCTL_GET_DRIVER_INFO:
        status = GetDriverByDriverObjectScan();
        if (!NT_SUCCESS(status))
        {
            TraceEvents(TRACE_LEVEL_ERROR,
                TRACE_DEVICE,
                "%!FUNC! Unable to retrieve driver information");
        }
        WdfRequestComplete(Request, status);
        handled = TRUE;
        break;

    case IOCTL_UDEFX2_GENERATE_INTERRUPT:
        status = WdfRequestRetrieveInputBuffer(Request,
            sizeof(DEVICE_INTR_FLAGS),
            &pflags,
            &pblen);// BufferLength

        if (!NT_SUCCESS(status))
        {
            TraceEvents(TRACE_LEVEL_ERROR,
                TRACE_QUEUE,
                "%!FUNC! Unable to retrieve input buffer");
        }
        else if (pblen == sizeof(DEVICE_INTR_FLAGS) && (pflags != NULL)) {
            DEVICE_INTR_FLAGS flags;
            memcpy(&flags, pflags, sizeof(DEVICE_INTR_FLAGS));
            TraceEvents(TRACE_LEVEL_INFORMATION,
                TRACE_QUEUE,
                "%!FUNC! Will generate interrupt");
            status = Io_RaiseInterrupt(pControllerContext->ChildDevice, flags);

        }
        else {
            TraceEvents(TRACE_LEVEL_ERROR,
                TRACE_QUEUE,
                "%!FUNC! Invalid buffer size");
            status = STATUS_INVALID_PARAMETER;
        }
        WdfRequestComplete(Request, status);
        handled = TRUE;
        break;
    }

    return handled;
}
