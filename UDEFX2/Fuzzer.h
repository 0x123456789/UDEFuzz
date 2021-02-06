#pragma once


#pragma pack(push, s2e_gc, 1)
typedef struct _S2E_GET_COVERAGE {
    UINT32 BlocksCovered;
} S2E_GET_COVERAGE, *PS2E_GET_COVERAGE;
#pragma pack(pop, s2e_gc)


NTSTATUS FuzzerInit(UINT64 seed);

UINT32 FuzzerGetCoverage();

void FuzzerDestroy();

void FuzzerMutate(PUCHAR buffer, int len);

void FuzzerMutateDescriptor(PUCHAR buffer, int len, BOOLEAN savePV);

// 
void FuzzerGetOldCaseIfCoverageChanged(PUCHAR buffer, UINT32 len);
void FuzzerSaveCase(PUCHAR buffer, int len);
