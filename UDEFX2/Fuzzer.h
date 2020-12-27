#pragma once

NTSTATUS FuzzerInit(UINT64 seed);

void FuzzerDestroy();

void FuzzerMutate(PUCHAR buffer, int len);