
#include "Driver.h"
#include "Device.h"
#include "usbdevice.h"

#include "USBHID.h"
#include "USBHID.tmh"



MOUSE_INPUT_REPORT DefaultMouseInputReport() {
    MOUSE_INPUT_REPORT r;
    r.Buttons = 0x00;
    r.Wheel = 0x00;
    r.X = 0x01;
    r.Y = 0x01;
    return r;
}

void HIDHandleBulkInResponse(
    _Inout_ PUCHAR ResponseBuffer,
    _In_    ULONG  ResponseBufferLen,
    _Out_   PULONG ResponseLen
)
{
    // if don't we have enouth space to save our response
    if (ResponseBufferLen < sizeof(MOUSE_INPUT_REPORT)) {
        LogError(TRACE_DEVICE, "[ERROR] Can't copy response to buffer: ResponseBufferLen < sizeof(MOUSE_INPUT_REPORT)");
        (*ResponseLen) = 0;
        return;
    }

    MOUSE_INPUT_REPORT report = DefaultMouseInputReport();
    memcpy(ResponseBuffer, &report, sizeof(MOUSE_INPUT_REPORT));
    (*ResponseLen) = sizeof(MOUSE_INPUT_REPORT);
    return;
}