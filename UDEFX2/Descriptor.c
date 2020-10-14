#include "Descriptor.h"

DESCRIPTOR_POOL GetDescriptorPool() {
	
	DESCRIPTOR_POOL p;

	p.Descriptors[DEFAULT_DESCRIPTOR_SET] = GetDefaultDevDescriptors();
	p.Descriptors[KINGSTON_DESCRIPTOR_SET] = GetKingstonDevDescriptors();
	p.Size = 2;
	return p;
}