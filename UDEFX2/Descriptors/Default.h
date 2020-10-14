#pragma once

#include <ntddk.h>
#include <wdf.h>
#include <usb.h>
#include <usbdlib.h>

#include "Common.h"

// ----- descriptor constants/strings/indexes
#define g_ManufacturerIndex   1
#define g_ProductIndex        2
#define g_BulkOutEndpointAddress 2
#define g_BulkInEndpointAddress    0x84
#define g_InterruptEndpointAddress 0x86


#define UDEFX2_DEVICE_VENDOR_ID  0x9, 0x12 // little endian
#define UDEFX2_DEVICE_PROD_ID    0x87, 0x8 // little endian

extern const UCHAR g_UsbDeviceDescriptor[];
extern const UCHAR g_UsbConfigDescriptorSet[];

// ------------------------------------------------


DESCRIPTORS GetDefaultDevDescriptors();
