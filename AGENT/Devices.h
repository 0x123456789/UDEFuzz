#pragma once

#include "..\UDEFX2\public.h"
#include "..\UDEFX2\Fuzzing.h"
#include "..\UDEFX2\AgentControl.h"

PZZWSTR GetPresentDeviceList(LPGUID interfaceGuid);

HANDLE RunUSBDeviceCheckerThread(UINT8 deviceCode);

USHORT ToDeviceCode(WCHAR number);

void UnplugUSBDevice();

void GenerateInterrupt(LPGUID interfaceGuid);

void AutoFuzzMode(BOOLEAN fuzzDesc, BOOLEAN savePV, BOOLEAN onlyDesc);

void DescriptorFuzzMode(UINT8 deviceCode);