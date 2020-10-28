#include "HID.h"

// see: https://eleccelerator.com/tutorial-about-usb-hid-report-descriptors/

#define ELAN_DEVICE_VENDOR_ID  0xF3, 0x04 // little endian
#define ELAN_DEVICE_PRODUCT_ID 0x35, 0x02 // little endian



const UCHAR g_HIDUsbDeviceDescriptor[18] =
{
    0x12,                            // Descriptor size
    USB_DEVICE_DESCRIPTOR_TYPE,      // Device descriptor type
    0x00, 0x02,                      // USB 2.0
    0x00,                            // Device class (interface-class defined)
    0x00,                            // Device subclass
    0x00,                            // Device protocol
    0x08,                            // Maxpacket size for EP0
    ELAN_DEVICE_VENDOR_ID,           // Vendor ID
    ELAN_DEVICE_PRODUCT_ID,          // Product ID
    0x58,                            // LSB of firmware revision
    0x24,                            // MSB of firmware revision
    0x00,                            // Manufacture string index
    0x00,                            // Product string index
    0x00,                            // Serial number string index
    0x01                             // Number of configurations
};


const UCHAR g_HIDUsbConfigDescriptorSet[] =
{
    // Configuration Descriptor Type
    0x9,                               // Descriptor Size
    USB_CONFIGURATION_DESCRIPTOR_TYPE, // Configuration Descriptor Type
    0x22, 0x00,                        // Length of this descriptor and all sub descriptors
    0x01,                              // Number of interfaces
    0x01,                              // Configuration number
    0x00,                              // Configuration string index
    0xA0,                              // Config characteristics - Bus Powered, Remote Wakeup
    0x32,                              // Max power consumption of device (in 2mA unit) : 100 mA

        // Interface  descriptor
        0x9,                                      // Descriptor size
        USB_INTERFACE_DESCRIPTOR_TYPE,            // Interface Association Descriptor Type
        0,                                        // bInterfaceNumber
        0,                                        // bAlternateSetting
        1,                                        // bNumEndpoints
        0x03,                                     // bInterfaceClass (HID)
        0x01,                                     // bInterfaceSubClass (Boot Interface)
        0x02,                                     // bInterfaceProtocol (Mouse)
        0x00,                                     // iInterface

        // HID Descriptor
        0x09,       // Descriptor size
        0x21,       // bDescriptorType (HID)
        0x11, 0x01, // HID Class Spec Version
        0x00,       // bCountryCode
        0x01,       // bNumDescriptors
        0x22,       // bDescriptorType (Report)
        0x3E, 0x00, // wDescriptorLength

        // Interrupt IN endpoint descriptor
        0x07,                           // Descriptor size 
        USB_ENDPOINT_DESCRIPTOR_TYPE,   // Descriptor type
        g_InterruptEndpointAddress,     // Endpoint address and description
        USB_ENDPOINT_TYPE_INTERRUPT,    // bmAttributes - interrupt
        0x04, 0x00,                     // Max packet size = 4 bytes
        0x0A                            // Servicing interval for interrupt (10 ms/1 frame)
};


// Interface 0 HID Report Descriptor Mouse

const UCHAR g_HIDUsbReportDescriptor[] =
{

    0x05, 0x01, // Usage Page (Generic Desktop)
    0x09, 0x02, // Usage(Mouse)
    0xA1, 0x01, // Collection(Application)
    0x09, 0x01, // Usage(Pointer)
    0xA1, 0x00, // Collection(Physical)
    0x05, 0x09, // Usage Page(Button)
    0x19, 0x01, // Usage Minimum(Button 1)
    0x29, 0x03, // Usage Maximum(Button 3)
    0x15, 0x00, // Logical Minimum(0)
    0x25, 0x01, // Logical Maximum(1)
    0x95, 0x03, // Report Count(3)
    0x75, 0x01, // Report Size(1)
    0x81, 0x02, // Input(Data, Var, Abs, NWrp, Lin, Pref, NNul, Bit)
    0x95, 0x05, // Report Count(5)
    0x75, 0x01, // Report Size(1)
    0x81, 0x03, // Input(Cnst, Var, Abs, NWrp, Lin, Pref, NNul, Bit)
    0x05, 0x01, // Usage Page(Generic Desktop)
    0x09, 0x30, // Usage(X)
    0x09, 0x31, // Usage(Y)
    0x15, 0x81, // Logical Minimum(-127)
    0x25, 0x7F, // Logical Maximum(127)
    0x75, 0x08, // Report Size(8)
    0x95, 0x02, // Report Count(2)
    0x81, 0x06, // Input(Data, Var, Rel, NWrp, Lin, Pref, NNul, Bit)
    0x09, 0x38, // Usage(Wheel)
    0x15, 0x81, // Logical Minimum(-127)
    0x25, 0x7F, // Logical Maximum(127)
    0x75, 0x08, // Report Size(8)
    0x95, 0x01, // Report Count(1)
    0x81, 0x06, // Input(Data, Var, Rel, NWrp, Lin, Pref, NNul, Bit)
    0xC0,       // End Collection
    0xC0        // End Collection
};



DESCRIPTORS GetHIDDevDescriptors() {

    DESCRIPTORS d;

    d.Device.Descriptor = (PUCHAR)g_HIDUsbDeviceDescriptor;
    d.Device.Length = sizeof(g_HIDUsbDeviceDescriptor);

    d.Configuration.Descriptor = (PUCHAR)g_HIDUsbConfigDescriptorSet;
    d.Configuration.Length = sizeof(g_HIDUsbConfigDescriptorSet);

    d.Report.Descriptor = (PUCHAR)g_HIDUsbReportDescriptor;
    d.Report.Length = sizeof(g_HIDUsbReportDescriptor);

    return d;
}