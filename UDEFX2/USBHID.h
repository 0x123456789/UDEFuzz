#pragma once

#include <ntddk.h>
#include <wdf.h>

#include "trace.h"

#pragma pack(push, mir, 1)
typedef struct _MOUSE_INPUT_REPORT {
	UINT8 Buttons;
	UINT8 X;
	UINT8 Y;
	UINT8 Wheel;
} MOUSE_INPUT_REPORT, * PMOUSE_INPUT_REPORT;
#pragma pack(pop, mir)

#pragma pack(push, kir, 1)
typedef struct _KEYBOARD_INPUT_REPORT {
	UINT8 ReportId;
	UINT8 Modifier;
	UINT8 KeyCodes[6];
} KEYBOARD_INPUT_REPORT, * PKEYBOARD_INPUT_REPORT;
#pragma pack(pop, kir)


void HIDHandleMouseResponse(
    _Inout_ PUCHAR ResponseBuffer,
    _In_ ULONG ResponseBufferLen,
    _Out_ PULONG ResponseLen
);


void HIDHandleKeyboardResponse(
	_Inout_ PUCHAR ResponseBuffer,
	_In_ ULONG ResponseBufferLen,
	_Out_ PULONG ResponseLen
);