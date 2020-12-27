
#include <limits.h>
#include <ntddk.h>
#include <wdf.h>

#include "Fuzzer.h"

#define MAX_BITS_FLIPPING 4
#define MAX_BYTES_FLIPPING 4
#define MAX_MUTATIONS 1


static WDFSPINLOCK sync = NULL;

static UINT64 next = 1;

UINT64 NextRand() {
	UINT64 rand = 0;
	WdfSpinLockAcquire(sync);
	next = next * 1103515245 + 12345;
	rand = (UINT64)(next / 65536) % (32767 + 1);
	WdfSpinLockRelease(sync);
	return rand;
}


// randomly flipping from one to 1
void BitFlipping(PUCHAR buffer, int len) {

	int bitsFlip = NextRand() % MAX_BITS_FLIPPING;

	for (int i = 0; i < bitsFlip; i++) {
		int bytePos = NextRand() % len;
		int bitPos = NextRand() % 8;
		// flipping bit in byte
		buffer[bytePos] ^= (1 << bitPos);
	}
}

void ByteFlipping(PUCHAR buffer, int len) {

	int bytesFlip = NextRand() % MAX_BYTES_FLIPPING;
	for (int i = 0; i < bytesFlip; i++) {
		int bytePos = NextRand() % len;
		// flipping byte
		buffer[bytePos] = ~buffer[bytePos];
	}
}

// TODO: Another known values
void KnownBytes(PUCHAR buffer, int len) {
	
	// int bytesFlip = NextRand() % MAX_BYTES_FLIPPING;

	for (int i = 0; i < len; i++) {
		int bytePos = NextRand() % len;
		if (i % 2) {
			buffer[bytePos] = CHAR_MAX;
		}
		else {
			buffer[bytePos] = (UCHAR)CHAR_MIN;
		}
	}
}

void FuzzerMutate(PUCHAR buffer, int len) {
	for (int i = 0; i < MAX_MUTATIONS; i++) {
		switch (NextRand() % 3)
		{
		case 0:
			BitFlipping(buffer, len);
			break;
		case 1:
			ByteFlipping(buffer, len);
			break;
		case 2:
			KnownBytes(buffer, len);
			break;
		default:
			break;
		}
	}
}

// now we just init our lock to syncronize random number generation
NTSTATUS FuzzerInit(UINT64 seed) {
	next = seed;
	return WdfSpinLockCreate(WDF_NO_OBJECT_ATTRIBUTES, &sync);
}

// TODO: call this
void FuzzerDestroy() {
	if (sync != NULL) {
		WdfObjectDelete(sync);
	}
}