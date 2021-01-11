#pragma once

#include <windef.h>

// please also see Descriptor/Descriptor.h

enum MODE {
    NONE_MODE,          // value the same as DEFAULT_DESCRIPTOR_SET
    RESERVED_USB_30,    // value the same as KINGSTON_DESCRIPTOR_SET (now 3.0 unsupported)
    SCSI_MODE,          // value the same as FLASH_20_DESCRIPTOR_SET
    HID_MOUSE_MODE,     // value the same as HID_MOUSE_DESCRIPTOR_SET
    HID_KEYBOARD_MODE   // value the same as HID_KEYBOARD_DESCRIPTOR_SET
};


typedef struct _FUZZING_CONTEXT {
    // vid&pid pair we want to fuzz now 
    UCHAR PID[2];
    UCHAR VID[2];
    // current seed
    UINT64 Seed;
    // helpfull field to understand with protocol under USB is used now
    MODE Mode;
    // idex of descriptor set which used at start
    UINT32 DescriptorSetIndx;

} FUZZING_CONTEXT, * PFUZZING_CONTEXT;


