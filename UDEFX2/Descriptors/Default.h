#pragma once

#include "Common.h"

// ----- descriptor constants/strings/indexes
#define g_ManufacturerIndex   1
#define g_ProductIndex        2



#define UDEFX2_DEVICE_VENDOR_ID  0x9, 0x12 // little endian
#define UDEFX2_DEVICE_PROD_ID    0x87, 0x8 // little endian

extern const UCHAR g_UsbDeviceDescriptor[];
extern const UCHAR g_UsbConfigDescriptorSet[];

// ------------------------------------------------


DESCRIPTORS GetDefaultDevDescriptors();
