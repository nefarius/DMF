/*++

    Copyright (c) Microsoft Corporation. All rights reserved.
    Licensed under the MIT license.

Module Name:

    Dmf_DeviceInterfaceTarget.h

Abstract:

    Companion file to Dmf_DeviceInterfaceTarget.c.

Environment:

    Kernel-mode Driver Framework
    User-mode Driver Framework

--*/

#pragma once

// Enum to specify IO Target State.
//
typedef enum
{
    DeviceInterfaceTarget_StateType_Invalid,
    DeviceInterfaceTarget_StateType_Open,
    DeviceInterfaceTarget_StateType_QueryRemove,
    // NOTE: This name is not correct. The correct name is on next line, but old name is kept for backward compatibility.
    //
    DeviceInterfaceTarget_StateType_QueryRemoveCancelled,
    DeviceInterfaceTarget_StateType_RemoveCancel = DeviceInterfaceTarget_StateType_QueryRemoveCancelled,
    // NOTE: This name is not correct. The correct name is on next line, but old name is kept for backward compatibility.
    //
    DeviceInterfaceTarget_StateType_QueryRemoveComplete,
    DeviceInterfaceTarget_StateType_RemoveComplete = DeviceInterfaceTarget_StateType_QueryRemoveComplete,
    DeviceInterfaceTarget_StateType_Close,
    DeviceInterfaceTarget_StateType_Maximum
} DeviceInterfaceTarget_StateType;

// Client Driver callback to notify IoTarget State.
//
typedef
_Function_class_(EVT_DMF_DeviceInterfaceTarget_OnStateChange)
_IRQL_requires_max_(PASSIVE_LEVEL)
_IRQL_requires_same_
VOID
EVT_DMF_DeviceInterfaceTarget_OnStateChange(_In_ DMFMODULE DmfModule,
                                            _In_ DeviceInterfaceTarget_StateType IoTargetState);

// Client Driver callback to notify IoTarget State.
// This version allows Client to veto the open and remove.
//
typedef
_Function_class_(EVT_DMF_DeviceInterfaceTarget_OnStateChangeEx)
_IRQL_requires_max_(PASSIVE_LEVEL)
_IRQL_requires_same_
NTSTATUS
EVT_DMF_DeviceInterfaceTarget_OnStateChangeEx(_In_ DMFMODULE DmfModule,
                                              _In_ DeviceInterfaceTarget_StateType IoTargetState);

// Client Driver callback to notify Interface arrival.
//
typedef
_Function_class_(EVT_DMF_DeviceInterfaceTarget_OnPnpNotification)
_IRQL_requires_max_(PASSIVE_LEVEL)
_IRQL_requires_same_
VOID
EVT_DMF_DeviceInterfaceTarget_OnPnpNotification(_In_ DMFMODULE DmfModule,
                                                _In_ PUNICODE_STRING SymbolicLinkName,
                                                _Out_ BOOLEAN* IoTargetOpen);

#if defined(DMF_USER_MODE)
// Client callback for custom device handle notifications user mode.
//
typedef
_Function_class_(EVT_DMF_DeviceInterfaceTarget_OnPnpCustomNotificationUser)
_IRQL_requires_max_(PASSIVE_LEVEL)
DWORD
EVT_DMF_DeviceInterfaceTarget_OnPnpCustomNotificationUser(_In_ DMFMODULE DmfModule,
                                                          _In_ CM_NOTIFY_ACTION Action,
                                                          _In_reads_bytes_(EventDataSize) PCM_NOTIFY_EVENT_DATA EventData,
                                                          _In_ DWORD EventDataSize);
#endif // defined(DMF_USER_MODE)

// Client uses this structure to configure the Module specific parameters.
//
typedef struct
{
    // Target Device Interface GUID.
    //
    GUID DeviceInterfaceTargetGuid;
    // Open in Read or Write mode.
    //
    ULONG OpenMode;
    // Share Access.
    //
    ULONG ShareAccess;
    // Module Config for Child Module.
    //
    DMF_CONFIG_ContinuousRequestTarget ContinuousRequestTargetModuleConfig;
    // Callback to specify IoTarget State.
    // Use Ex version instead. This version is included for legacy Clients only.
    //
    EVT_DMF_DeviceInterfaceTarget_OnStateChange* EvtDeviceInterfaceTargetOnStateChange;
    // Callback to specify IoTarget State.
    // This version allows Client to veto the open and remove.
    //
    EVT_DMF_DeviceInterfaceTarget_OnStateChangeEx* EvtDeviceInterfaceTargetOnStateChangeEx;
    // Callback to notify Interface arrival.
    //
    EVT_DMF_DeviceInterfaceTarget_OnPnpNotification* EvtDeviceInterfaceTargetOnPnpNotification;
#if defined(DMF_USER_MODE)
    // Callback to notify pnp custom event for user mode.
    //
    EVT_DMF_DeviceInterfaceTarget_OnPnpCustomNotificationUser* EvtPnpCustomNotificationCallbackUser;
#endif // defined(DMF_USER_MODE)
} DMF_CONFIG_DeviceInterfaceTarget;

// This macro declares the following functions:
// DMF_DeviceInterfaceTarget_ATTRIBUTES_INIT()
// DMF_CONFIG_DeviceInterfaceTarget_AND_ATTRIBUTES_INIT()
// DMF_DeviceInterfaceTarget_Create()
//
DECLARE_DMF_MODULE(DeviceInterfaceTarget)

// Module Methods
//

_IRQL_requires_max_(DISPATCH_LEVEL)
_Must_inspect_result_
NTSTATUS
DMF_DeviceInterfaceTarget_BufferPut(
    _In_ DMFMODULE DmfModule,
    _In_ VOID* ClientBuffer
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
_Must_inspect_result_
BOOLEAN
DMF_DeviceInterfaceTarget_Cancel(
    _In_ DMFMODULE DmfModule,
    _In_ RequestTarget_DmfRequestCancel DmfRequestIdCancel
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
_Must_inspect_result_
NTSTATUS
DMF_DeviceInterfaceTarget_Get(
    _In_ DMFMODULE DmfModule,
    _Out_ WDFIOTARGET* IoTarget
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
_Must_inspect_result_
NTSTATUS
DMF_DeviceInterfaceTarget_GuidGet(
    _In_ DMFMODULE DmfModule,
    _Out_ GUID* Guid
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
_Must_inspect_result_
NTSTATUS
DMF_DeviceInterfaceTarget_ReuseCreate(
    _In_ DMFMODULE DmfModule,
    _Out_ RequestTarget_DmfRequestReuse* DmfRequestIdReuse
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
BOOLEAN
DMF_DeviceInterfaceTarget_ReuseDelete(
    _In_ DMFMODULE DmfModule, 
    _In_ RequestTarget_DmfRequestReuse DmfRequestIdReuse
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
_Must_inspect_result_
NTSTATUS
DMF_DeviceInterfaceTarget_ReuseSend(
    _In_ DMFMODULE DmfModule,
    _In_ RequestTarget_DmfRequestReuse DmfRequestIdReuse,
    _In_reads_bytes_opt_(RequestLength) VOID* RequestBuffer,
    _In_ size_t RequestLength,
    _Out_writes_bytes_opt_(ResponseLength) VOID* ResponseBuffer,
    _In_ size_t ResponseLength,
    _In_ ContinuousRequestTarget_RequestType RequestType,
    _In_ ULONG RequestIoctl,
    _In_ ULONG RequestTimeoutMilliseconds,
    _In_opt_ EVT_DMF_ContinuousRequestTarget_SendCompletion* EvtContinuousRequestTargetSingleAsynchronousRequest,
    _In_opt_ VOID* SingleAsynchronousRequestClientContext,
    _Out_opt_ RequestTarget_DmfRequestCancel* DmfRequestIdCancel
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
_Must_inspect_result_
NTSTATUS
DMF_DeviceInterfaceTarget_Send(
    _In_ DMFMODULE DmfModule,
    _In_reads_bytes_opt_(RequestLength) VOID* RequestBuffer,
    _In_ size_t RequestLength,
    _Out_writes_bytes_opt_(ResponseLength) VOID* ResponseBuffer,
    _In_ size_t ResponseLength,
    _In_ ContinuousRequestTarget_RequestType RequestType,
    _In_ ULONG RequestIoctl,
    _In_ ULONG RequestTimeoutMilliseconds,
    _In_opt_ EVT_DMF_ContinuousRequestTarget_SendCompletion* EvtContinuousRequestTargetSingleAsynchronousRequest,
    _In_opt_ VOID* SingleAsynchronousRequestClientContext
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
_Must_inspect_result_
NTSTATUS
DMF_DeviceInterfaceTarget_SendEx(
    _In_ DMFMODULE DmfModule,
    _In_reads_bytes_opt_(RequestLength) VOID* RequestBuffer,
    _In_ size_t RequestLength,
    _Out_writes_bytes_opt_(ResponseLength) VOID* ResponseBuffer,
    _In_ size_t ResponseLength,
    _In_ ContinuousRequestTarget_RequestType RequestType,
    _In_ ULONG RequestIoctl,
    _In_ ULONG RequestTimeoutMilliseconds,
    _In_opt_ EVT_DMF_ContinuousRequestTarget_SendCompletion* EvtContinuousRequestTargetSingleAsynchronousRequest,
    _In_opt_ VOID* SingleAsynchronousRequestClientContext,
    _Out_opt_ RequestTarget_DmfRequestCancel* DmfRequestIdCancel
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
_Must_inspect_result_
NTSTATUS
DMF_DeviceInterfaceTarget_SendSynchronously(
    _In_ DMFMODULE DmfModule,
    _In_reads_bytes_opt_(RequestLength) VOID* RequestBuffer,
    _In_ size_t RequestLength,
    _Out_writes_bytes_opt_(ResponseLength) VOID* ResponseBuffer,
    _In_ size_t ResponseLength,
    _In_ ContinuousRequestTarget_RequestType RequestType,
    _In_ ULONG RequestIoctl,
    _In_ ULONG RequestTimeoutMilliseconds,
    _Out_opt_ size_t* BytesWritten
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
_Must_inspect_result_
NTSTATUS
DMF_DeviceInterfaceTarget_StreamStart(
    _In_ DMFMODULE DmfModule
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
DMF_DeviceInterfaceTarget_StreamStop(
    _In_ DMFMODULE DmfModule
    );

// eof: Dmf_DeviceInterfaceTarget.h
//
