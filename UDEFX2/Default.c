#include <ntddk.h>
#include <wdf.h>
#include <usb.h>
#include <usbdlib.h>

#include "Descriptors/Default.h"


// START------------------descriptor------------------------------ -


DECLARE_CONST_UNICODE_STRING(g_ManufacturerStringEnUs, L"Microsoft");
DECLARE_CONST_UNICODE_STRING(g_ProductStringEnUs, L"UDE Client");


const USHORT AMERICAN_ENGLISH = 0x0409;

const UCHAR g_LanguageDescriptor[] = {
    4,                          // bLength
    USB_STRING_DESCRIPTOR_TYPE, // bDescriptorType
    0x09, 0x04,                 // bString
};





const UCHAR g_UsbDeviceDescriptor[18] =
{
    0x12,                            // Descriptor size
    USB_DEVICE_DESCRIPTOR_TYPE,      // Device descriptor type
    0x00, 0x02,                      // USB 2.0
    0x00,                            // Device class (interface-class defined)
    0x00,                            // Device subclass
    0x00,                            // Device protocol
    0x40,                            // Maxpacket size for EP0
    UDEFX2_DEVICE_VENDOR_ID,         // Vendor ID
    UDEFX2_DEVICE_PROD_ID,           // Product ID
    0x00,                            // LSB of firmware revision
    0x01,                            // MSB of firmware revision
    0x01,                            // Manufacture string index [!]
    0x02,                            // Product string index     [!]
    0x00,                            // Serial number string index
    0x01                             // Number of configurations
};

const UCHAR g_UsbConfigDescriptorSet[] =
{
    // Configuration Descriptor Type
    0x9,                              // Descriptor Size
    USB_CONFIGURATION_DESCRIPTOR_TYPE, // Configuration Descriptor Type
    0x27, 0x00,                        // Length of this descriptor and all sub descriptors
    0x1,                               // Number of interfaces
    0x01,                              // Configuration number
    0x00,                              // Configuration string index
    0xA0,                              // Config characteristics - bus powered
    0x32,                              // Max power consumption of device (in 2mA unit) : 0 ma

        // Interface  descriptor
        0x9,                                      // Descriptor size
        USB_INTERFACE_DESCRIPTOR_TYPE,             // Interface Association Descriptor Type
        0,                                        // bInterfaceNumber
        0,                                        // bAlternateSetting
        3,                                        // bNumEndpoints
        0xFF,                                     // bInterfaceClass
        0x00,                                     // bInterfaceSubClass
        0x00,                                     // bInterfaceProtocol
        0x00,                                     // iInterface

        // Bulk Out Endpoint descriptor
        0x07,                           // Descriptor size
        USB_ENDPOINT_DESCRIPTOR_TYPE,   // bDescriptorType
        g_BulkOutEndpointAddress,       // bEndpointAddress
        USB_ENDPOINT_TYPE_BULK,         // bmAttributes - bulk
        0x00, 0x2,                      // wMaxPacketSize
        0x00,                           // bInterval

        // Bulk IN endpoint descriptor
        0x07,                           // Descriptor size 
        USB_ENDPOINT_DESCRIPTOR_TYPE,   // Descriptor type
        g_BulkInEndpointAddress,        // Endpoint address and description
        USB_ENDPOINT_TYPE_BULK,         // bmAttributes - bulk
        0x00, 0x02,                     // Max packet size
        0x00,                           // Servicing interval for data transfers : NA for bulk

        // Interrupt IN endpoint descriptor
        0x07,                           // Descriptor size 
        USB_ENDPOINT_DESCRIPTOR_TYPE,   // Descriptor type
        g_InterruptEndpointAddress,     // Endpoint address and description
        USB_ENDPOINT_TYPE_INTERRUPT,    // bmAttributes - interrupt
        0x40, 0x0,                      // Max packet size = 64
        0x01                            // Servicing interval for interrupt (1ms/1 frame)
};


DESCRIPTORS GetDefaultDevDescriptors() {
    DESCRIPTORS d;

    d.Device.Descriptor = (PUCHAR)g_UsbDeviceDescriptor;
    d.Device.Length = sizeof(g_UsbDeviceDescriptor);

    d.Configuration.Descriptor = (PUCHAR)g_UsbConfigDescriptorSet;
    d.Configuration.Length = sizeof(g_UsbConfigDescriptorSet);

    d.Report.Descriptor = NULL;
    d.Report.Length = 0;

    return d;
}