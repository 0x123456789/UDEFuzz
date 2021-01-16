
#include <limits.h>
#include <ntddk.h>
#include <wdf.h>

#include "Fuzzer.h"
#include <s2e.h>


#define MAX_BITS_FLIPPING 4
#define MAX_BYTES_FLIPPING 4
#define MAX_MUTATIONS 1


static WDFSPINLOCK sync = NULL;

static UINT64 next = 1;
static UINT32 LastCoverage = 0;


// m = 4294967296 = 2^32
// c = 3
// a = 29943829

UINT64 NextRand() {
	UINT64 rand = 0;
	WdfSpinLockAcquire(sync);
	next = (next * 29943829 + 3) % (4294967296);
	rand = next;
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
	
	int bytesFlip = NextRand() % MAX_BYTES_FLIPPING;

	for (int i = 0; i < bytesFlip; i++) {
		int bytePos = NextRand() % len;
		if (i % 2) {
			buffer[bytePos] = CHAR_MAX;
		}
		else {
			buffer[bytePos] = (UCHAR)CHAR_MIN;
		}
	}
}


void FuzzerMutateDescriptor(PUCHAR buffer, int len, BOOLEAN savePV, BOOLEAN onlyDesc) {
	
	UCHAR VIDPID[4];
	
	if (len == 0) {
		return;
	}

	// check current coverage
	UINT32 coverage = FuzzerGetCoverage();
	if (coverage > LastCoverage) {
		LastCoverage = coverage;

	}

	VIDPID[0] = buffer[8];
	VIDPID[1] = buffer[9];

	VIDPID[2] = buffer[10];
	VIDPID[3] = buffer[11];

	FuzzerMutate(buffer, savePV);

	// restore values
	if (savePV == TRUE) {
		buffer[8] = VIDPID[0];
		buffer[9] = VIDPID[1];
		buffer[10] = VIDPID[2];
		buffer[11] = VIDPID[3];
	}
}

void FuzzerMutate(PUCHAR buffer, int len) {
	// check if buffer is not empty before fuzzing
	if (len == 0) {
		return;
	}

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


UINT32 FuzzerGetCoverage() {

	S2E_GET_COVERAGE cmd;
	// send back only for debugging purpoces
	cmd.BlocksCovered = LastCoverage;

	DbgPrint("Invoking plugin...");
	S2EInvokePlugin("TranslationBlockCoverage", &cmd, sizeof(S2E_GET_COVERAGE));
	DbgPrint("Blocks covered: %d", cmd.BlocksCovered);

	return cmd.BlocksCovered;
}


// now we just init our lock to syncronize random number generation
NTSTATUS FuzzerInit(UINT64 seed) {
	next = seed;

	if (sync == NULL) {
		return WdfSpinLockCreate(WDF_NO_OBJECT_ATTRIBUTES, &sync);
	}
	else {
		return STATUS_SUCCESS;
	}
}

// TODO: call this
void FuzzerDestroy() {
	if (sync != NULL) {
		WdfObjectDelete(sync);
		sync = NULL;
	}
}