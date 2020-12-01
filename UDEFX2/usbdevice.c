/*++
Copyright (c) Microsoft Corporation

Module Name:

usbdevice.cpp

Abstract:


--*/

#include "Misc.h"
#include "Driver.h"
#include "Device.h"
#include "usbdevice.h"
#include "USBCom.h"
#include "ucx/1.4/ucxobjects.h"
#include "usbdevice.tmh"

UCHAR g_BOS[21] = {
        // BOS Descriptor
        0x05,                       // bLength
        USB_BOS_DESCRIPTOR_TYPE,    // bDescriptorType
        0x16, 0x00,                 // wTotalLength
        0x02,                       // bNumDeviceCaps

        // USB 2.0 extension descriptor
        0x07,                                   // bLength
        USB_DEVICE_CAPABILITY_DESCRIPTOR_TYPE,  // bDescriptorType
        USB_DEVICE_CAPABILITY_USB20_EXTENSION,  // bDevCapabilityType
        0x06, 0x00, 0x00, 0x00,                 // bmAttributes

        // SuperSpeed USB Device Capability Descriptor
        0x0A,                                   // bLength
        USB_DEVICE_CAPABILITY_DESCRIPTOR_TYPE,  // bDescriptorType
        USB_DEVICE_CAPABILITY_SUPERSPEED_USB,   // bDevCapabilityType
        0x00,                                   // bmAttributes
        0x0E,                                   // wSpeedsSupported
        0x02,                                   // bFunctionalitySupport (lower speed - high speed)
        0x0A,                                   // wU1DevExitLat (less than 10 micro-seconds)
        0xFF, 0x07,                             // wU2DevExitLat (less than 2047 micro-seconds)
};


#define UDECXMBIM_POOL_TAG 'UDEI'


////
//// Generic descriptor asserts
////
//static
//FORCEINLINE
//VOID
//UsbValidateConstants(
//)
//{
//    //
//    // C_ASSERT doesn't treat these expressions as constant, so use NT_ASSERT
//    //
//    NT_ASSERT(((PUSB_STRING_DESCRIPTOR)g_LanguageDescriptor)->bString[0] == AMERICAN_ENGLISH);
//    //NT_ASSERT(((PUSB_STRING_DESCRIPTOR)g_LanguageDescriptor)->bString[1] == PRC_CHINESE);
//    NT_ASSERT(((PUSB_DEVICE_DESCRIPTOR)g_UsbDeviceDescriptor)->iManufacturer == g_ManufacturerIndex);
//    NT_ASSERT(((PUSB_DEVICE_DESCRIPTOR)g_UsbDeviceDescriptor)->iProduct == g_ProductIndex);
//
//    NT_ASSERT(((PUSB_DEVICE_DESCRIPTOR)g_UsbDeviceDescriptor)->bLength ==
//        sizeof(USB_DEVICE_DESCRIPTOR));
//    NT_ASSERT(sizeof(g_UsbDeviceDescriptor) == sizeof(USB_DEVICE_DESCRIPTOR));
//    NT_ASSERT(((PUSB_CONFIGURATION_DESCRIPTOR)g_UsbConfigDescriptorSet)->wTotalLength ==
//        sizeof(g_UsbConfigDescriptorSet));
//    NT_ASSERT(((PUSB_STRING_DESCRIPTOR)g_LanguageDescriptor)->bLength ==
//        sizeof(g_LanguageDescriptor));
//
//    NT_ASSERT(((PUSB_DEVICE_DESCRIPTOR)g_UsbDeviceDescriptor)->bDescriptorType ==
//        USB_DEVICE_DESCRIPTOR_TYPE);
//    NT_ASSERT(((PUSB_CONFIGURATION_DESCRIPTOR)g_UsbConfigDescriptorSet)->bDescriptorType ==
//        USB_CONFIGURATION_DESCRIPTOR_TYPE);
//    NT_ASSERT(((PUSB_STRING_DESCRIPTOR)g_LanguageDescriptor)->bDescriptorType ==
//        USB_STRING_DESCRIPTOR_TYPE);
//}


// END ------------------ descriptor -------------------------------



NTSTATUS
Usb_Initialize(
    _In_ WDFDEVICE WdfDevice,
    _In_ DESCRIPTORS Descriptors
)
{
    NTSTATUS                                    status;
    PUDECX_USBCONTROLLER_CONTEXT                controllerContext;
    UDECX_USB_DEVICE_STATE_CHANGE_CALLBACKS     callbacks;
    PUSB_CONFIGURATION_DESCRIPTOR               pComputedConfigDescSet;

    FuncEntry(TRACE_DEVICE);
    
    //
    // Getting descriptors
    //

    PUCHAR UsbDeviceDescriptor = Descriptors.Device.Descriptor;
    PUCHAR UsbConfigDescriptor = Descriptors.Configuration.Descriptor;
    PUCHAR UsbReportDescriptor = Descriptors.Report.Descriptor;

    USHORT UsbDeviceDescriptorLen = Descriptors.Device.Length;
    USHORT UsbConfigDescriptorLen = Descriptors.Configuration.Length;
    USHORT UsbReportDescriptorLen = Descriptors.Report.Length;

    if (UsbDeviceDescriptorLen == 0) {
        status = STATUS_INVALID_PARAMETER;
        return 0;
    }

    //
    // Allocate per-controller private contexts used by other source code modules (I/O,
    // etc.)
    //

    pComputedConfigDescSet = NULL;

    controllerContext = GetUsbControllerContext(WdfDevice);

    //UsbValidateConstants();

    controllerContext->ChildDeviceInit = UdecxUsbDeviceInitAllocate(WdfDevice);

    if (controllerContext->ChildDeviceInit == NULL) {

        status = STATUS_INSUFFICIENT_RESOURCES;
        LogError(TRACE_DEVICE, "Failed to allocate UDECXUSBDEVICE_INIT %!STATUS!", status);
        goto exit;
    }

    //
    // State changed callbacks
    //
    UDECX_USB_DEVICE_CALLBACKS_INIT(&callbacks);

    callbacks.EvtUsbDeviceLinkPowerEntry = UsbDevice_EvtUsbDeviceLinkPowerEntry;
    callbacks.EvtUsbDeviceLinkPowerExit = UsbDevice_EvtUsbDeviceLinkPowerExit;
    callbacks.EvtUsbDeviceSetFunctionSuspendAndWake = UsbDevice_EvtUsbDeviceSetFunctionSuspendAndWake;

    UdecxUsbDeviceInitSetStateChangeCallbacks(controllerContext->ChildDeviceInit, &callbacks);

    //
    // Set required attributes.
    //

    // Checking version of USB in device descriptor and add it to init settings
    if (UsbDeviceDescriptor[3] == 0x02) {
        UdecxUsbDeviceInitSetSpeed(controllerContext->ChildDeviceInit, UdecxUsbHighSpeed);
        controllerContext->DeviceUSBVersion = 0x02;
    }
    else if (UsbDeviceDescriptor[3] == 0x03) {
        UdecxUsbDeviceInitSetSpeed(controllerContext->ChildDeviceInit, UdecxUsbSuperSpeed);
        controllerContext->DeviceUSBVersion = 0x03;
    }
    else {
        LogError(TRACE_DEVICE, "Unexpected USB device version in device descriptor");
        status = STATUS_INVALID_PARAMETER;
        goto exit;
    }

    UdecxUsbDeviceInitSetEndpointsType(controllerContext->ChildDeviceInit, UdecxEndpointTypeSimple);

    //
    // Device descriptor
    //

    status = UdecxUsbDeviceInitAddDescriptor(
        controllerContext->ChildDeviceInit,
        UsbDeviceDescriptor,
        UsbDeviceDescriptorLen);

    if (!NT_SUCCESS(status)) {
        goto exit;
    }

    if (UsbDeviceDescriptor[3] == 0x03) {
        status = UdecxUsbDeviceInitAddDescriptor(
            controllerContext->ChildDeviceInit,
            g_BOS,
            21);

        if (!NT_SUCCESS(status)) {
            goto exit;
        }
    }

    //
    // String descriptors
    //
    /*status = UdecxUsbDeviceInitAddDescriptorWithIndex(controllerContext->ChildDeviceInit,
        (PUCHAR)g_LanguageDescriptor,
        sizeof(g_LanguageDescriptor),
        0);

    if (!NT_SUCCESS(status)) {

        goto exit;
    }

    status = UdecxUsbDeviceInitAddStringDescriptor(controllerContext->ChildDeviceInit,
        &g_ManufacturerStringEnUs,
        g_ManufacturerIndex,
        AMERICAN_ENGLISH);

    if (!NT_SUCCESS(status)) {

        goto exit;
    }

    status = UdecxUsbDeviceInitAddStringDescriptor(controllerContext->ChildDeviceInit,
        &g_ProductStringEnUs,
        g_ProductIndex,
        AMERICAN_ENGLISH);

    if (!NT_SUCCESS(status)) {

        goto exit;
    }*/

    //
    // Remaining init requires lower edge interaction.  Postpone to Usb_ReadDescriptorsAndPlugIn.
    //

    //
    // Compute configuration descriptor dynamically.
    //

    pComputedConfigDescSet = (PUSB_CONFIGURATION_DESCRIPTOR)
        ExAllocatePoolWithTag(NonPagedPoolNx, UsbConfigDescriptorLen, UDECXMBIM_POOL_TAG);

    if (pComputedConfigDescSet == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        LogError(TRACE_DEVICE, "Failed to allocate %d bytes for temporary config descriptor %!STATUS!",
            UsbConfigDescriptorLen, status);
        goto exit;
    }

    RtlCopyMemory(pComputedConfigDescSet,
        UsbConfigDescriptor,
        UsbConfigDescriptorLen);

    status = UdecxUsbDeviceInitAddDescriptor(controllerContext->ChildDeviceInit,
        (PUCHAR)pComputedConfigDescSet,
        UsbConfigDescriptorLen);

    if (!NT_SUCCESS(status)) {
        LogError(TRACE_DEVICE, "Failed to add configuration descriptor %!STATUS!", status);
        goto exit;
    }

    
    // if USB device is HID then this descriptor will be exists

    if (UsbReportDescriptor != NULL) {
        status = UdecxUsbDeviceInitAddDescriptor(controllerContext->ChildDeviceInit,
            UsbReportDescriptor,
            UsbReportDescriptorLen);

        if (!NT_SUCCESS(status)) {
            LogError(TRACE_DEVICE, "Failed to add report descriptor %!STATUS!", status);
            goto exit;
        }
    }
   

exit:

    //
    // On failure in this function (or later but still before creating the UDECXUSBDEVICE),
    // UdecxUsbDeviceInit will be freed by Usb_Destroy.
    //

    //
    // Free temporary allocation always.
    //

    if (pComputedConfigDescSet != NULL) {

        ExFreePoolWithTag(pComputedConfigDescSet, UDECXMBIM_POOL_TAG);
        pComputedConfigDescSet = NULL;
    }

    FuncExit(TRACE_DEVICE, 0);
    return status;
}


NTSTATUS Usb_CreateDeviceAndEndpoints(
    _In_ WDFDEVICE WdfControllerDevice
)
{
    NTSTATUS                          status;
    WDF_OBJECT_ATTRIBUTES             attributes;
    PUSB_CONTEXT                      deviceContext = NULL;
    PUDECX_USBCONTROLLER_CONTEXT      controllerContext;

    controllerContext = GetUsbControllerContext(WdfControllerDevice);
    //
    // Create emulated USB device
    //
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, USB_CONTEXT);

    status = UdecxUsbDeviceCreate(&controllerContext->ChildDeviceInit,
        &attributes,
        &(controllerContext->ChildDevice));

    if (!NT_SUCCESS(status)) {
        LogError(TRACE_DEVICE, "UdecxUsbDeviceCreate failed with status %!STATUS!", status);
        goto exit;
    }


    status = Io_AllocateContext(controllerContext->ChildDevice);
    if (!NT_SUCCESS(status)) {
        LogError(TRACE_DEVICE, "Io_AllocateContext failed with status %!STATUS!", status);
        goto exit;
    }

    deviceContext = GetUsbDeviceContext(controllerContext->ChildDevice);

    // create link to parent
    deviceContext->ControllerDevice = WdfControllerDevice;

    deviceContext->IsAwake = TRUE;  // for some strange reason, it starts out awake!

    //
    // Create static endpoints.
    //
    status = UsbCreateEndpointObj(controllerContext->ChildDevice,
        USB_DEFAULT_ENDPOINT_ADDRESS,
        &(deviceContext->UDEFX2ControlEndpoint));

    if (!NT_SUCCESS(status)) {
        LogError(TRACE_DEVICE, "UsbCreateEndpointObj failed with status %!STATUS!", status);
        goto exit;
    }

    status = UsbCreateEndpointObj(controllerContext->ChildDevice,
        g_BulkOutEndpointAddress,
        &(deviceContext->UDEFX2BulkOutEndpoint));

    if (!NT_SUCCESS(status)) {
        LogError(TRACE_DEVICE, "UsbCreateEndpointObj failed with status %!STATUS!", status);
        goto exit;
    }

    status = UsbCreateEndpointObj(controllerContext->ChildDevice,
        g_BulkInEndpointAddress,
        &(deviceContext->UDEFX2BulkInEndpoint));

    if (!NT_SUCCESS(status)) {
        LogError(TRACE_DEVICE, "UsbCreateEndpointObj failed with status %!STATUS!", status);
        goto exit;
    }

    status = UsbCreateEndpointObj(controllerContext->ChildDevice,
        g_InterruptEndpointAddress,
        &(deviceContext->UDEFX2InterruptInEndpoint));

    if (!NT_SUCCESS(status)) {
        LogError(TRACE_DEVICE, "UsbCreateEndpointObj failed with status %!STATUS!", status);
        goto exit;
    }

exit:
    return status;
}


NTSTATUS
Usb_CreateEndpointsAndPlugIn(
    _In_ WDFDEVICE WdfControllerDevice
)
{
    NTSTATUS                          status;
   
    PUDECX_USBCONTROLLER_CONTEXT      controllerContext;
    UDECX_USB_DEVICE_PLUG_IN_OPTIONS  pluginOptions;

    FuncEntry(TRACE_DEVICE);

    controllerContext = GetUsbControllerContext(WdfControllerDevice);
    
    //
    // Create emulated USB device
    //
    LogInfo(TRACE_DEVICE, "Before create, controller=%p, UsbDevice=%p, UsbDeviceInit=%p",
        WdfControllerDevice, controllerContext->ChildDevice, controllerContext->ChildDeviceInit);

    status = Usb_CreateDeviceAndEndpoints(WdfControllerDevice);
    if (!NT_SUCCESS(status)) {
        LogError(TRACE_DEVICE, "Failed to create usb device %!STATUS!", status);
        goto exit;
    }
    LogInfo(TRACE_DEVICE, "USB device created, controller=%p, UsbDevice=%p, UsbDeviceInit=%p",
        WdfControllerDevice, controllerContext->ChildDevice, controllerContext->ChildDeviceInit);
    //
    // This begins USB communication and prevents us from modifying descriptors and simple endpoints.
    //
    UDECX_USB_DEVICE_PLUG_IN_OPTIONS_INIT(&pluginOptions);
    // Checking version of USB in device and select sutable port
    if (controllerContext->DeviceUSBVersion == 0x02) {
        pluginOptions.Usb20PortNumber = 1;
    }
    else if (controllerContext->DeviceUSBVersion == 0x03) {
        pluginOptions.Usb30PortNumber = 2;
    }
    else {
        LogError(TRACE_DEVICE, "Unexpected USB device version in controller context");
        status = STATUS_INVALID_PARAMETER;
        goto exit;
    }
    status = UdecxUsbDevicePlugIn(controllerContext->ChildDevice, &pluginOptions);
    if (!NT_SUCCESS(status)) {
        LogError(TRACE_DEVICE, "Failed to create plug-in device %!STATUS!", status);
        goto exit;
    }
    LogInfo(TRACE_DEVICE, "Usb_ReadDescriptorsAndPlugIn ends successfully");

exit:
    FuncExit(TRACE_DEVICE, 0);
    return status;
}

NTSTATUS
Usb_Disconnect(
    _In_  WDFDEVICE WdfDevice
)
{
    NTSTATUS status;
    PUDECX_USBCONTROLLER_CONTEXT controllerCtx;
    IO_CONTEXT ioContextCopy;


    controllerCtx = GetUsbControllerContext(WdfDevice);

    Io_StopDeferredProcessing(controllerCtx->ChildDevice, &ioContextCopy);


    status = UdecxUsbDevicePlugOutAndDelete(controllerCtx->ChildDevice);
    // Not deleting the queues that belong to the controller, as this
    // happens only in the last disconnect.  But if we were to connect again,
    // we would need to do that as the queues would leak.

    if (!NT_SUCCESS(status)) {
        LogError(TRACE_DEVICE, "UdecxUsbDevicePlugOutAndDelete failed with %!STATUS!", status);
        goto exit;
    }

    Io_FreeEndpointQueues(&ioContextCopy);

    LogInfo(TRACE_DEVICE, "Usb_Disconnect ends successfully");

exit:

    return status;
}


VOID
Usb_Destroy(
    _In_ WDFDEVICE WdfDevice
)
{
    PUDECX_USBCONTROLLER_CONTEXT pControllerContext;

    pControllerContext = GetUsbControllerContext(WdfDevice);

    //
    // Free device init in case we didn't successfully create the device.
    //
    if (pControllerContext != NULL && pControllerContext->ChildDeviceInit != NULL) {

        UdecxUsbDeviceInitFree(pControllerContext->ChildDeviceInit);
        pControllerContext->ChildDeviceInit = NULL;
    }
    LogError(TRACE_DEVICE, "Usb_Destroy ends successfully");

    return;
}

VOID
Usb_UdecxUsbEndpointEvtReset(
    _In_ UCXCONTROLLER ctrl,
    _In_ UCXENDPOINT ep,
    _In_ WDFREQUEST r
)
{
    UNREFERENCED_PARAMETER(ctrl);
    UNREFERENCED_PARAMETER(ep);
    UNREFERENCED_PARAMETER(r);

    // TODO: endpoint reset. will require a different function prototype
}



NTSTATUS
UsbCreateEndpointObj(
    _In_   UDECXUSBDEVICE    WdfUsbChildDevice,
    _In_   UCHAR             epAddr,
    _Out_  UDECXUSBENDPOINT *pNewEpObjAddr
)
{
    NTSTATUS                      status;
    PUSB_CONTEXT                  pUsbContext;
    WDFQUEUE                      epQueue;
    UDECX_USB_ENDPOINT_CALLBACKS  callbacks;
    PUDECXUSBENDPOINT_INIT        endpointInit;


    pUsbContext = GetUsbDeviceContext(WdfUsbChildDevice);
    endpointInit = NULL;

    status = Io_RetrieveEpQueue(WdfUsbChildDevice, epAddr, &epQueue);

    if (!NT_SUCCESS(status)) {
        goto exit;
    }

    endpointInit = UdecxUsbSimpleEndpointInitAllocate(WdfUsbChildDevice);

    if (endpointInit == NULL) {

        status = STATUS_INSUFFICIENT_RESOURCES;
        LogError(TRACE_DEVICE, "Failed to allocate endpoint init %!STATUS!", status);
        goto exit;
    }

    UdecxUsbEndpointInitSetEndpointAddress(endpointInit, epAddr);

    UDECX_USB_ENDPOINT_CALLBACKS_INIT(&callbacks, UsbEndpointReset);
    UdecxUsbEndpointInitSetCallbacks(endpointInit, &callbacks);

    status = UdecxUsbEndpointCreate(&endpointInit,
        WDF_NO_OBJECT_ATTRIBUTES,
        pNewEpObjAddr );

    if (!NT_SUCCESS(status)) {

        LogError(TRACE_DEVICE, "UdecxUsbEndpointCreate failed for endpoint %x, %!STATUS!", epAddr, status);
        goto exit;
    }

    UdecxUsbEndpointSetWdfIoQueue( *pNewEpObjAddr,  epQueue);

exit:

    if (endpointInit != NULL) {

        NT_ASSERT(!NT_SUCCESS(status));
        UdecxUsbEndpointInitFree(endpointInit);
        endpointInit = NULL;
    }

    return status;
}





VOID
UsbEndpointReset(
    _In_ UDECXUSBENDPOINT UdecxUsbEndpoint,
    _In_ WDFREQUEST     Request
)
{
    UNREFERENCED_PARAMETER(UdecxUsbEndpoint);
    UNREFERENCED_PARAMETER(Request);
}



VOID
UsbDevice_EvtUsbDeviceEndpointsConfigure(
    _In_ UDECXUSBDEVICE                    UdecxUsbDevice,
    _In_ WDFREQUEST                        Request,
    _In_ PUDECX_ENDPOINTS_CONFIGURE_PARAMS Params
)
{
    UNREFERENCED_PARAMETER(UdecxUsbDevice);
    UNREFERENCED_PARAMETER(Params);

    WdfRequestComplete(Request, STATUS_SUCCESS);
}

NTSTATUS
UsbDevice_EvtUsbDeviceLinkPowerEntry(
    _In_ WDFDEVICE       UdecxWdfDevice,
    _In_ UDECXUSBDEVICE    UdecxUsbDevice )
{
    PUSB_CONTEXT pUsbContext;
    UNREFERENCED_PARAMETER(UdecxWdfDevice);

    pUsbContext = GetUsbDeviceContext(UdecxUsbDevice);
    Io_DeviceWokeUp(UdecxUsbDevice);
    pUsbContext->IsAwake = TRUE;
    LogInfo(TRACE_DEVICE, "USB Device power ENTRY");

    return STATUS_SUCCESS;
}

NTSTATUS
UsbDevice_EvtUsbDeviceLinkPowerExit(
    _In_ WDFDEVICE UdecxWdfDevice,
    _In_ UDECXUSBDEVICE UdecxUsbDevice,
    _In_ UDECX_USB_DEVICE_WAKE_SETTING WakeSetting )
{
    PUSB_CONTEXT pUsbContext;
    UNREFERENCED_PARAMETER(UdecxWdfDevice);

    pUsbContext = GetUsbDeviceContext(UdecxUsbDevice);
    pUsbContext->IsAwake = FALSE;

    Io_DeviceSlept(UdecxUsbDevice);

    LogInfo(TRACE_DEVICE, "USB Device power EXIT [wdfDev=%p, usbDev=%p], WakeSetting=%x", UdecxWdfDevice, UdecxUsbDevice, WakeSetting);
    return STATUS_SUCCESS;
}

NTSTATUS
UsbDevice_EvtUsbDeviceSetFunctionSuspendAndWake(
    _In_ WDFDEVICE                        UdecxWdfDevice,
    _In_ UDECXUSBDEVICE                   UdecxUsbDevice,
    _In_ ULONG                            Interface,
    _In_ UDECX_USB_DEVICE_FUNCTION_POWER  FunctionPower
)
{
    UNREFERENCED_PARAMETER(UdecxWdfDevice);
    UNREFERENCED_PARAMETER(UdecxUsbDevice);
    UNREFERENCED_PARAMETER(Interface);
    UNREFERENCED_PARAMETER(FunctionPower);

    // this never gets printed!
    LogInfo(TRACE_DEVICE, "USB Device SuspendAwakeState=%x", FunctionPower );

    return STATUS_SUCCESS;
}





