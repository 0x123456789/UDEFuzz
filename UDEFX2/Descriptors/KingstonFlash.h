#pragma once

#include <ntddk.h>
#include <wdf.h>
#include <usb.h>
#include <usbdlib.h>

#include "Common.h"

extern const UCHAR g_KingstonUsbConfigDescriptorSet[];
extern const UCHAR g_KingstonUsbDeviceDescriptor[];


DESCRIPTORS GetKingstonDevDescriptors();

