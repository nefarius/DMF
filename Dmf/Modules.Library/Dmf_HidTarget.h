/*++

    Copyright (c) Microsoft Corporation. All rights reserved.
    Licensed under the MIT license.

Module Name:

    Dmf_HidTarget.h

Abstract:

    Companion file to Dmf_HidTarget.c.

Environment:

    Kernel-mode Driver Framework
    User-mode Driver Framework

--*/

#pragma once

// The maximum number of devices of supported device product Ids that are
// searched in the Module Config.
//
#define DMF_HidTarget_DEVICES_TO_OPEN     8

typedef
_IRQL_requires_same_
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
EVT_DMF_HidTarget_InputReport(_In_ DMFMODULE DmfModule,
                              _In_reads_(BufferLength) UCHAR* Buffer,
                              _In_ ULONG BufferLength);

typedef
_IRQL_requires_same_
_IRQL_requires_max_(DISPATCH_LEVEL)
BOOLEAN
EVT_DMF_HidTarget_DeviceSelectionCallback(_In_ DMFMODULE DmfModule,
                                          _In_ PUNICODE_STRING DevicePath,
                                          _In_ WDFIOTARGET IoTarget,
                                          _In_ PHIDP_PREPARSED_DATA PreparsedHidData,
                                          _In_ HID_COLLECTION_INFORMATION* HidCollectionInformation);

// Client Driver callback function to be called from Asynchronous Send Completion routine.
//
typedef
_Function_class_(EVT_DMF_HidTarget_FeatureGetAsynchronousSendCompletion)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
VOID
EVT_DMF_HidTarget_FeatureGetAsynchronousSendCompletion(_In_ DMFMODULE DmfModule,
                                                       _In_ VOID* ClientRequestContext,
                                                       _In_reads_(OutputBufferBytesRead) VOID* OutputBuffer,
                                                       _In_ size_t OutputBufferBytesRead,
                                                       _In_ NTSTATUS CompletionStatus);

// Client uses this structure to configure the Module specific parameters.
//
typedef struct
{
    // The Vendor Id of the Device to open.
    //
    UINT16 VendorId;
    // List of HID PIDs that are supported by the Client.
    //
    UINT16 PidsOfDevicesToOpen[DMF_HidTarget_DEVICES_TO_OPEN];
    // Number of entries in the above array.
    //
    ULONG PidCount;
    // Information needed to select the device to open.
    //
    UINT16 VendorUsage;
    UINT16 VendorUsagePage;
    UINT8 ExpectedReportId;
    // Open in Read or Write mode.
    //
    ULONG OpenMode;
    // Share Access.
    //
    ULONG ShareAccess;
    // Input report call back.
    //
    EVT_DMF_HidTarget_InputReport* EvtHidInputReport;
    // Skip search for device and use the provided hid device explicitly.
    //
    BOOLEAN SkipHidDeviceEnumerationSearch;
    // Device to open if search is not to be done.
    //
    WDFDEVICE HidTargetToConnect;
    // Allow the client to select the desired device after the module matches 
    // a device, based on the look up criteria the client provided.
    //
    EVT_DMF_HidTarget_DeviceSelectionCallback* EvtHidTargetDeviceSelectionCallback;
    // Number of input report read requests to pend asynchronously.
    //
    ULONG PendedInputReadRequestCount;
} DMF_CONFIG_HidTarget;

// This macro declares the following functions:
// DMF_HidTarget_ATTRIBUTES_INIT()
// DMF_CONFIG_HidTarget_AND_ATTRIBUTES_INIT()
// DMF_HidTarget_Create()
//
DECLARE_DMF_MODULE(HidTarget)

// Module Methods
//

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS
DMF_HidTarget_BufferRead(
    _In_ DMFMODULE DmfModule,
    _Inout_updates_(BufferLength) VOID* Buffer,
    _In_ ULONG BufferLength,
    _In_ ULONG TimeoutMs
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS
DMF_HidTarget_BufferWrite(
    _In_ DMFMODULE DmfModule,
    _In_ VOID* Buffer,
    _In_ ULONG BufferLength,
    _In_ ULONG TimeoutMs
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS
DMF_HidTarget_FeatureGet(
    _In_ DMFMODULE DmfModule,
    _In_ UCHAR FeatureId,
    _Out_writes_(BufferSize) UCHAR* Buffer,
    _In_ ULONG BufferSize,
    _In_ ULONG OffsetOfDataToCopy,
    _In_ ULONG NumberOfBytesToCopy
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS
DMF_HidTarget_FeatureGetAsynchronous(
    _In_ DMFMODULE DmfModule,
    _In_ UCHAR ReportId,
    _In_ EVT_DMF_HidTarget_FeatureGetAsynchronousSendCompletion* EvtContinuousRequestTargetSingleAsynchronousRequest,
    _In_opt_ VOID* HidFeatureGetCompletionCallbackClientContext
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS
DMF_HidTarget_FeatureGetWithTimeout(
    _In_ DMFMODULE DmfModule,
    _In_ UCHAR FeatureId,
    _Out_writes_(BufferSize) UCHAR* Buffer,
    _In_ ULONG BufferSize,
    _In_ ULONG OffsetOfDataToCopy,
    _In_ ULONG NumberOfBytesToCopy,
    _In_ ULONG TimeoutMs
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS
DMF_HidTarget_FeatureSet(
    _In_ DMFMODULE DmfModule,
    _In_ UCHAR FeatureId,
    _In_reads_(BufferSize) UCHAR* Buffer,
    _In_ ULONG BufferSize,
    _In_ ULONG OffsetOfDataToCopy,
    _In_ ULONG NumberOfBytesToCopy
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS
DMF_HidTarget_FeatureSetWithTimeout(
    _In_ DMFMODULE DmfModule,
    _In_ UCHAR FeatureId,
    _In_reads_(BufferSize) UCHAR* Buffer,
    _In_ ULONG BufferSize,
    _In_ ULONG OffsetOfDataToCopy,
    _In_ ULONG NumberOfBytesToCopy,
    _In_ ULONG TimeoutMs
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS
DMF_HidTarget_FeatureSetEx(
    _In_ DMFMODULE DmfModule,
    _In_ UCHAR FeatureId,
    _In_reads_(BufferSize) UCHAR* Buffer,
    _In_ ULONG BufferSize,
    _In_ ULONG OffsetOfDataToCopy,
    _In_ ULONG NumberOfBytesToCopy
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS
DMF_HidTarget_HidCollectionInformationGet(
    _In_ DMFMODULE DmfModule,
    _Out_ HID_COLLECTION_INFORMATION* CollectionInformation
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS
DMF_HidTarget_InputRead(
    _In_ DMFMODULE DmfModule
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
DMF_HidTarget_InputReadCancel(
    _In_ DMFMODULE DmfModule
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS
DMF_HidTarget_InputReadEx(
    _In_ DMFMODULE DmfModule
    );

#if defined(DMF_USER_MODE)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS
DMF_HidTarget_InputReportGet(
    _In_ DMFMODULE DmfModule,
    _In_ WDFMEMORY InputReportMemory,
    _Out_ ULONG* InputReportLength
    );
#endif

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS
DMF_HidTarget_OutputReportSet(
    _In_ DMFMODULE DmfModule,
    _In_reads_(BufferSize) UCHAR* Buffer,
    _In_ ULONG BufferSize,
    _In_ ULONG TimeoutMs
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS
DMF_HidTarget_PreparsedDataGet(
    _In_ DMFMODULE DmfModule,
    _Out_ PHIDP_PREPARSED_DATA* PreparsedData
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS
DMF_HidTarget_ReportCreate(
    _In_ DMFMODULE DmfModule,
    _In_ ULONG ReportType,
    _In_ UCHAR ReportId,
    _Out_ WDFMEMORY* ReportMemory
    );

// eof: Dmf_HidTarget.h
//
