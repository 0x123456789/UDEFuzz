#include "Flash20.h"

#define ADATA_DEVICE_VENDOR_ID   0x5F, 0x12 // little endian
#define ADATA_DEVICE_PROD_ID     0x2A, 0xC8 // little endian


const UCHAR g_Flash20UsbDeviceDescriptor[18] =
{
    0x12,                            // Descriptor size
    USB_DEVICE_DESCRIPTOR_TYPE,      // Device descriptor type
    0x00, 0x02,                      // USB 2.0
    0x00,                            // Device class (interface-class defined)
    0x00,                            // Device subclass
    0x00,                            // Device protocol
    0x40,                            // Maxpacket size for EP0
    ADATA_DEVICE_VENDOR_ID,          // Vendor ID
    ADATA_DEVICE_PROD_ID,            // Product ID
    0x00,                            // LSB of firmware revision
    0x01,                            // MSB of firmware revision
    0x00,                            // Manufacture string index
    0x00,                            // Product string index
    0x00,                            // Serial number string index
    0x01                             // Number of configurations
};

const UCHAR g_Flash20UsbConfigDescriptorSet[] =
{
    // Configuration Descriptor Type
    0x9,                              // Descriptor Size
    USB_CONFIGURATION_DESCRIPTOR_TYPE, // Configuration Descriptor Type
    0x20, 0x00,                        // Length of this descriptor and all sub descriptors
    0x01,                              // Number of interfaces
    0x01,                              // Configuration number
    0x00,                              // Configuration string index
    0x80,                              // Config characteristics - bus powered
    0xF0,                              // Max power consumption of device (in 2mA unit) : 480 mA

        // Interface  descriptor
        0x9,                                      // Descriptor size
        USB_INTERFACE_DESCRIPTOR_TYPE,             // Interface Association Descriptor Type
        0,                                        // bInterfaceNumber
        0,                                        // bAlternateSetting
        2,                                        // bNumEndpoints
        0x08,                                     // bInterfaceClass
        0x06,                                     // bInterfaceSubClass
        0x50,                                     // bInterfaceProtocol
        0x00,                                     // iInterface

        // Bulk Out Endpoint descriptor
        0x07,                           // Descriptor size
        USB_ENDPOINT_DESCRIPTOR_TYPE,   // bDescriptorType
        g_BulkOutEndpointAddress,       // bEndpointAddress
        USB_ENDPOINT_TYPE_BULK,         // bmAttributes - bulk
        0x00, 0x02,                     // wMaxPacketSize
        0x01,                           // bInterval

        // Bulk IN endpoint descriptor
        0x07,                           // Descriptor size 
        USB_ENDPOINT_DESCRIPTOR_TYPE,   // Descriptor type
        g_BulkInEndpointAddress,        // Endpoint address and description
        USB_ENDPOINT_TYPE_BULK,         // bmAttributes - bulk
        0x00, 0x02,                     // Max packet size
        0x01,                           // Servicing interval for data transfers : NA for bulk
};



DESCRIPTORS GetFlash20DevDescriptors() {

    DESCRIPTORS d;

    d.Device.Descriptor = (PUCHAR)g_Flash20UsbDeviceDescriptor;
    d.Device.Length = sizeof(g_Flash20UsbDeviceDescriptor);

    d.Configuration.Descriptor = (PUCHAR)g_Flash20UsbConfigDescriptorSet;
    d.Configuration.Length = sizeof(g_Flash20UsbConfigDescriptorSet);
    return d;

}
