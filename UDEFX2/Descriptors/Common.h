#pragma once

#include <ntddk.h>

#define MAX_DESCRIPTORS_POOL_SIZE 100

typedef struct {
	PUCHAR Descriptor;
	USHORT Length;
} DESCRIPTOR_INFO;

typedef struct {
	DESCRIPTOR_INFO Device;
	DESCRIPTOR_INFO Configuration;
} DESCRIPTORS;

typedef struct {
	// pool for all base descriptors which a stored in driver
	// increase value if your need or make dynamic size
	DESCRIPTORS Descriptors[MAX_DESCRIPTORS_POOL_SIZE];
	// current size of pool
	USHORT Size;


} DESCRIPTOR_POOL;