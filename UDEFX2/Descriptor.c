#include "Descriptor.h"

DESCRIPTOR_POOL GetDescriptorPool() {
	
	DESCRIPTOR_POOL p;

	p.Descriptors[DEFAULT_DESCRIPTOR_SET] = GetDefaultDevDescriptors();
	p.Descriptors[KINGSTON_DESCRIPTOR_SET] = GetKingstonDevDescriptors();
	p.Descriptors[FLASH_20_DESCRIPTOR_SET] = GetFlash20DevDescriptors();
	p.Size = 3;
	return p;
}