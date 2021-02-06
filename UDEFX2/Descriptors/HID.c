#include <ntddk.h>
#include <wdf.h>
#include <usb.h>
#include <usbdlib.h>

#include "HID.h"

// see: https://eleccelerator.com/tutorial-about-usb-hid-report-descriptors/

// Mouse HID 
#define ELAN_DEVICE_VENDOR_ID  0xF3, 0x04 // little endian
#define ELAN_DEVICE_PRODUCT_ID 0x35, 0x02 // little endian

// Keyboard HID
#define A4_TECH_VENDOR_ID 0xDA, 0x09  // little endian
#define A4_TECH_PRODUCT_ID 0x60, 0x02 // little endian

// Joystick
#define ATMEL_VENDOR_ID 0xEB, 0x03   // little endian
#define ATMEL_PRODUCT_ID 0x43, 0x20 // little endian



const UCHAR g_HIDJoystickDeviceDescriptor[18] =
{
    0x12,                            // Descriptor size
    USB_DEVICE_DESCRIPTOR_TYPE,      // Device descriptor type
    0x10, 0x01,                      // USB 2.0
    0x00,                            // Device class (interface-class defined)
    0x00,                            // Device subclass
    0x00,                            // Device protocol
    0x10,                            // Maxpacket size for EP0
    ATMEL_VENDOR_ID,                 // Vendor ID
    ATMEL_PRODUCT_ID,                // Product ID
    0x00,                            // LSB of firmware revision
    0x00,                            // MSB of firmware revision
    0x00,                            // Manufacture string index
    0x00,                            // Product string index
    0x00,                            // Serial number string index
    0x01                             // Number of configurations
};


const UCHAR g_HIDJoystickUsbConfigDescriptorSet[] =
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
        0x00,                                     // bInterfaceSubClass 
        0x00,                                     // bInterfaceProtocol (None)
        0x00,                                     // iInterface

        // HID Descriptor
        0x09,       // Descriptor size
        0x21,       // bDescriptorType (HID)
        0x10, 0x01, // HID Class Spec Version
        0x00,       // bCountryCode
        0x01,       // bNumDescriptors
        0x22,       // bDescriptorType (Report)
        0x30, 0x00, // wDescriptorLength

        // Interrupt IN endpoint descriptor
        0x07,                           // Descriptor size 
        USB_ENDPOINT_DESCRIPTOR_TYPE,   // Descriptor type
        g_InterruptEndpointAddress,     // Endpoint address and description
        USB_ENDPOINT_TYPE_INTERRUPT,    // bmAttributes - interrupt
        0x08, 0x00,                     // Max packet size = 8 bytes
        0x0A                            // Servicing interval for interrupt (10 ms/1 frame)
};

const UCHAR g_HIDJoystickUsbReportDescriptor[] =
{
    0x05, 0x01,                    // USAGE_PAGE (Generic Desktop)
    0x09, 0x05,                    // USAGE (Game Pad)
    0xa1, 0x01,                    // COLLECTION (Application)
    0xa1, 0x00,                    //   COLLECTION (Physical)
    0x85, 0x04,                    //     REPORT_ID (4)
    0x05, 0x09,                    //     USAGE_PAGE (Button)
    0x19, 0x01,                    //     USAGE_MINIMUM (Button 1)
    0x29, 0x10,                    //     USAGE_MAXIMUM (Button 16)
    0x15, 0x00,                    //     LOGICAL_MINIMUM (0)
    0x25, 0x01,                    //     LOGICAL_MAXIMUM (1)
    0x95, 0x10,                    //     REPORT_COUNT (16)
    0x75, 0x01,                    //     REPORT_SIZE (1)
    0x81, 0x02,                    //     INPUT (Data,Var,Abs)
    0x05, 0x01,                    //     USAGE_PAGE (Generic Desktop)
    0x09, 0x30,                    //     USAGE (X)
    0x09, 0x31,                    //     USAGE (Y)
    0x09, 0x32,                    //     USAGE (Z)
    0x09, 0x33,                    //     USAGE (Rx)
    0x15, 0x81,                    //     LOGICAL_MINIMUM (-127)
    0x25, 0x7f,                    //     LOGICAL_MAXIMUM (127)
    0x75, 0x08,                    //     REPORT_SIZE (8)
    0x95, 0x04,                    //     REPORT_COUNT (4)
    0x81, 0x02,                    //     INPUT (Data,Var,Abs)
    0xc0,                          //     END_COLLECTION
    0xc0                           // END_COLLECTION
};

// =================================================================================================================

const UCHAR g_HIDKeyboardDeviceDescriptor[18] =
{
    0x12,                            // Descriptor size
    USB_DEVICE_DESCRIPTOR_TYPE,      // Device descriptor type
    0x10, 0x01,                      // USB 2.0
    0x00,                            // Device class (interface-class defined)
    0x00,                            // Device subclass
    0x00,                            // Device protocol
    0x08,                            // Maxpacket size for EP0
    A4_TECH_VENDOR_ID,               // Vendor ID
    A4_TECH_PRODUCT_ID,              // Product ID
    0x50,                            // LSB of firmware revision
    0x02,                            // MSB of firmware revision
    0x00,                            // Manufacture string index
    0x00,                            // Product string index
    0x00,                            // Serial number string index
    0x01                             // Number of configurations
};

const UCHAR g_HIDKeyboardUsbConfigDescriptorSet[] =
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
        0x01,                                     // bInterfaceProtocol (Keyboard)
        0x00,                                     // iInterface

        // HID Descriptor
        0x09,       // Descriptor size
        0x21,       // bDescriptorType (HID)
        0x10, 0x01, // HID Class Spec Version
        0x00,       // bCountryCode
        0x01,       // bNumDescriptors
        0x22,       // bDescriptorType (Report)
        0x3E, 0x00, // wDescriptorLength

        // Interrupt IN endpoint descriptor
        0x07,                           // Descriptor size 
        USB_ENDPOINT_DESCRIPTOR_TYPE,   // Descriptor type
        g_InterruptEndpointAddress,     // Endpoint address and description
        USB_ENDPOINT_TYPE_INTERRUPT,    // bmAttributes - interrupt
        0x08, 0x00,                     // Max packet size = 4 bytes
        0x0A                            // Servicing interval for interrupt (10 ms/1 frame)
};


// Interface 0 HID Report Descriptor Keyboard

const UCHAR g_HIDKeyboardUsbReportDescriptor[] =
{
    0x05, 0x01,     // Usage Page(Generic Desktop) 
    0x09, 0x06,     // Usage(Keyboard) 
    0xA1, 0x01,     // Collection(Application) A1 01
    0x05, 0x07,     // Usage Page(Keyboard / Keypad) 05 07
    0x19, 0xE0,     // Usage Minimum(Keyboard Left Control) 
    0x29, 0xE7,     // Usage Maximum(Keyboard Right GUI) 
    0x15, 0x00,     // Logical Minimum(0) 15 00
    0x25, 0x01,     // Logical Maximum(1) 25 01
    0x75, 0x01,     // Report Size(1) 75 01
    0x95, 0x08,     // Report Count(8) 95 08
    0x81, 0x02,     // Input(Data, Var, Abs, NWrp, Lin, Pref, NNul, Bit) 81 02
    0x95, 0x01,     // Report Count(1) 95 01
    0x75, 0x08,     // Report Size(8) 75 08
    0x81, 0x01,     // Input(Cnst, Ary, Abs) 81 01
    0x95, 0x03,     // Report Count(3) 95 03
    0x75, 0x01,     // Report Size(1) 75 01
    0x05, 0x08,     // Usage Page(LEDs) 05 08
    0x19, 0x01,     // Usage Minimum(Num Lock) 19 01
    0x29, 0x03,     // Usage Maximum(Scroll Lock) 29 03
    0x91, 0x02,     // Output(Data, Var, Abs, NWrp, Lin, Pref, NNul, NVol, Bit) 91 02
    0x95, 0x05,     // Report Count(5) 95 05
    0x75, 0x01,     // Report Size(1) 75 01
    0x91, 0x01,     // Output(Cnst, Ary, Abs, NWrp, Lin, Pref, NNul, NVol, Bit) 91 01
    0x95, 0x06,     // Report Count(6) 95 06
    0x75, 0x08,     // Report Size(8) 75 08
    0x26, 0xFF, 00, // Logical Maximum(255) 26 FF 00
    0x05, 0x07,     // Usage Page(Keyboard / Keypad) 05 07
    0x19, 0x00,     // Usage Minimum(Undefined) 19 00
    0x29, 0x91,     // Usage Maximum(Keyboard LANG2) 29 91
    0x81, 0x00,     // Input(Data, Ary, Abs) 81 00
    0xC0,           // End Collection C0
};

// ================================================================================================

const UCHAR g_HIDMouseDeviceDescriptor[18] =
{
    0x12,                            // Descriptor size
    USB_DEVICE_DESCRIPTOR_TYPE,      // Device descriptor type
    0x10, 0x01,                      // USB 2.0
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


const UCHAR g_HIDMouseUsbConfigDescriptorSet[] =
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

const UCHAR g_HIDMouseUsbReportDescriptor[] =
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

// ================================================================================================

DESCRIPTORS GetHIDMouseDevDescriptors() {

    DESCRIPTORS d;

    d.Device.Descriptor = (PUCHAR)g_HIDMouseDeviceDescriptor;
    d.Device.Length = sizeof(g_HIDMouseDeviceDescriptor);

    d.Configuration.Descriptor = (PUCHAR)g_HIDMouseUsbConfigDescriptorSet;
    d.Configuration.Length = sizeof(g_HIDMouseUsbConfigDescriptorSet);

    d.Report.Descriptor = (PUCHAR)g_HIDMouseUsbReportDescriptor;
    d.Report.Length = sizeof(g_HIDMouseUsbReportDescriptor);

    return d;
}

DESCRIPTORS GetHIDKeyboardDevDescriptors() {

    DESCRIPTORS d;

    d.Device.Descriptor = (PUCHAR)g_HIDKeyboardDeviceDescriptor;
    d.Device.Length = sizeof(g_HIDKeyboardDeviceDescriptor);

    d.Configuration.Descriptor = (PUCHAR)g_HIDKeyboardUsbConfigDescriptorSet;
    d.Configuration.Length = sizeof(g_HIDKeyboardUsbConfigDescriptorSet);

    d.Report.Descriptor = (PUCHAR)g_HIDKeyboardUsbReportDescriptor;
    d.Report.Length = sizeof(g_HIDKeyboardUsbReportDescriptor);

    return d;
}

DESCRIPTORS GetHIDJoystickDevDescriptors() {

    DESCRIPTORS d;

    d.Device.Descriptor = (PUCHAR)g_HIDJoystickDeviceDescriptor;
    d.Device.Length = sizeof(g_HIDJoystickDeviceDescriptor);

    d.Configuration.Descriptor = (PUCHAR)g_HIDJoystickUsbConfigDescriptorSet;
    d.Configuration.Length = sizeof(g_HIDJoystickUsbConfigDescriptorSet);

    d.Report.Descriptor = (PUCHAR)g_HIDJoystickUsbReportDescriptor;
    d.Report.Length = sizeof(g_HIDJoystickUsbReportDescriptor);

    return d;
}