#include "Descriptor.h"

DESCRIPTOR_POOL GetDescriptorPool() {
	
	DESCRIPTOR_POOL p;

	p.Descriptors[DEFAULT_DESCRIPTOR_SET] = GetDefaultDevDescriptors();
	p.Descriptors[KINGSTON_DESCRIPTOR_SET] = GetKingstonDevDescriptors();
	p.Descriptors[FLASH_20_DESCRIPTOR_SET] = GetFlash20DevDescriptors();
	p.Descriptors[HID_MOUSE_DESCRIPTOR_SET] = GetHIDMouseDevDescriptors();
	p.Descriptors[HID_KEYBOARD_DESCRIPTOR_SET] = GetHIDKeyboardDevDescriptors();
	p.Descriptors[HID_JOYSTICK_DESCRIPTOR_SET] = GetHIDJoystickDevDescriptors();
	p.Size = 6;

	return p;
}