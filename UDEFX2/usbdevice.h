/*++
Copyright (c) Microsoft Corporation

Module Name:

misc.h

Abstract:


--*/

#pragma once

#include <ntddk.h>
#include <wdf.h>
#include <usb.h>
#include <wdfusb.h>
#include <usbdlib.h>
#include <ude/1.0/UdeCx.h>
#include <initguid.h>
#include <usbioctl.h>

#include "Descriptor.h"
#include "trace.h"



// device context
typedef struct _USB_CONTEXT {
    WDFDEVICE             ControllerDevice;
    UDECXUSBENDPOINT      UDEFX2ControlEndpoint;
	UDECXUSBENDPOINT      UDEFX2BulkOutEndpoint;
    UDECXUSBENDPOINT      UDEFX2BulkInEndpoint;
    UDECXUSBENDPOINT      UDEFX2InterruptInEndpoint;
    BOOLEAN               IsAwake;
} USB_CONTEXT, *PUSB_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(USB_CONTEXT, GetUsbDeviceContext);





EXTERN_C_START


NTSTATUS
Usb_Initialize(
    _In_ WDFDEVICE WdfDevice,
    _In_ DESCRIPTORS Descriptors
);

NTSTATUS Usb_CreateDeviceAndEndpoints(
    _In_ WDFDEVICE WdfControllerDevice
);

NTSTATUS
Usb_CreateEndpointsAndPlugIn(
	_In_ WDFDEVICE WdfControllerDevice
);

NTSTATUS
Usb_Disconnect(
	_In_ WDFDEVICE WdfDevice
);

VOID
Usb_Destroy(
	_In_ WDFDEVICE WdfDevice
);

//
// Private functions
//
NTSTATUS
UsbCreateEndpointObj(
	_In_   UDECXUSBDEVICE    WdfUsbChildDevice,
    _In_   UCHAR             epAddr,
    _Out_  UDECXUSBENDPOINT *pNewEpObjAddr
);


EVT_UDECX_USB_DEVICE_ENDPOINTS_CONFIGURE              UsbDevice_EvtUsbDeviceEndpointsConfigure;
EVT_UDECX_USB_DEVICE_D0_ENTRY                         UsbDevice_EvtUsbDeviceLinkPowerEntry;
EVT_UDECX_USB_DEVICE_D0_EXIT                          UsbDevice_EvtUsbDeviceLinkPowerExit;
EVT_UDECX_USB_DEVICE_SET_FUNCTION_SUSPEND_AND_WAKE    UsbDevice_EvtUsbDeviceSetFunctionSuspendAndWake;
EVT_UDECX_USB_ENDPOINT_RESET                          UsbEndpointReset;


EXTERN_C_END


