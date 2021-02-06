#pragma once

#include <windef.h>

// please also see Descriptor/Descriptor.h

enum MODE {
    NONE_MODE,          // value the same as DEFAULT_DESCRIPTOR_SET
    RESERVED_USB_30,    // value the same as KINGSTON_DESCRIPTOR_SET
    SCSI_MODE,          // value the same as FLASH_20_DESCRIPTOR_SET
    HID_MOUSE_MODE,     // value the same as HID_MOUSE_DESCRIPTOR_SET
    HID_KEYBOARD_MODE,  // value the same as HID_KEYBOARD_DESCRIPTOR_SET
    HID_JOYSTICK_MODE   // value the same as HID_JOYSTICK_DESCRIPTOR_SET
};


typedef struct _FUZZING_CONTEXT {
    // current seed
    UINT64 Seed;
    // helpfull field to understand with protocol under USB is used now
    MODE Mode;
    // idex of descriptor set which used at start
    UINT32 DescriptorSetIndx;
    // Should fuzzing descriptor also (default false)
    BOOLEAN FuzzDescriptor;
    // Save PID&VID. if FuzzDescriptor is false  SavePV doesn't make sense
    BOOLEAN SavePV;
    // Fuzz only descriptors (need when using S2E)
    BOOLEAN OnlyDesc;

} FUZZING_CONTEXT, * PFUZZING_CONTEXT;


