#include "Descriptors/KingstonFlash.h"

#define KINGSTON_DEVICE_VENDOR_ID  0x51, 0x09 // little endian
#define KINGSTON_DEVICE_PROD_ID    0x66, 0x16 // little endian

// ----- descriptor constants/strings/indexes

// These values NOT used for now.
// For simplicity Manufacture string index, Product string index and Serial number string index is set to zero
// which means no string descriptor is presented.
// To add string descriptors call UdecxUsbDeviceInitAddDescriptorWithIndex (with zero index) and UdecxUsbDeviceInitAddStringDescriptor

#define g_KingstonManufacturerIndex   1
#define g_KingstonProductIndex        2

DECLARE_CONST_UNICODE_STRING(g_KingstonManufacturerStringEnUs, L"Kingston");
DECLARE_CONST_UNICODE_STRING(g_KingstonProductStringEnUs, L"DataTraveler 3.0");

// ----------------------------------------------------------------------------------------------------------------------------------

const UCHAR g_KingstonUsbDeviceDescriptor[18] =
{
    0x12,                            // Descriptor size
    USB_DEVICE_DESCRIPTOR_TYPE,      // Device descriptor type
    0x00, 0x03,                      // USB 3.1
    0x00,                            // Device class (interface-class defined)
    0x00,                            // Device subclass
    0x00,                            // Device protocol
    0x09,                            // Maxpacket size for EP0
    KINGSTON_DEVICE_VENDOR_ID,       // Vendor ID
    KINGSTON_DEVICE_PROD_ID,         // Product ID
    0x01,                            // LSB of firmware revision
    0x00,                            // MSB of firmware revision
    0x00,                            // Manufacture string index
    0x00,                            // Product string index
    0x00,                            // Serial number string index
    0x01                             // Number of configurations
};

const UCHAR g_KingstonUsbConfigDescriptorSet[] =
{
    // Configuration Descriptor Type
    0x9,                               // Descriptor Size
    USB_CONFIGURATION_DESCRIPTOR_TYPE, // Configuration Descriptor Type
    0x2C, 0x00,                        // Length of this descriptor and all sub descriptors
    0x01,                              // Number of interfaces
    0x01,                              // Configuration number
    0x00,                              // Configuration string index
    0x80,                              // Config characteristics - bus powered
    0x25,                              // Max power consumption of device (296 mA)

        // Interface  descriptor
        0x9,                                      // Descriptor size
        USB_INTERFACE_DESCRIPTOR_TYPE,            // Interface Association Descriptor Type
        0,                                        // bInterfaceNumber
        0,                                        // bAlternateSetting
        2,                                        // bNumEndpoints
        0x08,                                     // bInterfaceClass
        0x06,                                     // bInterfaceSubClass
        0x50,                                     // bInterfaceProtocol
        0x00,                                     // iInterface

        // Bulk IN Endpoint descriptor
        0x07,                           // Descriptor size
        USB_ENDPOINT_DESCRIPTOR_TYPE,   // bDescriptorType
        0x81,                           // bEndpointAddress
        USB_ENDPOINT_TYPE_BULK,         // bmAttributes - bulk
        0x00, 0x04,                     // wMaxPacketSize
        0x00,                           // bInterval

        // SuperSpeed Endpoint Companion Descriptor
        0x06,                                                   // bLength
        USB_SUPERSPEED_ENDPOINT_COMPANION_DESCRIPTOR_TYPE,      // bDescriptorType
        0x0F,                                                   // bMaxBurst
        USB_ENDPOINT_TYPE_CONTROL,                              // bmAttributes (in usbview 0x00, but I don't sure that it's for control, but USB_ENDPOINT_TYPE_CONTROL == 0x00)
        0x00, 0x00,                                             // wBytesPerInterval


        // Bulk OUT Endpoint descriptor
        0x07,                           // Descriptor size
        USB_ENDPOINT_DESCRIPTOR_TYPE,   // bDescriptorType
        0x02,                           // bEndpointAddress
        USB_ENDPOINT_TYPE_BULK,         // bmAttributes - bulk
        0x00, 0x04,                     // wMaxPacketSize
        0x00,                           // bInterval

        // SuperSpeed Endpoint Companion Descriptor
        0x06,                                                   // bLength
        USB_SUPERSPEED_ENDPOINT_COMPANION_DESCRIPTOR_TYPE,      // bDescriptorType
        0x0F,                                                   // bMaxBurst
        USB_ENDPOINT_TYPE_CONTROL,                              // bmAttributes (in usbview 0x00, but I don't sure that it's for control, but USB_ENDPOINT_TYPE_CONTROL == 0x00)
        0x00, 0x00,

        //// BOS Descriptor
        //0x05,                       // bLength
        //USB_BOS_DESCRIPTOR_TYPE,    // bDescriptorType
        //0x16, 0x00,                 // wTotalLength
        //0x02,                       // bNumDeviceCaps

        //// USB 2.0 extension descriptor
        //0x07,                                   // bLength
        //USB_DEVICE_CAPABILITY_DESCRIPTOR_TYPE,  // bDescriptorType
        //USB_DEVICE_CAPABILITY_USB20_EXTENSION,  // bDevCapabilityType
        //0x06, 0x00, 0x00, 0x00,                 // bmAttributes

        //// SuperSpeed USB Device Capability Descriptor
        //0x0A,                                   // bLength
        //USB_DEVICE_CAPABILITY_DESCRIPTOR_TYPE,  // bDescriptorType
        //USB_DEVICE_CAPABILITY_SUPERSPEED_USB,   // bDevCapabilityType
        //0x00,                                   // bmAttributes
        //0x0E,                                   // wSpeedsSupported
        //0x02,                                   // bFunctionalitySupport (lower speed - high speed)
        //0x0A,                                   // wU1DevExitLat (less than 10 micro-seconds)
        //0xFF, 0x07,                             // wU2DevExitLat (less than 2047 micro-seconds)
};


DESCRIPTORS GetKingstonDevDescriptors() {

    DESCRIPTORS d;

    d.Device.Descriptor = (PUCHAR)g_KingstonUsbDeviceDescriptor;
    d.Device.Length = sizeof(g_KingstonUsbDeviceDescriptor);
    

    d.Configuration.Descriptor = (PUCHAR)g_KingstonUsbConfigDescriptorSet;
    d.Configuration.Length = sizeof(g_KingstonUsbConfigDescriptorSet);
    
    d.Report.Descriptor = NULL;
    d.Report.Length = 0;

    return d;
}

