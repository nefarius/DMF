/*++

    Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    Dmf_Tests_DeviceInterfaceTarget.c

Abstract:

    Functional tests for Dmf_DeviceInterfaceTarget Module.

Environment:

    Kernel-mode Driver Framework
    User-mode Driver Framework

--*/

// DMF and this Module's Library specific definitions.
//
#include "DmfModule.h"
#include "DmfModules.Library.Tests.h"
#include "DmfModules.Library.Tests.Trace.h"

#if defined(DMF_INCLUDE_TMH)
#include "Dmf_Tests_DeviceInterfaceTarget.tmh"
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////////
// Module Private Enumerations and Structures
///////////////////////////////////////////////////////////////////////////////////////////////////////
//

#include <initguid.h>

// Define TEST_SIMPLE to only execute simple tests that test cancel.
//
#define NO_TEST_CANCEL_NORMAL
#define NO_TEST_SIMPLE

// Choose one of these or none of these.
//
#define NO_TEST_SYNCHRONOUS_ONLY
#define NO_TEST_ASYNCHRONOUS_ONLY
#define NO_TEST_ASYNCHRONOUSCANCEL_ONLY
#define NO_TEST_DYNAMIC_ONLY
#define NO_TEST_ASYNCHRONOUSREUSE_ONLY

// {5F4F3758-D11E-4684-B5AD-FE6D19D82A51}
//
DEFINE_GUID(GUID_NO_DEVICE, 0x5f4f3758, 0xd11e, 0x4684, 0xb5, 0xad, 0xfe, 0x6d, 0x19, 0xd8, 0x2a, 0x51);

#define THREAD_COUNT                            (1)
#define MAXIMUM_SLEEP_TIME_MS                   (8000)
// Keep synchronous maximum time short to make driver disable faster.
//
#if !defined(TEST_SIMPLE)
#define MAXIMUM_SLEEP_TIME_SYNCHRONOUS_MS       (1000)
#else
#define MAXIMUM_SLEEP_TIME_SYNCHRONOUS_MS       (1000)
#endif

// Asynchronous minimum sleep time to make sure request can be canceled.
//
#define MINIMUM_SLEEP_TIME_MS                   (4000)

// Random timeouts for IOCTLs sent.
//
#define TIMEOUT_FAST_MS             100
#define TIMEOUT_SLOW_MS             5000
#define TIMEOUT_TRAFFIC_DELAY_MS    1000
#define TIMEOUT_CANCEL_MS           15
#define TIMEOUT_CANCEL_LONG_MS      250

#define USE_STREAMING
#if defined(USE_STREAMING)
    #define NUMBER_OF_CONTINUOUS_REQUESTS   32
#else
    #define NUMBER_OF_CONTINUOUS_REQUESTS   0
#endif

typedef enum _TEST_ACTION
{
    TEST_ACTION_SYNCHRONOUS,
    TEST_ACTION_ASYNCHRONOUS,
    TEST_ACTION_ASYNCHRONOUSCANCEL,
    TEST_ACTION_ASYNCHRONOUSREUSE,
#if defined(DMF_KERNEL_MODE)
    TEST_ACTION_DIRECTINTERFACE,
#endif // defined(DMF_KERNEL_MODE)
    TEST_ACTION_DYNAMIC,
    TEST_ACTION_COUNT,
    TEST_ACTION_MINIUM = TEST_ACTION_SYNCHRONOUS,
    TEST_ACTION_MAXIMUM = (TEST_ACTION_COUNT - 1)
} TEST_ACTION;

// This option causes the veto of the remote target removal.
//
#define NO_TEST_VETO

// This option prevents Dynamic Modules from being created.
//
#define NO_DYNAMIC_DISABLE

///////////////////////////////////////////////////////////////////////////////////////////////////////
// Module Private Context
///////////////////////////////////////////////////////////////////////////////////////////////////////
//

typedef struct _DMF_CONTEXT_Tests_DeviceInterfaceTarget
{
    // Modules under test.
    //
    DMFMODULE DmfModuleDeviceInterfaceTargetDispatchInput;
    DMFMODULE DmfModuleDeviceInterfaceTargetPassiveInput;
    DMFMODULE DmfModuleDeviceInterfaceTargetPassiveOutput;
    DMFMODULE DmfModuleDeviceInterfaceTargetPassiveNonContinuous;
    DMFMODULE DmfModuleDeviceInterfaceTargetDispatchNonContinuous;
    // Source of buffers sent asynchronously.
    //
    DMFMODULE DmfModuleBufferPool;
    // Work threads that perform actions on the DeviceInterfaceTarget Module.
    // +1 makes it easy to set THREAD_COUNT = 0 for test purposes.
    //
    DMFMODULE DmfModuleThreadDispatchInput[THREAD_COUNT + 1];
    DMFMODULE DmfModuleThreadPassiveInput[THREAD_COUNT + 1];
    DMFMODULE DmfModuleThreadPassiveOutput[THREAD_COUNT + 1];
    DMFMODULE DmfModuleThreadPassiveNonContinuous[THREAD_COUNT + 1];
    DMFMODULE DmfModuleThreadDispatchNonContinuous[THREAD_COUNT + 1];
    // Use alertable sleep to allow driver to unload faster.
    //
    DMFMODULE DmfModuleAlertableSleepDispatchInput[THREAD_COUNT + 1];
    DMFMODULE DmfModuleAlertableSleepPassiveInput[THREAD_COUNT + 1];
    DMFMODULE DmfModuleAlertableSleepPassiveOutput[THREAD_COUNT + 1];
    DMFMODULE DmfModuleAlertableSleepPassiveNonContinuous[THREAD_COUNT + 1];
    DMFMODULE DmfModuleAlertableSleepDispatchNonContinuous[THREAD_COUNT + 1];

#if defined(DMF_KERNEL_MODE)
    // Direct interface via IRP_MN_QUERY_INTERFACE.
    //
    Tests_IoctlHandler_INTERFACE_STANDARD DirectInterface;
#endif // defined(DMF_KERNEL_MODE)
} DMF_CONTEXT_Tests_DeviceInterfaceTarget;

// This macro declares the following function:
// DMF_CONTEXT_GET()
//
DMF_MODULE_DECLARE_CONTEXT(Tests_DeviceInterfaceTarget)

// This Module has no Config.
//
DMF_MODULE_DECLARE_NO_CONFIG(Tests_DeviceInterfaceTarget)

// Memory Pool Tag.
//
#define MemoryTag 'TiDT'

///////////////////////////////////////////////////////////////////////////////////////////////////////
// DMF Module Support Code
///////////////////////////////////////////////////////////////////////////////////////////////////////
//

// Stores the Module threadIndex so that the corresponding alterable sleep
// can be retrieved inside the thread's callback.
//
typedef struct
{
    DMFMODULE DmfModuleAlertableSleep;
} Tests_DeviceInterfaceTarget_THREAD_INDEX_CONTEXT;
WDF_DECLARE_CONTEXT_TYPE(Tests_DeviceInterfaceTarget_THREAD_INDEX_CONTEXT);

#if defined(DMF_KERNEL_MODE)

// NOTE: This call can fail but there is no way to return that status. So it is necessary to check
//       for the InterfaceDereference pointer before using it in the PreClose callback.
//
NTSTATUS
Tests_DeviceInterfaceTarget_QueryInterface(
    _In_ DMFMODULE DmfModuleInterfaceTarget
    )
{
    DMFMODULE parentDmfModule;
    DMF_CONTEXT_Tests_DeviceInterfaceTarget* moduleContext;
    WDFIOTARGET ioTarget;
    NTSTATUS ntStatus;

    parentDmfModule = DMF_ParentModuleGet(DmfModuleInterfaceTarget);
    moduleContext = DMF_CONTEXT_GET(parentDmfModule);

    // Acquire reference to target Module before calling its Method get WDFIOTARGET
    // handle and hold it while it is in use.
    // 
    ntStatus = DMF_ModuleReference(DmfModuleInterfaceTarget);
    if (!NT_SUCCESS(ntStatus))
    {
        goto ExitNoDereference;
    }

    // Try to acquire the handle.
    //
    ntStatus = DMF_DeviceInterfaceTarget_Get(DmfModuleInterfaceTarget,
                                             &ioTarget);
    if (!NT_SUCCESS(ntStatus))
    {
        goto Exit;
    }

    // Use the underlying WDFIOTARGET to query for the direct interface.
    // Keep reference while doing so.
    // NOTE: This function acquires a reference to the interface using InterfaceReference.
    //       This interface remains available until a corresponding InterfaceDereference 
    //       is called.
    // Pass something unique to InterfaceSpecificData (i.e. the last argument) so that the driver exposing
    // the interface can distinguish to caller.
    //
    ntStatus = WdfIoTargetQueryForInterface(ioTarget,
                                            &GUID_Tests_IoctlHandler_INTERFACE_STANDARD,
                                            (INTERFACE*)&moduleContext->DirectInterface,
                                            sizeof(Tests_IoctlHandler_INTERFACE_STANDARD),
                                            1,
                                            ioTarget);
    DmfAssert(NT_SUCCESS(ntStatus));

Exit:

    // Release reference to Module which allowed safe use of ioTarget.
    //
    DMF_ModuleDereference(DmfModuleInterfaceTarget);

ExitNoDereference:

    // If successful, call into the interface.
    // 
    // NOTE: It is not necessary to call DMF_ModuleReference() to call the interface because it has its own
    //       reference counter that was acquired by WdfIoTargetQueryForInterface().
    //
    if (NT_SUCCESS(ntStatus))
    {
        UCHAR interfaceValue;

        // Call the direct callback functions to get the property or
        // configuration information of the device.
        //
        (*moduleContext->DirectInterface.InterfaceValueGet)(moduleContext->DirectInterface.InterfaceHeader.Context,
                                                            &interfaceValue);
        interfaceValue++;
        (*moduleContext->DirectInterface.InterfaceValueSet)(moduleContext->DirectInterface.InterfaceHeader.Context,
                                                            interfaceValue);
    }

    return ntStatus;
}

VOID
Tests_DeviceInterfaceTarget_DirectInterfaceTest(
    _In_ DMFMODULE DmfModuleInterfaceTarget
    )
{
    DMFMODULE parentDmfModule;
    DMF_CONTEXT_Tests_DeviceInterfaceTarget* moduleContext;

    parentDmfModule = DMF_ParentModuleGet(DmfModuleInterfaceTarget);
    moduleContext = DMF_CONTEXT_GET(parentDmfModule);

    NTSTATUS ntStatus = DMF_ModuleReference(DmfModuleInterfaceTarget);
    if (NT_SUCCESS(ntStatus))
    {
        UCHAR interfaceValue;

        (*moduleContext->DirectInterface.InterfaceValueGet)(moduleContext->DirectInterface.InterfaceHeader.Context,
                                                            &interfaceValue);

        DMF_ModuleDereference(DmfModuleInterfaceTarget);
        TraceEvents(TRACE_LEVEL_INFORMATION, DMF_TRACE, "DI: InterfaceValue=%d", interfaceValue);
    }
}

VOID
Tests_DeviceInterfaceTarget_DirectInterfaceDecrement(
    DMFMODULE DmfModule
    )
{
    DMF_CONTEXT_Tests_DeviceInterfaceTarget* moduleContext;

    moduleContext = DMF_CONTEXT_GET(DmfModule);

    // NOTE: Call the query interface may have failed in the PostOpen callback. Regardless, PreClose always
    //       happens so this pointer must be checked prior to use in PreClose.
    //
    if (moduleContext->DirectInterface.InterfaceHeader.InterfaceDereference != NULL)
    {
        moduleContext->DirectInterface.InterfaceHeader.InterfaceDereference(moduleContext->DirectInterface.InterfaceHeader.Context);
    }
}

#endif // defined(DMF_KERNEL_MODE)

_IRQL_requires_max_(PASSIVE_LEVEL)
_IRQL_requires_same_
NTSTATUS
Tests_DeviceInterfaceTarget_OnStateChange(
    _In_ DMFMODULE DeviceInterfaceTarget,
    _In_ DeviceInterfaceTarget_StateType IoTargetState
    )
/*++

Routine Description:

    Called when a given Target arrives or is being removed.
    This code tests veto during query remove.

Arguments:

    DeviceInterfaceTarget - DMF_DeviceInterfaceTarget.
    Target - The given target.
    IoTargetState - Indicates how the given Target state is changing.

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS ntStatus;

    ntStatus = STATUS_SUCCESS;

    switch (IoTargetState)
    {
        case DeviceInterfaceTarget_StateType_Open:
        {
            TraceEvents(TRACE_LEVEL_INFORMATION, DMF_TRACE, "Tests_DeviceInterfaceTarget_OnStateChange: DeviceInterfaceTarget=0x%p IoTargetState=DeviceInterfaceTarget_StateType_Open", DeviceInterfaceTarget);
            break;
        }
        case DeviceInterfaceTarget_StateType_QueryRemove:
        {
            TraceEvents(TRACE_LEVEL_INFORMATION, DMF_TRACE, "Tests_DeviceInterfaceTarget_OnStateChange: DeviceInterfaceTarget=0x%p IoTargetState=DeviceInterfaceTarget_StateType_QueryRemove", DeviceInterfaceTarget);
#if defined(TEST_VETO)
            ntStatus = STATUS_UNSUCCESSFUL;
            TraceEvents(TRACE_LEVEL_INFORMATION, DMF_TRACE, "Tests_DeviceInterfaceTarget_OnStateChange: VETO DeviceInterfaceTarget=0x%p IoTargetState=DeviceInterfaceTarget_StateType_QueryRemove", DeviceInterfaceTarget);
#endif
            break;
        }
        case DeviceInterfaceTarget_StateType_RemoveCancel:
        {
            TraceEvents(TRACE_LEVEL_INFORMATION, DMF_TRACE, "Tests_DeviceInterfaceTarget_OnStateChange: DeviceInterfaceTarget=0x%p IoTargetState=DeviceInterfaceTarget_StateType_RemoveCancel", DeviceInterfaceTarget);
            break;
        }
        case DeviceInterfaceTarget_StateType_RemoveComplete:
        {
            TraceEvents(TRACE_LEVEL_INFORMATION, DMF_TRACE, "Tests_DeviceInterfaceTarget_OnStateChange: DeviceInterfaceTarget=0x%p IoTargetState=DeviceInterfaceTarget_StateType_RemoveComplete", DeviceInterfaceTarget);
            break;
        }
        case DeviceInterfaceTarget_StateType_Close:
        {
            TraceEvents(TRACE_LEVEL_INFORMATION, DMF_TRACE, "Tests_DeviceInterfaceTarget_OnStateChange: DeviceInterfaceTarget=0x%p IoTargetState=DeviceInterfaceTarget_StateType_Close", DeviceInterfaceTarget);
            break;
        }
        default:
        {
            DmfAssert(FALSE);
            break;
        }
    }
    
    return ntStatus;
}

_Function_class_(EVT_DMF_ContinuousRequestTarget_BufferInput)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
VOID
Tests_DeviceInterfaceTarget_BufferInput(
    _In_ DMFMODULE DmfModule,
    _Out_writes_(*InputBufferSize) VOID* InputBuffer,
    _Out_ size_t* InputBufferSize,
    _In_ VOID* ClientBuferContextInput
    )
{
    Tests_IoctlHandler_Sleep sleepIoctlBuffer;
    GUID guid;

    UNREFERENCED_PARAMETER(InputBufferSize);
    UNREFERENCED_PARAMETER(ClientBuferContextInput);

    DMF_DeviceInterfaceTarget_GuidGet(DmfModule,
                                      &guid);

    sleepIoctlBuffer.TimeToSleepMilliseconds = TestsUtility_GenerateRandomNumber(0,
                                                                                 MAXIMUM_SLEEP_TIME_MS);
    RtlCopyMemory(InputBuffer,
                  &sleepIoctlBuffer,
                  sizeof(sleepIoctlBuffer));
    *InputBufferSize = sizeof(sleepIoctlBuffer);

    TraceEvents(TRACE_LEVEL_INFORMATION, DMF_TRACE, "DIBI:dmfModule=0x%p sleepIoctlBuffer->TimeToSleepMilliseconds=%ld", DmfModule, sleepIoctlBuffer.TimeToSleepMilliseconds);
}

_Function_class_(EVT_DMF_ContinuousRequestTarget_BufferOutput)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
ContinuousRequestTarget_BufferDisposition
Tests_DeviceInterfaceTarget_BufferOutput(
    _In_ DMFMODULE DmfModule,
    _In_reads_(OutputBufferSize) VOID* OutputBuffer,
    _In_ size_t OutputBufferSize,
    _In_ VOID* ClientBufferContextOutput,
    _In_ NTSTATUS CompletionStatus
    )
{
    GUID guid;
    ContinuousRequestTarget_BufferDisposition bufferDisposition;

    UNREFERENCED_PARAMETER(ClientBufferContextOutput);
    UNREFERENCED_PARAMETER(DmfModule);
    UNREFERENCED_PARAMETER(CompletionStatus);

    bufferDisposition = ContinuousRequestTarget_BufferDisposition_ContinuousRequestTargetAndContinueStreaming;

    DMF_DeviceInterfaceTarget_GuidGet(DmfModule,
                                      &guid);

    DmfAssert(NT_SUCCESS(CompletionStatus) ||
              (CompletionStatus == STATUS_CANCELLED) ||
              (CompletionStatus == STATUS_INVALID_DEVICE_STATE));
    if (OutputBufferSize != sizeof(DWORD) &&
        CompletionStatus != STATUS_INVALID_DEVICE_STATE)
    {
        // Request can be completed with InformationSize of 0 by framework.
        // This can happen during suspend/resume of machine.
        //
    }
    if (NT_SUCCESS(CompletionStatus))
    {
        if (*((DWORD*)OutputBuffer) != 0)
        {
            DmfAssert(FALSE);
        }
    }

    // If IoTarget is closing but streaming has not been stopped, ContinuousRequestTarget will continue to send the request
    // back to the closing IoTarget if we don't stop streaming here.
    //
    if (! NT_SUCCESS(CompletionStatus))
    {
        TraceEvents(TRACE_LEVEL_ERROR, DMF_TRACE, "Completed Request CompletionStatus=%!STATUS!", CompletionStatus);
        bufferDisposition = ContinuousRequestTarget_BufferDisposition_ContinuousRequestTargetAndStopStreaming;
    }

    return bufferDisposition;
}

#pragma code_seg("PAGE")
_IRQL_requires_max_(PASSIVE_LEVEL)
static
void
Tests_DeviceInterfaceTarget_ThreadAction_Synchronous(
    _In_ DMFMODULE DmfModule,
    _In_ DMFMODULE DmfModuleAlertableSleep,
    _In_ DMFMODULE DmfModuleDeviceInterfaceTarget
    )
{
    NTSTATUS ntStatus;
    Tests_IoctlHandler_Sleep sleepIoctlBuffer;
    size_t bytesWritten;
    ULONG timeoutMs;

    UNREFERENCED_PARAMETER(DmfModule);
    UNREFERENCED_PARAMETER(DmfModuleAlertableSleep);

    PAGED_CODE();

    RtlZeroMemory(&sleepIoctlBuffer,
                  sizeof(sleepIoctlBuffer));

#if defined(TEST_CANCEL_NORMAL)
    // Test buffer never completes, always cancels.
    //
    sleepIoctlBuffer.TimeToSleepMilliseconds = TestsUtility_GenerateRandomNumber(MINIMUM_SLEEP_TIME_MS, 
                                                                                 MAXIMUM_SLEEP_TIME_SYNCHRONOUS_MS);
    timeoutMs = TIMEOUT_CANCEL_MS;
#else
    sleepIoctlBuffer.TimeToSleepMilliseconds = TestsUtility_GenerateRandomNumber(0, 
                                                                                 MAXIMUM_SLEEP_TIME_SYNCHRONOUS_MS);
    timeoutMs = 0;
#endif
    bytesWritten = 0;
    TraceEvents(TRACE_LEVEL_INFORMATION, DMF_TRACE, "DI01:dmfModule=0x%p sleepIoctlBuffer->TimeToSleepMilliseconds=%ld", DmfModuleDeviceInterfaceTarget, sleepIoctlBuffer.TimeToSleepMilliseconds);
    ntStatus = DMF_DeviceInterfaceTarget_SendSynchronously(DmfModuleDeviceInterfaceTarget,
                                                           &sleepIoctlBuffer,
                                                           sizeof(Tests_IoctlHandler_Sleep),
                                                           &sleepIoctlBuffer,
                                                           sizeof(Tests_IoctlHandler_Sleep),
                                                           ContinuousRequestTarget_RequestType_Ioctl,
                                                           IOCTL_Tests_IoctlHandler_SLEEP,
                                                           timeoutMs,
                                                           &bytesWritten);
    TraceEvents(TRACE_LEVEL_INFORMATION, DMF_TRACE, "DI01:dmfModule=0x%p sleepIoctlBuffer->TimeToSleepMilliseconds=%ld SyncComplete", DmfModuleDeviceInterfaceTarget, sleepIoctlBuffer.TimeToSleepMilliseconds);
    DmfAssert(NT_SUCCESS(ntStatus) ||
              (ntStatus == STATUS_CANCELLED) ||
              (ntStatus == STATUS_INVALID_DEVICE_STATE) ||
              (ntStatus == STATUS_DELETE_PENDING));
    // TODO: Get time and compare with send time.
    //

#if defined(TEST_CANCEL_NORMAL)
    // Test buffer always completes, no timeout.
    //
    sleepIoctlBuffer.TimeToSleepMilliseconds = TestsUtility_GenerateRandomNumber(0, 
                                                                                 TIMEOUT_CANCEL_LONG_MS);
    timeoutMs = MAXIMUM_SLEEP_TIME_SYNCHRONOUS_MS;
#else
    sleepIoctlBuffer.TimeToSleepMilliseconds = TestsUtility_GenerateRandomNumber(0, 
                                                                                 MAXIMUM_SLEEP_TIME_SYNCHRONOUS_MS);
    timeoutMs = 0;
#endif
    bytesWritten = 0;
    TraceEvents(TRACE_LEVEL_INFORMATION, DMF_TRACE, "DI02:dmfModule=0x%p sleepIoctlBuffer->TimeToSleepMilliseconds=%ld", DmfModuleDeviceInterfaceTarget, sleepIoctlBuffer.TimeToSleepMilliseconds);
    ntStatus = DMF_DeviceInterfaceTarget_SendSynchronously(DmfModuleDeviceInterfaceTarget,
                                                           &sleepIoctlBuffer,
                                                           sizeof(Tests_IoctlHandler_Sleep),
                                                           &sleepIoctlBuffer,
                                                           sizeof(Tests_IoctlHandler_Sleep),
                                                           ContinuousRequestTarget_RequestType_Ioctl,
                                                           IOCTL_Tests_IoctlHandler_SLEEP,
                                                           timeoutMs,
                                                           &bytesWritten);
    TraceEvents(TRACE_LEVEL_INFORMATION, DMF_TRACE, "DI02:dmfModule=0x%p sleepIoctlBuffer->TimeToSleepMilliseconds=%ld SyncComplete", DmfModuleDeviceInterfaceTarget, sleepIoctlBuffer.TimeToSleepMilliseconds);
    DmfAssert(NT_SUCCESS(ntStatus) ||
              (ntStatus == STATUS_CANCELLED) ||
              (ntStatus == STATUS_INVALID_DEVICE_STATE) ||
              (ntStatus == STATUS_DELETE_PENDING));
    // TODO: Get time and compare with send time.
    //

#if defined(DMF_KERNEL_MODE)
    // Execute the QueryInterface functions.
    //
    Tests_DeviceInterfaceTarget_DirectInterfaceTest(DmfModuleDeviceInterfaceTarget);
#endif
}
#pragma code_seg()

_Function_class_(EVT_DMF_ContinuousRequestTarget_SendCompletion)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
VOID
Tests_DeviceInterfaceTarget_SendCompletion(
    _In_ DMFMODULE DmfModuleDeviceInterfaceTarget,
    _In_ VOID* ClientRequestContext,
    _In_reads_(InputBufferBytesWritten) VOID* InputBuffer,
    _In_ size_t InputBufferBytesWritten,
    _In_reads_(OutputBufferBytesRead) VOID* OutputBuffer,
    _In_ size_t OutputBufferBytesRead,
    _In_ NTSTATUS CompletionStatus
    )
{
    DMF_CONTEXT_Tests_DeviceInterfaceTarget* moduleContext;
    Tests_IoctlHandler_Sleep* sleepIoctlBuffer;

    // TODO: Get time and compare with send time.
    //
    UNREFERENCED_PARAMETER(InputBuffer);
    UNREFERENCED_PARAMETER(InputBufferBytesWritten);
    UNREFERENCED_PARAMETER(OutputBuffer);
    UNREFERENCED_PARAMETER(OutputBufferBytesRead);

    DMFMODULE dmfModule = DMF_ParentModuleGet(DmfModuleDeviceInterfaceTarget);
    moduleContext = DMF_CONTEXT_GET(dmfModule);

    sleepIoctlBuffer = (Tests_IoctlHandler_Sleep*)ClientRequestContext;

    TraceEvents(TRACE_LEVEL_INFORMATION, DMF_TRACE, "DI: RECEIVE sleepIoctlBuffer->TimeToSleepMilliseconds=%d sleepIoctlBuffer=0x%p CompletionStatus=%!STATUS!", 
                sleepIoctlBuffer->TimeToSleepMilliseconds,
                sleepIoctlBuffer,
                CompletionStatus);

    if (sleepIoctlBuffer->ReuseEvent != NULL)
    {
        DMF_Portable_EventSet(sleepIoctlBuffer->ReuseEvent);
    }

    DMF_BufferPool_Put(moduleContext->DmfModuleBufferPool,
                       (VOID*)sleepIoctlBuffer);
}

_Function_class_(EVT_DMF_ContinuousRequestTarget_SendCompletion)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
VOID
Tests_DeviceInterfaceTarget_SendCompletionMustBeCancelled(
    _In_ DMFMODULE DmfModuleDeviceInterfaceTarget,
    _In_ VOID* ClientRequestContext,
    _In_reads_(InputBufferBytesWritten) VOID* InputBuffer,
    _In_ size_t InputBufferBytesWritten,
    _In_reads_(OutputBufferBytesRead) VOID* OutputBuffer,
    _In_ size_t OutputBufferBytesRead,
    _In_ NTSTATUS CompletionStatus
    )
{
    DMF_CONTEXT_Tests_DeviceInterfaceTarget* moduleContext;
    Tests_IoctlHandler_Sleep* sleepIoctlBuffer;

    // TODO: Get time and compare with send time.
    //
    UNREFERENCED_PARAMETER(InputBuffer);
    UNREFERENCED_PARAMETER(InputBufferBytesWritten);
    UNREFERENCED_PARAMETER(OutputBuffer);
    UNREFERENCED_PARAMETER(OutputBufferBytesRead);
    UNREFERENCED_PARAMETER(CompletionStatus);

    DMFMODULE dmfModule = DMF_ParentModuleGet(DmfModuleDeviceInterfaceTarget);
    moduleContext = DMF_CONTEXT_GET(dmfModule);

    sleepIoctlBuffer = (Tests_IoctlHandler_Sleep*)ClientRequestContext;

    TraceEvents(TRACE_LEVEL_INFORMATION, DMF_TRACE, "DI: CANCELED sleepIoctlBuffer->TimeToSleepMilliseconds=%d sleepIoctlBuffer=0x%p", 
                sleepIoctlBuffer->TimeToSleepMilliseconds,
                OutputBuffer);

    if (sleepIoctlBuffer->ReuseEvent != NULL)
    {
        DMF_Portable_EventSet(sleepIoctlBuffer->ReuseEvent);
    }

    DMF_BufferPool_Put(moduleContext->DmfModuleBufferPool,
                       (VOID*)sleepIoctlBuffer);

#if !defined(DMF_WIN32_MODE)
    DmfAssert(STATUS_CANCELLED == CompletionStatus);
#endif
}

#pragma code_seg("PAGE")
_IRQL_requires_max_(PASSIVE_LEVEL)
static
void
Tests_DeviceInterfaceTarget_ThreadAction_Asynchronous(
    _In_ DMFMODULE DmfModule,
    _In_ DMFMODULE DmfModuleAlertableSleep,
    _In_ DMFMODULE DmfModuleDeviceInterfaceTarget
    )
{
    DMF_CONTEXT_Tests_DeviceInterfaceTarget* moduleContext;
    NTSTATUS ntStatus;
    size_t bytesWritten;
    ULONG timeoutMs;

    PAGED_CODE();

    moduleContext = DMF_CONTEXT_GET(DmfModule);

    Tests_IoctlHandler_Sleep* sleepIoctlBuffer;
    ntStatus = DMF_BufferPool_Get(moduleContext->DmfModuleBufferPool,
                                  (VOID**)&sleepIoctlBuffer,
                                  NULL);
    if (!NT_SUCCESS(ntStatus))
    {
        goto Exit;
    }

#if !defined(TEST_CANCEL_NORMAL)
    if (TestsUtility_GenerateRandomNumber(0,
                                          1))
    {
        timeoutMs = TestsUtility_GenerateRandomNumber(TIMEOUT_FAST_MS,
                                                      TIMEOUT_SLOW_MS);
    }
    else
    {
        timeoutMs = 0;
    }
#else
    timeoutMs = TIMEOUT_CANCEL_MS;
#endif

    RtlZeroMemory(sleepIoctlBuffer,
                  sizeof(Tests_IoctlHandler_Sleep));
    sleepIoctlBuffer->TimeToSleepMilliseconds = TestsUtility_GenerateRandomNumber(0, 
                                                                                  MAXIMUM_SLEEP_TIME_MS);
    bytesWritten = 0;
    TraceEvents(TRACE_LEVEL_INFORMATION, DMF_TRACE, "DI03:dmfModule=0x%p sleepIoctlBuffer->TimeToSleepMilliseconds=%ld", DmfModuleDeviceInterfaceTarget, sleepIoctlBuffer->TimeToSleepMilliseconds);
    ntStatus = DMF_DeviceInterfaceTarget_Send(DmfModuleDeviceInterfaceTarget,
                                              sleepIoctlBuffer,
                                              sizeof(Tests_IoctlHandler_Sleep),
                                              sleepIoctlBuffer,
                                              sizeof(Tests_IoctlHandler_Sleep),
                                              ContinuousRequestTarget_RequestType_Ioctl,
                                              IOCTL_Tests_IoctlHandler_SLEEP,
                                              timeoutMs,
                                              Tests_DeviceInterfaceTarget_SendCompletion,
                                              sleepIoctlBuffer);
    DmfAssert(NT_SUCCESS(ntStatus) ||
              (ntStatus == STATUS_CANCELLED) ||
              (ntStatus == STATUS_INVALID_DEVICE_STATE) ||
              (ntStatus == STATUS_DELETE_PENDING));

    // Reduce traffic to reduce CPU usage and make debugging easier.
    //
    ntStatus = DMF_AlertableSleep_Sleep(DmfModuleAlertableSleep,
                                        0,
                                        1000);
    if (!NT_SUCCESS(ntStatus))
    {
        goto Exit;
    }

Exit:
    ;
}
#pragma code_seg()

#pragma code_seg("PAGE")
_IRQL_requires_max_(PASSIVE_LEVEL)
static
void
Tests_DeviceInterfaceTarget_ThreadAction_AsynchronousCancel(
    _In_ DMFMODULE DmfModule,
    _In_ DMFMODULE DmfModuleAlertableSleep,
    _In_ DMFMODULE DmfModuleDeviceInterfaceTarget
    )
{
    DMF_CONTEXT_Tests_DeviceInterfaceTarget* moduleContext;
    NTSTATUS ntStatus;
    size_t bytesWritten;
    RequestTarget_DmfRequestCancel dmfRequestIdCancel;
    BOOLEAN requestCanceled;
    LONG timeToSleepMilliseconds;
    Tests_IoctlHandler_Sleep* sleepIoctlBuffer;

    PAGED_CODE();

    moduleContext = DMF_CONTEXT_GET(DmfModule);

    /////////////////////////////////////////////////////////////////////////////////////
    // Cancel the request after it is normally completed. It should never cancel unless driver is shutting down.
    //

    ntStatus = DMF_BufferPool_Get(moduleContext->DmfModuleBufferPool,
                                  (VOID**)&sleepIoctlBuffer,
                                  NULL);
    if (!NT_SUCCESS(ntStatus))
    {
        goto Exit;
    }

    timeToSleepMilliseconds = TestsUtility_GenerateRandomNumber(MINIMUM_SLEEP_TIME_MS, 
                                                                MAXIMUM_SLEEP_TIME_MS);

    RtlZeroMemory(sleepIoctlBuffer,
                  sizeof(Tests_IoctlHandler_Sleep));
    sleepIoctlBuffer->TimeToSleepMilliseconds = timeToSleepMilliseconds;
    bytesWritten = 0;
    TraceEvents(TRACE_LEVEL_INFORMATION, DMF_TRACE, "DI04:dmfModule=0x%p sleepIoctlBuffer->TimeToSleepMilliseconds=%ld", DmfModuleDeviceInterfaceTarget, sleepIoctlBuffer->TimeToSleepMilliseconds);
    ntStatus = DMF_DeviceInterfaceTarget_SendEx(DmfModuleDeviceInterfaceTarget,
                                                sleepIoctlBuffer,
                                                sizeof(Tests_IoctlHandler_Sleep),
                                                sleepIoctlBuffer,
                                                sizeof(Tests_IoctlHandler_Sleep),
                                                ContinuousRequestTarget_RequestType_Ioctl,
                                                IOCTL_Tests_IoctlHandler_SLEEP,
                                                0,
                                                Tests_DeviceInterfaceTarget_SendCompletion,
                                                sleepIoctlBuffer,
                                                &dmfRequestIdCancel);

    DmfAssert(NT_SUCCESS(ntStatus) ||
              (ntStatus == STATUS_CANCELLED) ||
              (ntStatus == STATUS_INVALID_DEVICE_STATE) ||
              (ntStatus == STATUS_DELETE_PENDING));
    if (!NT_SUCCESS(ntStatus))
    {
        goto Exit;
    }

    ntStatus = DMF_AlertableSleep_Sleep(DmfModuleAlertableSleep,
                                        0,
                                        timeToSleepMilliseconds * 4);
    // Cancel the request if possible.
    // It should never cancel since the time just waited is 4 times what was sent above.
    //
    requestCanceled = DMF_DeviceInterfaceTarget_Cancel(DmfModuleDeviceInterfaceTarget,
                                                       dmfRequestIdCancel);
    if (!NT_SUCCESS(ntStatus))
    {
        // Driver is shutting down...get out.
        //
        goto Exit;
    }

#if !defined(DMF_WIN32_MODE)
    DmfAssert(!requestCanceled);
#endif

    TraceEvents(TRACE_LEVEL_INFORMATION, DMF_TRACE, "DI: CANCELED: sleepIoctlBuffer->TimeToSleepMilliseconds=%d sleepIoctlBuffer=0x%p", 
                timeToSleepMilliseconds,
                sleepIoctlBuffer);

    /////////////////////////////////////////////////////////////////////////////////////
    // Cancel the request after waiting for a while. It may or may not be canceled.
    //

    ntStatus = DMF_BufferPool_Get(moduleContext->DmfModuleBufferPool,
                                  (VOID**)&sleepIoctlBuffer,
                                  NULL);
    if (!NT_SUCCESS(ntStatus))
    {
        goto Exit;
    }

    timeToSleepMilliseconds = TestsUtility_GenerateRandomNumber(0, 
                                                                MAXIMUM_SLEEP_TIME_MS);

    RtlZeroMemory(sleepIoctlBuffer,
                  sizeof(Tests_IoctlHandler_Sleep));
    sleepIoctlBuffer->TimeToSleepMilliseconds = timeToSleepMilliseconds;
    bytesWritten = 0;
    TraceEvents(TRACE_LEVEL_INFORMATION, DMF_TRACE, "DI05:dmfModule=0x%p sleepIoctlBuffer->TimeToSleepMilliseconds=%ld", DmfModuleDeviceInterfaceTarget, sleepIoctlBuffer->TimeToSleepMilliseconds);
    ntStatus = DMF_DeviceInterfaceTarget_SendEx(DmfModuleDeviceInterfaceTarget,
                                                sleepIoctlBuffer,
                                                sizeof(Tests_IoctlHandler_Sleep),
                                                sleepIoctlBuffer,
                                                sizeof(Tests_IoctlHandler_Sleep),
                                                ContinuousRequestTarget_RequestType_Ioctl,
                                                IOCTL_Tests_IoctlHandler_SLEEP,
                                                0,
                                                Tests_DeviceInterfaceTarget_SendCompletion,
                                                sleepIoctlBuffer,
                                                &dmfRequestIdCancel);

    DmfAssert(NT_SUCCESS(ntStatus) ||
              (ntStatus == STATUS_CANCELLED) ||
              (ntStatus == STATUS_INVALID_DEVICE_STATE) ||
              (ntStatus == STATUS_DELETE_PENDING));
    if (!NT_SUCCESS(ntStatus))
    {
        goto Exit;
    }

    timeToSleepMilliseconds = TestsUtility_GenerateRandomNumber(0, 
                                                                MAXIMUM_SLEEP_TIME_MS);
    ntStatus = DMF_AlertableSleep_Sleep(DmfModuleAlertableSleep,
                                        0,
                                        timeToSleepMilliseconds);

    // Cancel the request if possible.
    //
    requestCanceled = DMF_DeviceInterfaceTarget_Cancel(DmfModuleDeviceInterfaceTarget,
                                                       dmfRequestIdCancel);

    if (!NT_SUCCESS(ntStatus))
    {
        // Driver is shutting down...get out.
        //
        goto Exit;
    }

    /////////////////////////////////////////////////////////////////////////////////////
    // Cancel the request after waiting the same time sent in timeout. 
    // It may or may not be canceled.
    //

    ntStatus = DMF_BufferPool_Get(moduleContext->DmfModuleBufferPool,
                                  (VOID**)&sleepIoctlBuffer,
                                  NULL);
    if (!NT_SUCCESS(ntStatus))
    {
        goto Exit;
    }

    timeToSleepMilliseconds = TestsUtility_GenerateRandomNumber(0, 
                                                                MAXIMUM_SLEEP_TIME_MS);

    RtlZeroMemory(sleepIoctlBuffer,
                  sizeof(Tests_IoctlHandler_Sleep));
    sleepIoctlBuffer->TimeToSleepMilliseconds = timeToSleepMilliseconds;
    bytesWritten = 0;
    TraceEvents(TRACE_LEVEL_INFORMATION, DMF_TRACE, "DI06:dmfModule=0x%p sleepIoctlBuffer->TimeToSleepMilliseconds=%ld", DmfModuleDeviceInterfaceTarget, sleepIoctlBuffer->TimeToSleepMilliseconds);
    ntStatus = DMF_DeviceInterfaceTarget_SendEx(DmfModuleDeviceInterfaceTarget,
                                                sleepIoctlBuffer,
                                                sizeof(Tests_IoctlHandler_Sleep),
                                                sleepIoctlBuffer,
                                                sizeof(Tests_IoctlHandler_Sleep),
                                                ContinuousRequestTarget_RequestType_Ioctl,
                                                IOCTL_Tests_IoctlHandler_SLEEP,
                                                0,
                                                Tests_DeviceInterfaceTarget_SendCompletion,
                                                sleepIoctlBuffer,
                                                &dmfRequestIdCancel);

    DmfAssert(NT_SUCCESS(ntStatus) ||
              (ntStatus == STATUS_CANCELLED) ||
              (ntStatus == STATUS_INVALID_DEVICE_STATE) ||
              (ntStatus == STATUS_DELETE_PENDING));
    if (!NT_SUCCESS(ntStatus))
    {
        goto Exit;
    }

    ntStatus = DMF_AlertableSleep_Sleep(DmfModuleAlertableSleep,
                                        0,
                                        timeToSleepMilliseconds);

    // Cancel the request if possible.
    //
    requestCanceled = DMF_DeviceInterfaceTarget_Cancel(DmfModuleDeviceInterfaceTarget,
                                                       dmfRequestIdCancel);

    if (!NT_SUCCESS(ntStatus))
    {
        // Driver is shutting down...get out.
        //
        goto Exit;
    }

    /////////////////////////////////////////////////////////////////////////////////////
    // Cancel the request immediately after sending it. It may or may not be canceled.
    //

    ntStatus = DMF_BufferPool_Get(moduleContext->DmfModuleBufferPool,
                                  (VOID**)&sleepIoctlBuffer,
                                  NULL);
    if (!NT_SUCCESS(ntStatus))
    {
        goto Exit;
    }

    timeToSleepMilliseconds = TestsUtility_GenerateRandomNumber(0, 
                                                                MAXIMUM_SLEEP_TIME_MS);

    RtlZeroMemory(sleepIoctlBuffer,
                  sizeof(Tests_IoctlHandler_Sleep));
    sleepIoctlBuffer->TimeToSleepMilliseconds = timeToSleepMilliseconds;
    bytesWritten = 0;
    TraceEvents(TRACE_LEVEL_INFORMATION, DMF_TRACE, "DI07:dmfModule=0x%p sleepIoctlBuffer->TimeToSleepMilliseconds=%ld", DmfModuleDeviceInterfaceTarget, sleepIoctlBuffer->TimeToSleepMilliseconds);
    ntStatus = DMF_DeviceInterfaceTarget_SendEx(DmfModuleDeviceInterfaceTarget,
                                                sleepIoctlBuffer,
                                                sizeof(Tests_IoctlHandler_Sleep),
                                                sleepIoctlBuffer,
                                                sizeof(Tests_IoctlHandler_Sleep),
                                                ContinuousRequestTarget_RequestType_Ioctl,
                                                IOCTL_Tests_IoctlHandler_SLEEP,
                                                0,
                                                Tests_DeviceInterfaceTarget_SendCompletion,
                                                sleepIoctlBuffer,
                                                &dmfRequestIdCancel);

    DmfAssert(NT_SUCCESS(ntStatus) ||
              (ntStatus == STATUS_CANCELLED) ||
              (ntStatus == STATUS_INVALID_DEVICE_STATE) ||
              (ntStatus == STATUS_DELETE_PENDING));
    if (!NT_SUCCESS(ntStatus))
    {
        goto Exit;
    }

    // Cancel the request immediately after sending it.
    //
    requestCanceled = DMF_DeviceInterfaceTarget_Cancel(DmfModuleDeviceInterfaceTarget,
                                                       dmfRequestIdCancel);

    /////////////////////////////////////////////////////////////////////////////////////
    // Cancel the request before it is normally completed. It should always cancel.
    //

    ntStatus = DMF_BufferPool_Get(moduleContext->DmfModuleBufferPool,
                                  (VOID**)&sleepIoctlBuffer,
                                  NULL);
    if (!NT_SUCCESS(ntStatus))
    {
        goto Exit;
    }

    timeToSleepMilliseconds = TestsUtility_GenerateRandomNumber(MINIMUM_SLEEP_TIME_MS, 
                                                                MAXIMUM_SLEEP_TIME_MS);

    RtlZeroMemory(sleepIoctlBuffer,
                  sizeof(Tests_IoctlHandler_Sleep));
    sleepIoctlBuffer->TimeToSleepMilliseconds = timeToSleepMilliseconds;
    bytesWritten = 0;
    TraceEvents(TRACE_LEVEL_INFORMATION, DMF_TRACE, "DI08:dmfModule=0x%p sleepIoctlBuffer->TimeToSleepMilliseconds=%ld", DmfModuleDeviceInterfaceTarget, sleepIoctlBuffer->TimeToSleepMilliseconds);
    ntStatus = DMF_DeviceInterfaceTarget_SendEx(DmfModuleDeviceInterfaceTarget,
                                                sleepIoctlBuffer,
                                                sizeof(Tests_IoctlHandler_Sleep),
                                                sleepIoctlBuffer,
                                                sizeof(Tests_IoctlHandler_Sleep),
                                                ContinuousRequestTarget_RequestType_Ioctl,
                                                IOCTL_Tests_IoctlHandler_SLEEP,
                                                0,
                                                Tests_DeviceInterfaceTarget_SendCompletionMustBeCancelled,
                                                sleepIoctlBuffer,
                                                &dmfRequestIdCancel);

    DmfAssert(NT_SUCCESS(ntStatus) ||
              (ntStatus == STATUS_CANCELLED) ||
              (ntStatus == STATUS_INVALID_DEVICE_STATE) ||
              (ntStatus == STATUS_DELETE_PENDING));
    if (!NT_SUCCESS(ntStatus))
    {
        goto Exit;
    }

    ntStatus = DMF_AlertableSleep_Sleep(DmfModuleAlertableSleep,
                                        0,
                                        timeToSleepMilliseconds / 4);

    // Cancel the request if possible.
    // It should always cancel since the time just waited is 1/4 the time that was sent above.
    //
    requestCanceled = DMF_DeviceInterfaceTarget_Cancel(DmfModuleDeviceInterfaceTarget,
                                                       dmfRequestIdCancel);
    // Even though the attempt to cancel happens in 1/4 of the total time out, it is possible
    // that the cancel call happens just as the underlying driver is going away. In that case,
    // the request is not canceled by this call, but it will be canceled by the underlying
    // driver. (In this case the call to cancel returns FALSE.) Thus, no assert is possible here.
    // This case happens often as the underlying driver comes and goes every second.
    //

Exit:
    ;
}
#pragma code_seg()

#pragma code_seg("PAGE")
_IRQL_requires_max_(PASSIVE_LEVEL)
static
void
Tests_DeviceInterfaceTarget_ThreadAction_AsynchronousReuse(
    _In_ DMFMODULE DmfModule,
    _In_ DMFMODULE DmfModuleAlertableSleep,
    _In_ DMFMODULE DmfModuleDeviceInterfaceTarget
    )
{
    DMF_CONTEXT_Tests_DeviceInterfaceTarget* moduleContext;
    NTSTATUS ntStatus;
    size_t bytesWritten;
    RequestTarget_DmfRequestCancel dmfRequestIdCancel;
    RequestTarget_DmfRequestReuse dmfRequestIdReuse;
    BOOLEAN requestCanceled;
    LONG timeToSleepMilliseconds;
    Tests_IoctlHandler_Sleep* sleepIoctlBuffer;
    DMF_PORTABLE_EVENT reuseEvent;

    PAGED_CODE();

    moduleContext = DMF_CONTEXT_GET(DmfModule);

    dmfRequestIdReuse = 0;

    ntStatus = DMF_Portable_EventCreate(&reuseEvent,
                                        SynchronizationEvent,
                                        FALSE);
    if (!NT_SUCCESS(ntStatus))
    {
        goto ExitNoClearEvent;
    }

    ntStatus = DMF_DeviceInterfaceTarget_ReuseCreate(DmfModuleDeviceInterfaceTarget,
                                                     &dmfRequestIdReuse);
    if (!NT_SUCCESS(ntStatus))
    {
        goto Exit;
    }

    /////////////////////////////////////////////////////////////////////////////////////
    // Cancel the request after it is normally completed. It should never cancel unless driver is shutting down.
    //

    ntStatus = DMF_BufferPool_Get(moduleContext->DmfModuleBufferPool,
                                  (VOID**)&sleepIoctlBuffer,
                                  NULL);
    if (!NT_SUCCESS(ntStatus))
    {
        goto Exit;
    }

    timeToSleepMilliseconds = TestsUtility_GenerateRandomNumber(MINIMUM_SLEEP_TIME_MS, 
                                                                MAXIMUM_SLEEP_TIME_MS);

    RtlZeroMemory(sleepIoctlBuffer,
                  sizeof(Tests_IoctlHandler_Sleep));
    sleepIoctlBuffer->TimeToSleepMilliseconds = timeToSleepMilliseconds;
    sleepIoctlBuffer->ReuseEvent = &reuseEvent;

    bytesWritten = 0;
    TraceEvents(TRACE_LEVEL_INFORMATION, DMF_TRACE, "DI09:dmfModule=0x%p sleepIoctlBuffer->TimeToSleepMilliseconds=%ld", DmfModuleDeviceInterfaceTarget, sleepIoctlBuffer->TimeToSleepMilliseconds);
    ntStatus = DMF_DeviceInterfaceTarget_ReuseSend(DmfModuleDeviceInterfaceTarget,
                                                   dmfRequestIdReuse,
                                                   sleepIoctlBuffer,
                                                   sizeof(Tests_IoctlHandler_Sleep),
                                                   sleepIoctlBuffer,
                                                   sizeof(Tests_IoctlHandler_Sleep),
                                                   ContinuousRequestTarget_RequestType_Ioctl,
                                                   IOCTL_Tests_IoctlHandler_SLEEP,
                                                   0,
                                                   Tests_DeviceInterfaceTarget_SendCompletion,
                                                   sleepIoctlBuffer,
                                                   &dmfRequestIdCancel);

    DmfAssert(NT_SUCCESS(ntStatus) ||
              (ntStatus == STATUS_CANCELLED) ||
              (ntStatus == STATUS_INVALID_DEVICE_STATE) ||
              (ntStatus == STATUS_DELETE_PENDING));
    if (!NT_SUCCESS(ntStatus))
    {
        goto Exit;
    }

    ntStatus = DMF_AlertableSleep_Sleep(DmfModuleAlertableSleep,
                                        0,
                                        timeToSleepMilliseconds * 4);
    // Cancel the request if possible.
    // It should never cancel since the time just waited is 4 times what was sent above.
    //
    requestCanceled = DMF_DeviceInterfaceTarget_Cancel(DmfModuleDeviceInterfaceTarget,
                                                       dmfRequestIdCancel);

    DMF_Portable_EventWaitForSingleObject(&reuseEvent,
                                          NULL,
                                          FALSE);

    if (!NT_SUCCESS(ntStatus))
    {
        // Driver is shutting down...get out.
        //
        goto Exit;
    }

#if !defined(DMF_WIN32_MODE)
    DmfAssert(!requestCanceled);
#endif

    TraceEvents(TRACE_LEVEL_INFORMATION, DMF_TRACE, "DI: CANCELED: sleepIoctlBuffer->TimeToSleepMilliseconds=%d sleepIoctlBuffer=0x%p", 
                timeToSleepMilliseconds,
                sleepIoctlBuffer);

    /////////////////////////////////////////////////////////////////////////////////////
    // Cancel the request after waiting for a while. It may or may not be canceled.
    //

    ntStatus = DMF_BufferPool_Get(moduleContext->DmfModuleBufferPool,
                                  (VOID**)&sleepIoctlBuffer,
                                  NULL);
    if (!NT_SUCCESS(ntStatus))
    {
        goto Exit;
    }

    timeToSleepMilliseconds = TestsUtility_GenerateRandomNumber(0, 
                                                                MAXIMUM_SLEEP_TIME_MS);

    RtlZeroMemory(sleepIoctlBuffer,
                  sizeof(Tests_IoctlHandler_Sleep));
    sleepIoctlBuffer->TimeToSleepMilliseconds = timeToSleepMilliseconds;
    sleepIoctlBuffer->ReuseEvent = &reuseEvent;
    bytesWritten = 0;
    TraceEvents(TRACE_LEVEL_INFORMATION, DMF_TRACE, "DI10:dmfModule=0x%p sleepIoctlBuffer->TimeToSleepMilliseconds=%ld", DmfModuleDeviceInterfaceTarget, sleepIoctlBuffer->TimeToSleepMilliseconds);
    ntStatus = DMF_DeviceInterfaceTarget_ReuseSend(DmfModuleDeviceInterfaceTarget,
                                                   dmfRequestIdReuse,
                                                   sleepIoctlBuffer,
                                                   sizeof(Tests_IoctlHandler_Sleep),
                                                   sleepIoctlBuffer,
                                                   sizeof(Tests_IoctlHandler_Sleep),
                                                   ContinuousRequestTarget_RequestType_Ioctl,
                                                   IOCTL_Tests_IoctlHandler_SLEEP,
                                                   0,
                                                   Tests_DeviceInterfaceTarget_SendCompletion,
                                                   sleepIoctlBuffer,
                                                   &dmfRequestIdCancel);

    DmfAssert(NT_SUCCESS(ntStatus) ||
              (ntStatus == STATUS_CANCELLED) ||
              (ntStatus == STATUS_INVALID_DEVICE_STATE) ||
              (ntStatus == STATUS_DELETE_PENDING));
    if (!NT_SUCCESS(ntStatus))
    {
        goto Exit;
    }

    timeToSleepMilliseconds = TestsUtility_GenerateRandomNumber(0, 
                                                                MAXIMUM_SLEEP_TIME_MS);
    ntStatus = DMF_AlertableSleep_Sleep(DmfModuleAlertableSleep,
                                        0,
                                        timeToSleepMilliseconds);

    // Cancel the request if possible.
    //
    requestCanceled = DMF_DeviceInterfaceTarget_Cancel(DmfModuleDeviceInterfaceTarget,
                                                       dmfRequestIdCancel);

    DMF_Portable_EventWaitForSingleObject(&reuseEvent,
                                          NULL,
                                          FALSE);

    if (!NT_SUCCESS(ntStatus))
    {
        // Driver is shutting down...get out.
        //
        goto Exit;
    }

    /////////////////////////////////////////////////////////////////////////////////////
    // Cancel the request after waiting the same time sent in timeout. 
    // It may or may not be canceled.
    //

    ntStatus = DMF_BufferPool_Get(moduleContext->DmfModuleBufferPool,
                                  (VOID**)&sleepIoctlBuffer,
                                  NULL);
    if (!NT_SUCCESS(ntStatus))
    {
        goto Exit;
    }

    timeToSleepMilliseconds = TestsUtility_GenerateRandomNumber(0, 
                                                                MAXIMUM_SLEEP_TIME_MS);

    RtlZeroMemory(sleepIoctlBuffer,
                  sizeof(Tests_IoctlHandler_Sleep));
    sleepIoctlBuffer->TimeToSleepMilliseconds = timeToSleepMilliseconds;
    sleepIoctlBuffer->ReuseEvent = &reuseEvent;
    bytesWritten = 0;
    TraceEvents(TRACE_LEVEL_INFORMATION, DMF_TRACE, "DI11:dmfModule=0x%p sleepIoctlBuffer->TimeToSleepMilliseconds=%ld", DmfModuleDeviceInterfaceTarget, sleepIoctlBuffer->TimeToSleepMilliseconds);
    ntStatus = DMF_DeviceInterfaceTarget_ReuseSend(DmfModuleDeviceInterfaceTarget,
                                                   dmfRequestIdReuse,
                                                   sleepIoctlBuffer,
                                                   sizeof(Tests_IoctlHandler_Sleep),
                                                   sleepIoctlBuffer,
                                                   sizeof(Tests_IoctlHandler_Sleep),
                                                   ContinuousRequestTarget_RequestType_Ioctl,
                                                   IOCTL_Tests_IoctlHandler_SLEEP,
                                                   0,
                                                   Tests_DeviceInterfaceTarget_SendCompletion,
                                                   sleepIoctlBuffer,
                                                   &dmfRequestIdCancel);

    DmfAssert(NT_SUCCESS(ntStatus) ||
              (ntStatus == STATUS_CANCELLED) ||
              (ntStatus == STATUS_INVALID_DEVICE_STATE) ||
              (ntStatus == STATUS_DELETE_PENDING));
    if (!NT_SUCCESS(ntStatus))
    {
        goto Exit;
    }

    ntStatus = DMF_AlertableSleep_Sleep(DmfModuleAlertableSleep,
                                        0,
                                        timeToSleepMilliseconds);

    // Cancel the request if possible.
    //
    requestCanceled = DMF_DeviceInterfaceTarget_Cancel(DmfModuleDeviceInterfaceTarget,
                                                       dmfRequestIdCancel);

    DMF_Portable_EventWaitForSingleObject(&reuseEvent,
                                          NULL,
                                          FALSE);

    if (!NT_SUCCESS(ntStatus))
    {
        // Driver is shutting down...get out.
        //
        goto Exit;
    }

    /////////////////////////////////////////////////////////////////////////////////////
    // Cancel the request immediately after sending it. It may or may not be canceled.
    //

    ntStatus = DMF_BufferPool_Get(moduleContext->DmfModuleBufferPool,
                                  (VOID**)&sleepIoctlBuffer,
                                  NULL);
    if (!NT_SUCCESS(ntStatus))
    {
        goto Exit;
    }

    timeToSleepMilliseconds = TestsUtility_GenerateRandomNumber(0, 
                                                                MAXIMUM_SLEEP_TIME_MS);

    RtlZeroMemory(sleepIoctlBuffer,
                  sizeof(Tests_IoctlHandler_Sleep));
    sleepIoctlBuffer->TimeToSleepMilliseconds = timeToSleepMilliseconds;
    sleepIoctlBuffer->ReuseEvent = &reuseEvent;
    bytesWritten = 0;
    TraceEvents(TRACE_LEVEL_INFORMATION, DMF_TRACE, "DI12:dmfModule=0x%p sleepIoctlBuffer->TimeToSleepMilliseconds=%ld", DmfModuleDeviceInterfaceTarget, sleepIoctlBuffer->TimeToSleepMilliseconds);
    ntStatus = DMF_DeviceInterfaceTarget_ReuseSend(DmfModuleDeviceInterfaceTarget,
                                                   dmfRequestIdReuse,
                                                   sleepIoctlBuffer,
                                                   sizeof(Tests_IoctlHandler_Sleep),
                                                   sleepIoctlBuffer,
                                                   sizeof(Tests_IoctlHandler_Sleep),
                                                   ContinuousRequestTarget_RequestType_Ioctl,
                                                   IOCTL_Tests_IoctlHandler_SLEEP,
                                                   0,
                                                   Tests_DeviceInterfaceTarget_SendCompletion,
                                                   sleepIoctlBuffer,
                                                   &dmfRequestIdCancel);

    DmfAssert(NT_SUCCESS(ntStatus) ||
              (ntStatus == STATUS_CANCELLED) ||
              (ntStatus == STATUS_INVALID_DEVICE_STATE) ||
              (ntStatus == STATUS_DELETE_PENDING));
    if (!NT_SUCCESS(ntStatus))
    {
        goto Exit;
    }

    // Cancel the request immediately after sending it.
    //
    requestCanceled = DMF_DeviceInterfaceTarget_Cancel(DmfModuleDeviceInterfaceTarget,
                                                       dmfRequestIdCancel);

    DMF_Portable_EventWaitForSingleObject(&reuseEvent,
                                          NULL,
                                          FALSE);

    /////////////////////////////////////////////////////////////////////////////////////
    // Cancel the request before it is normally completed. It should always cancel.
    //

    ntStatus = DMF_BufferPool_Get(moduleContext->DmfModuleBufferPool,
                                  (VOID**)&sleepIoctlBuffer,
                                  NULL);
    if (!NT_SUCCESS(ntStatus))
    {
        goto Exit;
    }

    timeToSleepMilliseconds = TestsUtility_GenerateRandomNumber(MINIMUM_SLEEP_TIME_MS, 
                                                                MAXIMUM_SLEEP_TIME_MS);

    RtlZeroMemory(sleepIoctlBuffer,
                  sizeof(Tests_IoctlHandler_Sleep));
    sleepIoctlBuffer->TimeToSleepMilliseconds = timeToSleepMilliseconds;
    sleepIoctlBuffer->ReuseEvent = &reuseEvent;
    bytesWritten = 0;
    TraceEvents(TRACE_LEVEL_INFORMATION, DMF_TRACE, "DI13:dmfModule=0x%p sleepIoctlBuffer->TimeToSleepMilliseconds=%ld", DmfModuleDeviceInterfaceTarget, sleepIoctlBuffer->TimeToSleepMilliseconds);
    ntStatus = DMF_DeviceInterfaceTarget_ReuseSend(DmfModuleDeviceInterfaceTarget,
                                                   dmfRequestIdReuse,
                                                   sleepIoctlBuffer,
                                                   sizeof(Tests_IoctlHandler_Sleep),
                                                   sleepIoctlBuffer,
                                                   sizeof(Tests_IoctlHandler_Sleep),
                                                   ContinuousRequestTarget_RequestType_Ioctl,
                                                   IOCTL_Tests_IoctlHandler_SLEEP,
                                                   0,
                                                   Tests_DeviceInterfaceTarget_SendCompletionMustBeCancelled,
                                                   sleepIoctlBuffer,
                                                   &dmfRequestIdCancel);

    DmfAssert(NT_SUCCESS(ntStatus) ||
              (ntStatus == STATUS_CANCELLED) ||
              (ntStatus == STATUS_INVALID_DEVICE_STATE) ||
              (ntStatus == STATUS_DELETE_PENDING));
    if (!NT_SUCCESS(ntStatus))
    {
        goto Exit;
    }

    ntStatus = DMF_AlertableSleep_Sleep(DmfModuleAlertableSleep,
                                        0,
                                        timeToSleepMilliseconds / 4);

    // Cancel the request if possible.
    // It should always cancel since the time just waited is 1/4 the time that was sent above.
    //
    requestCanceled = DMF_DeviceInterfaceTarget_Cancel(DmfModuleDeviceInterfaceTarget,
                                                       dmfRequestIdCancel);
    // Even though the attempt to cancel happens in 1/4 of the total time out, it is possible
    // that the cancel call happens just as the underlying driver is going away. In that case,
    // the request is not canceled by this call, but it will be canceled by the underlying
    // driver. (In this case the call to cancel returns FALSE.) Thus, no assert is possible here.
    // This case happens often as the underlying driver comes and goes every second.
    //

    DMF_Portable_EventWaitForSingleObject(&reuseEvent,
                                          NULL,
                                          FALSE);

Exit:

    DMF_Portable_EventClose(&reuseEvent);

ExitNoClearEvent:

    if (dmfRequestIdReuse != NULL)
    {
        DMF_DeviceInterfaceTarget_ReuseDelete(DmfModuleDeviceInterfaceTarget,
                                              dmfRequestIdReuse);
    }
}
#pragma code_seg()

#if !defined(DYNAMIC_DISABLE)

#pragma code_seg("PAGE")
_IRQL_requires_max_(PASSIVE_LEVEL)
static
VOID
Tests_DeviceInterfaceTarget_ThreadAction_Dynamic(
    _In_ DMFMODULE DmfModule,
    _In_ DMFMODULE DmfModuleAlertableSleep
    )
{
    DMF_CONTEXT_Tests_DeviceInterfaceTarget* moduleContext;
    NTSTATUS ntStatus;
    size_t bytesWritten;
    ULONG timeoutMs;
    WDFMEMORY memory;
    WDF_OBJECT_ATTRIBUTES objectAttributes;

    PAGED_CODE();

    // Create a parent object for the Module Under Test.
    // Size does not matter because it is just used for parent object.
    //
    memory = NULL;
    WDF_OBJECT_ATTRIBUTES_INIT(&objectAttributes);
    objectAttributes.ParentObject = DmfModule;
    ntStatus = WdfMemoryCreate(&objectAttributes,
                               NonPagedPoolNx,
                               MemoryTag,
                               sizeof(VOID*),
                               &memory,
                               NULL);
    DmfAssert(NT_SUCCESS(ntStatus));

    moduleContext = DMF_CONTEXT_GET(DmfModule);

    Tests_IoctlHandler_Sleep* sleepIoctlBuffer;
    ntStatus = DMF_BufferPool_Get(moduleContext->DmfModuleBufferPool,
                                  (VOID**)&sleepIoctlBuffer,
                                  NULL);
    if (!NT_SUCCESS(ntStatus))
    {
        goto Exit;
    }

    RtlZeroMemory(sleepIoctlBuffer,
                  sizeof(Tests_IoctlHandler_Sleep));

#if !defined(TEST_CANCEL_NORMAL)
    if (TestsUtility_GenerateRandomNumber(0,
                                          1))
    {
        timeoutMs = TestsUtility_GenerateRandomNumber(TIMEOUT_FAST_MS,
                                                      TIMEOUT_SLOW_MS);
    }
    else
    {
        timeoutMs = 0;
    }
#else
    timeoutMs = TIMEOUT_CANCEL_MS;
#endif

    DMFMODULE dynamicDeviceInterfaceTarget;
    DMF_CONFIG_DeviceInterfaceTarget moduleConfigDeviceInterfaceTarget;
    DMF_MODULE_ATTRIBUTES moduleAttributes;
    WDFDEVICE device;

    device = DMF_ParentDeviceGet(DmfModule);
    WDF_OBJECT_ATTRIBUTES_INIT(&objectAttributes);
    objectAttributes.ParentObject = memory;

    // DeviceInterfaceTarget (DISPATCH_LEVEL)
    // Processes Input Buffers.
    //
    DMF_CONFIG_DeviceInterfaceTarget_AND_ATTRIBUTES_INIT(&moduleConfigDeviceInterfaceTarget,
                                                         &moduleAttributes);
    moduleConfigDeviceInterfaceTarget.DeviceInterfaceTargetGuid = GUID_DEVINTERFACE_Tests_IoctlHandler;
#if !defined(TEST_SIMPLE)
    moduleConfigDeviceInterfaceTarget.ContinuousRequestTargetModuleConfig.BufferCountInput = 1;
    moduleConfigDeviceInterfaceTarget.ContinuousRequestTargetModuleConfig.BufferCountOutput = 1;
    moduleConfigDeviceInterfaceTarget.ContinuousRequestTargetModuleConfig.BufferInputSize = sizeof(Tests_IoctlHandler_Sleep);
    moduleConfigDeviceInterfaceTarget.ContinuousRequestTargetModuleConfig.BufferOutputSize = sizeof(Tests_IoctlHandler_Sleep);
    moduleConfigDeviceInterfaceTarget.ContinuousRequestTargetModuleConfig.ContinuousRequestCount = 1;
    moduleConfigDeviceInterfaceTarget.ContinuousRequestTargetModuleConfig.PoolTypeInput = NonPagedPoolNx;
    moduleConfigDeviceInterfaceTarget.ContinuousRequestTargetModuleConfig.PurgeAndStartTargetInD0Callbacks = FALSE;
    moduleConfigDeviceInterfaceTarget.ContinuousRequestTargetModuleConfig.ContinuousRequestTargetIoctl = IOCTL_Tests_IoctlHandler_SLEEP;
    moduleConfigDeviceInterfaceTarget.ContinuousRequestTargetModuleConfig.EvtContinuousRequestTargetBufferInput = Tests_DeviceInterfaceTarget_BufferInput;
    moduleConfigDeviceInterfaceTarget.ContinuousRequestTargetModuleConfig.RequestType = ContinuousRequestTarget_RequestType_Ioctl;
    moduleConfigDeviceInterfaceTarget.ContinuousRequestTargetModuleConfig.ContinuousRequestTargetMode = ContinuousRequestTarget_Mode_Automatic;
#endif
    ntStatus = DMF_DeviceInterfaceTarget_Create(device,
                                                &moduleAttributes,
                                                &objectAttributes,
                                                &dynamicDeviceInterfaceTarget);
    if (!NT_SUCCESS(ntStatus))
    {
        goto Exit;
    }

    // Wait for underlying target to open.
    //
    ULONG timeToWaitMs = TestsUtility_GenerateRandomNumber(0, 
                                                           MAXIMUM_SLEEP_TIME_MS);
    ntStatus = DMF_AlertableSleep_Sleep(DmfModuleAlertableSleep,
                                        0,
                                        timeToWaitMs);

    // Send it some data synchronously..
    //
    RtlZeroMemory(sleepIoctlBuffer,
                  sizeof(Tests_IoctlHandler_Sleep));
    sleepIoctlBuffer->TimeToSleepMilliseconds = TestsUtility_GenerateRandomNumber(0, 
                                                                                  MAXIMUM_SLEEP_TIME_MS);
    // Wait at least this long for the synchronous request to return.
    //
    timeToWaitMs = sleepIoctlBuffer->TimeToSleepMilliseconds;

    bytesWritten = 0;
    TraceEvents(TRACE_LEVEL_INFORMATION, DMF_TRACE, "DI14:dmfModule=0x%p sleepIoctlBuffer->TimeToSleepMilliseconds=%ld", dynamicDeviceInterfaceTarget, sleepIoctlBuffer->TimeToSleepMilliseconds);
    ntStatus = DMF_DeviceInterfaceTarget_SendSynchronously(dynamicDeviceInterfaceTarget,
                                                           sleepIoctlBuffer,
                                                           sizeof(Tests_IoctlHandler_Sleep),
                                                           sleepIoctlBuffer,
                                                           sizeof(Tests_IoctlHandler_Sleep),
                                                           ContinuousRequestTarget_RequestType_Ioctl,
                                                           IOCTL_Tests_IoctlHandler_SLEEP,
                                                           timeoutMs,
                                                           &bytesWritten);
    TraceEvents(TRACE_LEVEL_INFORMATION, DMF_TRACE, "DI14:dmfModule=0x%p sleepIoctlBuffer->TimeToSleepMilliseconds=%ld SyncComplete", dynamicDeviceInterfaceTarget, sleepIoctlBuffer->TimeToSleepMilliseconds);
    DmfAssert(NT_SUCCESS(ntStatus) ||
              (ntStatus == STATUS_CANCELLED) ||
              (ntStatus == STATUS_INVALID_DEVICE_STATE) ||
              (ntStatus == STATUS_DELETE_PENDING) ||
              (ntStatus == STATUS_IO_TIMEOUT));

#if !defined(TEST_SIMPLE)
    DMF_DeviceInterfaceTarget_StreamStop(dynamicDeviceInterfaceTarget);
#endif

Exit:

    if (memory != NULL)
    {
        // Delete the Dynamic Module by deleting its parent to execute the hardest path.
        //
        // NOTE: When sending asynchronous requests, the object cannot be deleted if a
        //       WDFREQEUST is pending. WDF checks first for pending requests prior to
        //       invoke Close() callbacks.
        //
        WdfObjectDelete(memory);
    }
}
#pragma code_seg()

#endif

#pragma code_seg("PAGE")
_Function_class_(EVT_DMF_Thread_Function)
_IRQL_requires_max_(PASSIVE_LEVEL)
static
VOID
Tests_DeviceInterfaceTarget_WorkThreadDispatchInput(
    _In_ DMFMODULE DmfModuleThread
    )
{
    DMFMODULE dmfModule;
    DMF_CONTEXT_Tests_DeviceInterfaceTarget* moduleContext;
    TEST_ACTION testAction;
    Tests_DeviceInterfaceTarget_THREAD_INDEX_CONTEXT* threadIndex;

    PAGED_CODE();

    dmfModule = DMF_ParentModuleGet(DmfModuleThread);
    threadIndex = WdfObjectGet_Tests_DeviceInterfaceTarget_THREAD_INDEX_CONTEXT(DmfModuleThread);
    moduleContext = DMF_CONTEXT_GET(dmfModule);

#if defined(TEST_SYNCHRONOUS_ONLY)
    testAction = TEST_ACTION_SYNCHRONOUS;
#elif defined(TEST_ASYNCHRONOUS_ONLY)
    testAction = TEST_ACTION_ASYNCHRONOUS;
#elif defined(TEST_ASYNCHRONOUSCANCEL_ONLY)
    testAction = TEST_ACTION_ASYNCHRONOUSCANCEL;
#elif defined(TEST_ASYNCHRONOUSREUSE_ONLY)
    testAction = TEST_ACTION_ASYNCHRONOUSREUSE;
#elif defined(TEST_DYNAMIC_ONLY)
    testAction = TEST_ACTION_DYNAMIC;
#else
    // Generate a random test action Id for a current iteration.
    //
    testAction = (TEST_ACTION)TestsUtility_GenerateRandomNumber(TEST_ACTION_MINIUM,
                                                                TEST_ACTION_MAXIMUM);
#endif

    // Execute the test action.
    //
    switch (testAction)
    {
        case TEST_ACTION_SYNCHRONOUS:
            Tests_DeviceInterfaceTarget_ThreadAction_Synchronous(dmfModule,
                                                                 threadIndex->DmfModuleAlertableSleep,
                                                                 moduleContext->DmfModuleDeviceInterfaceTargetDispatchInput);
            break;
        case TEST_ACTION_ASYNCHRONOUS:
            Tests_DeviceInterfaceTarget_ThreadAction_Asynchronous(dmfModule,
                                                                  threadIndex->DmfModuleAlertableSleep,
                                                                  moduleContext->DmfModuleDeviceInterfaceTargetDispatchInput);
            break;
        case TEST_ACTION_ASYNCHRONOUSCANCEL:
            Tests_DeviceInterfaceTarget_ThreadAction_AsynchronousCancel(dmfModule,
                                                                        threadIndex->DmfModuleAlertableSleep,
                                                                        moduleContext->DmfModuleDeviceInterfaceTargetDispatchInput);
            break;
        case TEST_ACTION_ASYNCHRONOUSREUSE:
            Tests_DeviceInterfaceTarget_ThreadAction_AsynchronousReuse(dmfModule,
                                                                       threadIndex->DmfModuleAlertableSleep,
                                                                       moduleContext->DmfModuleDeviceInterfaceTargetDispatchInput);
            break;
#if defined(DMF_KERNEL_MODE)
        case TEST_ACTION_DIRECTINTERFACE:
            Tests_DeviceInterfaceTarget_DirectInterfaceTest(moduleContext->DmfModuleDeviceInterfaceTargetDispatchInput);
            break;
#endif // defined(DMF_KERNEL_MODE)
        case TEST_ACTION_DYNAMIC:
#if !defined(DYNAMIC_DISABLE)
            Tests_DeviceInterfaceTarget_ThreadAction_Dynamic(dmfModule,
                                                             threadIndex->DmfModuleAlertableSleep);
#endif // !defined(DYNAMIC_DISABLE)
            break;
        default:
            DmfAssert(FALSE);
            break;
    }

    // Repeat the test, until stop is signaled.
    //
    if (!DMF_Thread_IsStopPending(DmfModuleThread))
    {
        // Short delay to reduce traffic.
        //
        DMF_Utility_DelayMilliseconds(TIMEOUT_TRAFFIC_DELAY_MS);
        DMF_Thread_WorkReady(DmfModuleThread);
    }

    TestsUtility_YieldExecution();
}
#pragma code_seg()

#if !defined(TEST_SIMPLE)

#pragma code_seg("PAGE")
_Function_class_(EVT_DMF_Thread_Function)
_IRQL_requires_max_(PASSIVE_LEVEL)
static
VOID
Tests_DeviceInterfaceTarget_WorkThreadPassiveInput(
    _In_ DMFMODULE DmfModuleThread
    )
{
    DMFMODULE dmfModule;
    DMF_CONTEXT_Tests_DeviceInterfaceTarget* moduleContext;
    TEST_ACTION testAction;
    Tests_DeviceInterfaceTarget_THREAD_INDEX_CONTEXT* threadIndex;

    PAGED_CODE();

    dmfModule = DMF_ParentModuleGet(DmfModuleThread);
    threadIndex = WdfObjectGet_Tests_DeviceInterfaceTarget_THREAD_INDEX_CONTEXT(DmfModuleThread);
    moduleContext = DMF_CONTEXT_GET(dmfModule);

#if defined(TEST_SYNCHRONOUS_ONLY)
    testAction = TEST_ACTION_SYNCHRONOUS;
#elif defined(TEST_ASYNCHRONOUS_ONLY)
    testAction = TEST_ACTION_ASYNCHRONOUS;
#elif defined(TEST_ASYNCHRONOUSCANCEL_ONLY)
    testAction = TEST_ACTION_ASYNCHRONOUSCANCEL;
#elif defined(TEST_ASYNCHRONOUSREUSE_ONLY)
    testAction = TEST_ACTION_ASYNCHRONOUSREUSE;
#elif defined(TEST_DYNAMIC_ONLY)
    testAction = TEST_ACTION_DYNAMIC;
#else
    // Generate a random test action Id for a current iteration.
    //
    testAction = (TEST_ACTION)TestsUtility_GenerateRandomNumber(TEST_ACTION_MINIUM,
                                                                TEST_ACTION_MAXIMUM);
#endif

    // Execute the test action.
    //
    switch (testAction)
    {
        case TEST_ACTION_SYNCHRONOUS:
            Tests_DeviceInterfaceTarget_ThreadAction_Synchronous(dmfModule,
                                                                 threadIndex->DmfModuleAlertableSleep,
                                                                 moduleContext->DmfModuleDeviceInterfaceTargetPassiveInput);
            break;
        case TEST_ACTION_ASYNCHRONOUS:
            Tests_DeviceInterfaceTarget_ThreadAction_Asynchronous(dmfModule,
                                                                  threadIndex->DmfModuleAlertableSleep,
                                                                  moduleContext->DmfModuleDeviceInterfaceTargetPassiveInput);
            break;
        case TEST_ACTION_ASYNCHRONOUSCANCEL:
            Tests_DeviceInterfaceTarget_ThreadAction_AsynchronousCancel(dmfModule,
                                                                        threadIndex->DmfModuleAlertableSleep,
                                                                        moduleContext->DmfModuleDeviceInterfaceTargetPassiveInput);
            break;
        case TEST_ACTION_ASYNCHRONOUSREUSE:
            Tests_DeviceInterfaceTarget_ThreadAction_AsynchronousReuse(dmfModule,
                                                                       threadIndex->DmfModuleAlertableSleep,
                                                                       moduleContext->DmfModuleDeviceInterfaceTargetPassiveInput);
            break;
#if defined(DMF_KERNEL_MODE)
        case TEST_ACTION_DIRECTINTERFACE:
            Tests_DeviceInterfaceTarget_DirectInterfaceTest(moduleContext->DmfModuleDeviceInterfaceTargetPassiveInput);
            break;
#endif // defined(DMF_KERNEL_MODE)
        case TEST_ACTION_DYNAMIC:
#if !defined(DYNAMIC_DISABLE)
            Tests_DeviceInterfaceTarget_ThreadAction_Dynamic(dmfModule,
                                                             threadIndex->DmfModuleAlertableSleep);
#endif // !defined(DYNAMIC_DISABLE)
            break;
        default:
            DmfAssert(FALSE);
            break;
    }

    // Repeat the test, until stop is signaled.
    //
    if (!DMF_Thread_IsStopPending(DmfModuleThread))
    {
        // Short delay to reduce traffic.
        //
        DMF_Utility_DelayMilliseconds(TIMEOUT_TRAFFIC_DELAY_MS);
        DMF_Thread_WorkReady(DmfModuleThread);
    }

    TestsUtility_YieldExecution();
}
#pragma code_seg()

#pragma code_seg("PAGE")
_Function_class_(EVT_DMF_Thread_Function)
_IRQL_requires_max_(PASSIVE_LEVEL)
static
VOID
Tests_DeviceInterfaceTarget_WorkThreadPassiveOutput(
    _In_ DMFMODULE DmfModuleThread
    )
{
    DMFMODULE dmfModule;
    DMF_CONTEXT_Tests_DeviceInterfaceTarget* moduleContext;
    TEST_ACTION testAction;
    Tests_DeviceInterfaceTarget_THREAD_INDEX_CONTEXT* threadIndex;

    PAGED_CODE();

    dmfModule = DMF_ParentModuleGet(DmfModuleThread);
    threadIndex = WdfObjectGet_Tests_DeviceInterfaceTarget_THREAD_INDEX_CONTEXT(DmfModuleThread);
    moduleContext = DMF_CONTEXT_GET(dmfModule);

#if defined(TEST_SYNCHRONOUS_ONLY)
    testAction = TEST_ACTION_SYNCHRONOUS;
#elif defined(TEST_ASYNCHRONOUS_ONLY)
    testAction = TEST_ACTION_ASYNCHRONOUS;
#elif defined(TEST_ASYNCHRONOUSCANCEL_ONLY)
    testAction = TEST_ACTION_ASYNCHRONOUSCANCEL;
#elif defined(TEST_ASYNCHRONOUSREUSE_ONLY)
    testAction = TEST_ACTION_ASYNCHRONOUSREUSE;
#elif defined(TEST_DYNAMIC_ONLY)
    testAction = TEST_ACTION_DYNAMIC;
#else
    // Generate a random test action Id for a current iteration.
    //
    testAction = (TEST_ACTION)TestsUtility_GenerateRandomNumber(TEST_ACTION_MINIUM,
                                                                TEST_ACTION_MAXIMUM);
#endif

    // Execute the test action.
    //
    switch (testAction)
    {
        case TEST_ACTION_SYNCHRONOUS:
            Tests_DeviceInterfaceTarget_ThreadAction_Synchronous(dmfModule,
                                                                 threadIndex->DmfModuleAlertableSleep,
                                                                 moduleContext->DmfModuleDeviceInterfaceTargetPassiveOutput);
            break;
        case TEST_ACTION_ASYNCHRONOUS:
            Tests_DeviceInterfaceTarget_ThreadAction_Asynchronous(dmfModule,
                                                                  threadIndex->DmfModuleAlertableSleep,
                                                                  moduleContext->DmfModuleDeviceInterfaceTargetPassiveOutput);
            break;
        case TEST_ACTION_ASYNCHRONOUSCANCEL:
            Tests_DeviceInterfaceTarget_ThreadAction_AsynchronousCancel(dmfModule,
                                                                        threadIndex->DmfModuleAlertableSleep,
                                                                        moduleContext->DmfModuleDeviceInterfaceTargetPassiveOutput);
            break;
        case TEST_ACTION_ASYNCHRONOUSREUSE:
            Tests_DeviceInterfaceTarget_ThreadAction_AsynchronousReuse(dmfModule,
                                                                       threadIndex->DmfModuleAlertableSleep,
                                                                       moduleContext->DmfModuleDeviceInterfaceTargetPassiveOutput);
            break;
#if defined(DMF_KERNEL_MODE)
        case TEST_ACTION_DIRECTINTERFACE:
            Tests_DeviceInterfaceTarget_DirectInterfaceTest(moduleContext->DmfModuleDeviceInterfaceTargetPassiveOutput);
            break;
#endif // defined(DMF_KERNEL_MODE)
        case TEST_ACTION_DYNAMIC:
#if !defined(DYNAMIC_DISABLE)
            Tests_DeviceInterfaceTarget_ThreadAction_Dynamic(dmfModule,
                                                             threadIndex->DmfModuleAlertableSleep);
#endif // !defined(DYNAMIC_DISABLE)
            break;
        default:
            DmfAssert(FALSE);
            break;
    }

    // Repeat the test, until stop is signaled.
    //
    if (!DMF_Thread_IsStopPending(DmfModuleThread))
    {
        // Short delay to reduce traffic.
        //
        DMF_Utility_DelayMilliseconds(TIMEOUT_TRAFFIC_DELAY_MS);
        DMF_Thread_WorkReady(DmfModuleThread);
    }

    TestsUtility_YieldExecution();
}
#pragma code_seg()

#pragma code_seg("PAGE")
_Function_class_(EVT_DMF_Thread_Function)
_IRQL_requires_max_(PASSIVE_LEVEL)
static
VOID
Tests_DeviceInterfaceTarget_WorkThreadPassiveNonContinuous(
    _In_ DMFMODULE DmfModuleThread
    )
{
    DMFMODULE dmfModule;
    DMF_CONTEXT_Tests_DeviceInterfaceTarget* moduleContext;
    TEST_ACTION testAction;
    Tests_DeviceInterfaceTarget_THREAD_INDEX_CONTEXT* threadIndex;

    PAGED_CODE();

    dmfModule = DMF_ParentModuleGet(DmfModuleThread);
    threadIndex = WdfObjectGet_Tests_DeviceInterfaceTarget_THREAD_INDEX_CONTEXT(DmfModuleThread);
    moduleContext = DMF_CONTEXT_GET(dmfModule);

#if defined(TEST_SYNCHRONOUS_ONLY)
    testAction = TEST_ACTION_SYNCHRONOUS;
#elif defined(TEST_ASYNCHRONOUS_ONLY)
    testAction = TEST_ACTION_ASYNCHRONOUS;
#elif defined(TEST_ASYNCHRONOUSCANCEL_ONLY)
    testAction = TEST_ACTION_ASYNCHRONOUSCANCEL;
#elif defined(TEST_ASYNCHRONOUSREUSE_ONLY)
    testAction = TEST_ACTION_ASYNCHRONOUSREUSE;
#elif defined(TEST_DYNAMIC_ONLY)
    testAction = TEST_ACTION_DYNAMIC;
#else
    // Generate a random test action Id for a current iteration.
    //
    testAction = (TEST_ACTION)TestsUtility_GenerateRandomNumber(TEST_ACTION_MINIUM,
                                                                TEST_ACTION_MAXIMUM);
#endif

    // Execute the test action.
    //
    switch (testAction)
    {
        case TEST_ACTION_SYNCHRONOUS:
            Tests_DeviceInterfaceTarget_ThreadAction_Synchronous(dmfModule,
                                                                 threadIndex->DmfModuleAlertableSleep,
                                                                 moduleContext->DmfModuleDeviceInterfaceTargetPassiveNonContinuous);
            break;
        case TEST_ACTION_ASYNCHRONOUS:
            Tests_DeviceInterfaceTarget_ThreadAction_Asynchronous(dmfModule,
                                                                  threadIndex->DmfModuleAlertableSleep,
                                                                  moduleContext->DmfModuleDeviceInterfaceTargetPassiveNonContinuous);
            break;
        case TEST_ACTION_ASYNCHRONOUSCANCEL:
            Tests_DeviceInterfaceTarget_ThreadAction_AsynchronousCancel(dmfModule,
                                                                        threadIndex->DmfModuleAlertableSleep,
                                                                        moduleContext->DmfModuleDeviceInterfaceTargetPassiveNonContinuous);
            break;
        case TEST_ACTION_ASYNCHRONOUSREUSE:
            Tests_DeviceInterfaceTarget_ThreadAction_AsynchronousReuse(dmfModule,
                                                                       threadIndex->DmfModuleAlertableSleep,
                                                                       moduleContext->DmfModuleDeviceInterfaceTargetPassiveNonContinuous);
            break;
#if defined(DMF_KERNEL_MODE)
        case TEST_ACTION_DIRECTINTERFACE:
            Tests_DeviceInterfaceTarget_DirectInterfaceTest(moduleContext->DmfModuleDeviceInterfaceTargetPassiveNonContinuous);
            break;
#endif // defined(DMF_KERNEL_MODE)
        case TEST_ACTION_DYNAMIC:
#if !defined(DYNAMIC_DISABLE)
            Tests_DeviceInterfaceTarget_ThreadAction_Dynamic(dmfModule,
                                                             threadIndex->DmfModuleAlertableSleep);
#endif // !defined(DYNAMIC_DISABLE)
            break;
        default:
            DmfAssert(FALSE);
            break;
    }

    // Repeat the test, until stop is signaled.
    //
    if (!DMF_Thread_IsStopPending(DmfModuleThread))
    {
        // Short delay to reduce traffic.
        //
        DMF_Utility_DelayMilliseconds(TIMEOUT_TRAFFIC_DELAY_MS);
        DMF_Thread_WorkReady(DmfModuleThread);
    }

    TestsUtility_YieldExecution();
}
#pragma code_seg()

#pragma code_seg("PAGE")
_Function_class_(EVT_DMF_Thread_Function)
_IRQL_requires_max_(PASSIVE_LEVEL)
static
VOID
Tests_DeviceInterfaceTarget_WorkThreadDispatchNonContinuous(
    _In_ DMFMODULE DmfModuleThread
    )
{
    DMFMODULE dmfModule;
    DMF_CONTEXT_Tests_DeviceInterfaceTarget* moduleContext;
    TEST_ACTION testAction;
    Tests_DeviceInterfaceTarget_THREAD_INDEX_CONTEXT* threadIndex;

    PAGED_CODE();

    dmfModule = DMF_ParentModuleGet(DmfModuleThread);
    threadIndex = WdfObjectGet_Tests_DeviceInterfaceTarget_THREAD_INDEX_CONTEXT(DmfModuleThread);
    moduleContext = DMF_CONTEXT_GET(dmfModule);

#if defined(TEST_SYNCHRONOUS_ONLY)
    testAction = TEST_ACTION_SYNCHRONOUS;
#elif defined(TEST_ASYNCHRONOUS_ONLY)
    testAction = TEST_ACTION_ASYNCHRONOUS;
#elif defined(TEST_ASYNCHRONOUSCANCEL_ONLY)
    testAction = TEST_ACTION_ASYNCHRONOUSCANCEL;
#elif defined(TEST_ASYNCHRONOUSREUSE_ONLY)
    testAction = TEST_ACTION_ASYNCHRONOUSREUSE;
#elif defined(TEST_DYNAMIC_ONLY)
    testAction = TEST_ACTION_DYNAMIC;
#else
    // Generate a random test action Id for a current iteration.
    //
    testAction = (TEST_ACTION)TestsUtility_GenerateRandomNumber(TEST_ACTION_MINIUM,
                                                                TEST_ACTION_MAXIMUM);
#endif

    // Execute the test action.
    //
    switch (testAction)
    {
        case TEST_ACTION_SYNCHRONOUS:
            Tests_DeviceInterfaceTarget_ThreadAction_Synchronous(dmfModule,
                                                                 threadIndex->DmfModuleAlertableSleep,
                                                                 moduleContext->DmfModuleDeviceInterfaceTargetDispatchNonContinuous);
            break;
        case TEST_ACTION_ASYNCHRONOUS:
            Tests_DeviceInterfaceTarget_ThreadAction_Asynchronous(dmfModule,
                                                                  threadIndex->DmfModuleAlertableSleep,
                                                                  moduleContext->DmfModuleDeviceInterfaceTargetDispatchNonContinuous);
            break;
        case TEST_ACTION_ASYNCHRONOUSCANCEL:
            Tests_DeviceInterfaceTarget_ThreadAction_AsynchronousCancel(dmfModule,
                                                                        threadIndex->DmfModuleAlertableSleep,
                                                                        moduleContext->DmfModuleDeviceInterfaceTargetDispatchNonContinuous);
            break;
        case TEST_ACTION_ASYNCHRONOUSREUSE:
            Tests_DeviceInterfaceTarget_ThreadAction_AsynchronousReuse(dmfModule,
                                                                       threadIndex->DmfModuleAlertableSleep,
                                                                       moduleContext->DmfModuleDeviceInterfaceTargetDispatchNonContinuous);
            break;
#if defined(DMF_KERNEL_MODE)
        case TEST_ACTION_DIRECTINTERFACE:
            Tests_DeviceInterfaceTarget_DirectInterfaceTest(moduleContext->DmfModuleDeviceInterfaceTargetDispatchNonContinuous);
            break;
#endif // defined(DMF_KERNEL_MODE)
        case TEST_ACTION_DYNAMIC:
#if !defined(DYNAMIC_DISABLE)
            Tests_DeviceInterfaceTarget_ThreadAction_Dynamic(dmfModule,
                                                             threadIndex->DmfModuleAlertableSleep);
#endif // !defined(DYNAMIC_DISABLE)
            break;
        default:
            DmfAssert(FALSE);
            break;
    }

    // Repeat the test, until stop is signaled.
    //
    if (!DMF_Thread_IsStopPending(DmfModuleThread))
    {
        // Short delay to reduce traffic.
        //
        DMF_Utility_DelayMilliseconds(TIMEOUT_TRAFFIC_DELAY_MS);
        DMF_Thread_WorkReady(DmfModuleThread);
    }

    TestsUtility_YieldExecution();
}
#pragma code_seg()

#endif // !defined(TEST_SIMPLE)

#pragma code_seg("PAGE")
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
Tests_DeviceInterfaceTarget_StartDispatchInput(
    _In_ DMFMODULE DmfModule
    )
/*++

Routine Description:

    Starts the threads that send asynchronous data to the automatically started
    DMF_DeviceInterfaceTarget Modules.

Arguments:

    DmfModule - DMF_Tests_DeviceInterfaceTarget.

Return Value:

    NTSTATUS

--*/
{
    DMF_CONTEXT_Tests_DeviceInterfaceTarget* moduleContext;
    NTSTATUS ntStatus;
    LONG threadIndex;

    PAGED_CODE();

    FuncEntry(DMF_TRACE);

    moduleContext = DMF_CONTEXT_GET(DmfModule);

    ntStatus = STATUS_SUCCESS;

    for (threadIndex = 0; threadIndex < THREAD_COUNT; threadIndex++)
    {
        ntStatus = DMF_Thread_Start(moduleContext->DmfModuleThreadDispatchInput[threadIndex]);
        if (!NT_SUCCESS(ntStatus))
        {
            goto Exit;
        }
    }

    for (threadIndex = 0; threadIndex < THREAD_COUNT; threadIndex++)
    {
        DMF_Thread_WorkReady(moduleContext->DmfModuleThreadDispatchInput[threadIndex]);
    }

Exit:
    
    FuncExit(DMF_TRACE, "ntStatus=%!STATUS!", ntStatus);

    return ntStatus;
}
#pragma code_seg()

#pragma code_seg("PAGE")
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
Tests_DeviceInterfaceTarget_StopDispatchInput(
    _In_ DMFMODULE DmfModule
    )
/*++

Routine Description:

    Stops the threads that send asynchronous data to the automatically started
    DMF_DeviceInterfaceTarget Modules.

Arguments:

    DmfModule - DMF_Tests_DeviceInterfaceTarget.

Return Value:

    None

--*/
{
    DMF_CONTEXT_Tests_DeviceInterfaceTarget* moduleContext;
    LONG threadIndex;

    PAGED_CODE();

    FuncEntry(DMF_TRACE);

    moduleContext = DMF_CONTEXT_GET(DmfModule);

    for (threadIndex = 0; threadIndex < THREAD_COUNT; threadIndex++)
    {
        // Interrupt any long sleeps.
        //
        DMF_AlertableSleep_Abort(moduleContext->DmfModuleAlertableSleepDispatchInput[threadIndex],
                                 0);
        // Stop thread.
        //
        DMF_Thread_Stop(moduleContext->DmfModuleThreadDispatchInput[threadIndex]);
    }

    FuncExitVoid(DMF_TRACE);
}
#pragma code_seg()

#pragma code_seg("PAGE")
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
Tests_DeviceInterfaceTarget_StartPassiveInput(
    _In_ DMFMODULE DmfModule
    )
/*++

Routine Description:

    Starts the threads that send asynchronous data to the manually started
    DMF_DeviceInterfaceTarget Modules.

Arguments:

    DmfModule - DMF_Tests_DeviceInterfaceTarget.

Return Value:

    NTSTATUS

--*/
{
    DMF_CONTEXT_Tests_DeviceInterfaceTarget* moduleContext;
    NTSTATUS ntStatus;
    LONG threadIndex;

    PAGED_CODE();

    FuncEntry(DMF_TRACE);

    moduleContext = DMF_CONTEXT_GET(DmfModule);

    ntStatus = STATUS_SUCCESS;

    for (threadIndex = 0; threadIndex < THREAD_COUNT; threadIndex++)
    {
        ntStatus = DMF_Thread_Start(moduleContext->DmfModuleThreadPassiveInput[threadIndex]);
        if (!NT_SUCCESS(ntStatus))
        {
            goto Exit;
        }
    }

    for (threadIndex = 0; threadIndex < THREAD_COUNT; threadIndex++)
    {
        DMF_Thread_WorkReady(moduleContext->DmfModuleThreadPassiveInput[threadIndex]);
    }

Exit:
    
    FuncExit(DMF_TRACE, "ntStatus=%!STATUS!", ntStatus);

    return ntStatus;
}
#pragma code_seg()

#pragma code_seg("PAGE")
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
Tests_DeviceInterfaceTarget_StartPassiveOutput(
    _In_ DMFMODULE DmfModule
    )
/*++

Routine Description:

    Starts the threads that send asynchronous data to the manually started
    DMF_DeviceInterfaceTarget Modules.

Arguments:

    DmfModule - DMF_Tests_DeviceInterfaceTarget.

Return Value:

    NTSTATUS

--*/
{
    DMF_CONTEXT_Tests_DeviceInterfaceTarget* moduleContext;
    NTSTATUS ntStatus;
    LONG threadIndex;

    PAGED_CODE();

    FuncEntry(DMF_TRACE);

    moduleContext = DMF_CONTEXT_GET(DmfModule);

    ntStatus = STATUS_SUCCESS;

    for (threadIndex = 0; threadIndex < THREAD_COUNT; threadIndex++)
    {
        ntStatus = DMF_Thread_Start(moduleContext->DmfModuleThreadPassiveOutput[threadIndex]);
        if (!NT_SUCCESS(ntStatus))
        {
            goto Exit;
        }
    }

    for (threadIndex = 0; threadIndex < THREAD_COUNT; threadIndex++)
    {
        DMF_Thread_WorkReady(moduleContext->DmfModuleThreadPassiveOutput[threadIndex]);
    }

Exit:
    
    FuncExit(DMF_TRACE, "ntStatus=%!STATUS!", ntStatus);

    return ntStatus;
}
#pragma code_seg()

#pragma code_seg("PAGE")
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
Tests_DeviceInterfaceTarget_StopPassiveInput(
    _In_ DMFMODULE DmfModule
    )
/*++

Routine Description:

    Stops the threads that send asynchronous data to the manually started
    DMF_DeviceInterfaceTarget Modules.

Arguments:

    DmfModule - DMF_Tests_DeviceInterfaceTarget.

Return Value:

    None

--*/
{
    DMF_CONTEXT_Tests_DeviceInterfaceTarget* moduleContext;
    LONG threadIndex;

    PAGED_CODE();

    FuncEntry(DMF_TRACE);

    moduleContext = DMF_CONTEXT_GET(DmfModule);

    for (threadIndex = 0; threadIndex < THREAD_COUNT; threadIndex++)
    {
        // Interrupt any long sleeps.
        //
        DMF_AlertableSleep_Abort(moduleContext->DmfModuleAlertableSleepPassiveInput[threadIndex],
                                 0);
        // Stop thread.
        //
        DMF_Thread_Stop(moduleContext->DmfModuleThreadPassiveInput[threadIndex]);
    }

    FuncExitVoid(DMF_TRACE);
}
#pragma code_seg()

#pragma code_seg("PAGE")
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
Tests_DeviceInterfaceTarget_StopPassiveOutput(
    _In_ DMFMODULE DmfModule
    )
/*++

Routine Description:

    Stops the threads that send asynchronous data to the manually started
    DMF_DeviceInterfaceTarget Modules.

Arguments:

    DmfModule - DMF_Tests_DeviceInterfaceTarget.

Return Value:

    None

--*/
{
    DMF_CONTEXT_Tests_DeviceInterfaceTarget* moduleContext;
    LONG threadIndex;

    PAGED_CODE();

    FuncEntry(DMF_TRACE);

    moduleContext = DMF_CONTEXT_GET(DmfModule);

    for (threadIndex = 0; threadIndex < THREAD_COUNT; threadIndex++)
    {
        // Interrupt any long sleeps.
        //
        DMF_AlertableSleep_Abort(moduleContext->DmfModuleAlertableSleepPassiveOutput[threadIndex],
                                 0);
        // Stop thread.
        //
        DMF_Thread_Stop(moduleContext->DmfModuleThreadPassiveOutput[threadIndex]);
    }

    FuncExitVoid(DMF_TRACE);
}
#pragma code_seg()

#pragma code_seg("PAGE")
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
Tests_DeviceInterfaceTarget_OnDeviceArrivalNotification_DispatchInput(
    _In_ DMFMODULE DmfModule
    )
/*++

Routine Description:

    Callback function for Device Arrival Notification.
    This function starts the threads that send asynchronous data to automatically started
    DMF_DeviceInterfaceTarget Modules.

Arguments:

    DmfModule - The Child Module from which this callback is called.

Return Value:

    VOID

--*/
{
    NTSTATUS ntStatus;
    DMFMODULE dmfModuleParent;
    DMF_CONTEXT_Tests_DeviceInterfaceTarget* moduleContext;

    PAGED_CODE();

    dmfModuleParent = DMF_ParentModuleGet(DmfModule);
    moduleContext = DMF_CONTEXT_GET(dmfModuleParent);

#if defined(DMF_KERNEL_MODE)
    ntStatus = Tests_DeviceInterfaceTarget_QueryInterface(moduleContext->DmfModuleDeviceInterfaceTargetDispatchInput);
    DmfAssert(NT_SUCCESS(ntStatus));
#endif // defined(DMF_KERNEL_MODE)

    for (LONG threadIndex = 0; threadIndex < THREAD_COUNT; threadIndex++)
    {
        Tests_DeviceInterfaceTarget_THREAD_INDEX_CONTEXT* threadIndexContext;
        WDF_OBJECT_ATTRIBUTES objectAttributes;

        WDF_OBJECT_ATTRIBUTES_INIT(&objectAttributes);
        WDF_OBJECT_ATTRIBUTES_SET_CONTEXT_TYPE(&objectAttributes,
                                               Tests_DeviceInterfaceTarget_THREAD_INDEX_CONTEXT);
        ntStatus = WdfObjectAllocateContext(moduleContext->DmfModuleThreadDispatchInput[threadIndex],
                                            &objectAttributes,
                                            (PVOID*)&threadIndexContext);
        DmfAssert(NT_SUCCESS(ntStatus));
        threadIndexContext->DmfModuleAlertableSleep = moduleContext->DmfModuleAlertableSleepDispatchInput[threadIndex];
        // Reset in case target comes and goes and comes back.
        //
        DMF_AlertableSleep_ResetForReuse(threadIndexContext->DmfModuleAlertableSleep,
                                         0);
    }

    // Start the threads. Streaming is automatically started.
    //
    ntStatus = Tests_DeviceInterfaceTarget_StartDispatchInput(dmfModuleParent);
    DmfAssert(NT_SUCCESS(ntStatus));
}
#pragma code_seg()

#pragma code_seg("PAGE")
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
Tests_DeviceInterfaceTarget_OnDeviceRemovalNotification_DispatchInput(
    _In_ DMFMODULE DmfModule
    )
/*++

Routine Description:

    Callback function for Device Removal Notification.
    This function stops the threads that send asynchronous data to automatically started
    DMF_DeviceInterfaceTarget Modules.

Arguments:

    DmfModule - The Child Module from which this callback is called.

Return Value:

    VOID

--*/
{
    DMFMODULE dmfModuleParent;
    NTSTATUS ntStatus;
    WDFIOTARGET ioTarget;

    PAGED_CODE();

    dmfModuleParent = DMF_ParentModuleGet(DmfModule);

#if defined(DMF_KERNEL_MODE)
    Tests_DeviceInterfaceTarget_DirectInterfaceDecrement(dmfModuleParent);
#endif // defined(DMF_KERNEL_MODE)

    ntStatus = DMF_DeviceInterfaceTarget_Get(DmfModule,
                                             &ioTarget);
    if (NT_SUCCESS(ntStatus))
    {
        WdfIoTargetPurge(ioTarget,
                         WdfIoTargetPurgeIoAndWait);
    }

    // Stop the threads. Streaming is automatically stopped.
    //
    Tests_DeviceInterfaceTarget_StopDispatchInput(dmfModuleParent);
}
#pragma code_seg()

#pragma code_seg("PAGE")
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
Tests_DeviceInterfaceTarget_OnDeviceArrivalNotification_PassiveInput(
    _In_ DMFMODULE DmfModule
    )
/*++

Routine Description:

    Callback function for Device Arrival Notification.
    Manually starts the manual DMF_DeviceInterfaceTarget Module.
    This function starts the threads that send asynchronous data to manually started
    DMF_DeviceInterfaceTarget Modules.

Arguments:

    DmfModule - The Child Module from which this callback is called.

Return Value:

    VOID

--*/
{
    DMFMODULE dmfModuleParent;
    DMF_CONTEXT_Tests_DeviceInterfaceTarget* moduleContext;
    NTSTATUS ntStatus;

    PAGED_CODE();

    ntStatus = STATUS_SUCCESS;
    dmfModuleParent = DMF_ParentModuleGet(DmfModule);
    moduleContext = DMF_CONTEXT_GET(dmfModuleParent);

#if defined(DMF_KERNEL_MODE)
    ntStatus = Tests_DeviceInterfaceTarget_QueryInterface(moduleContext->DmfModuleDeviceInterfaceTargetPassiveInput);
    DmfAssert(NT_SUCCESS(ntStatus));
#endif // defined(DMF_KERNEL_MODE)

#if !defined(TEST_SIMPLE)
    for (LONG threadIndex = 0; threadIndex < THREAD_COUNT; threadIndex++)
    {
        Tests_DeviceInterfaceTarget_THREAD_INDEX_CONTEXT* threadIndexContext;
        WDF_OBJECT_ATTRIBUTES objectAttributes;

        WDF_OBJECT_ATTRIBUTES_INIT(&objectAttributes);
        WDF_OBJECT_ATTRIBUTES_SET_CONTEXT_TYPE(&objectAttributes,
                                               Tests_DeviceInterfaceTarget_THREAD_INDEX_CONTEXT);
        ntStatus = WdfObjectAllocateContext(moduleContext->DmfModuleThreadPassiveInput[threadIndex],
                                            &objectAttributes,
                                            (PVOID*)&threadIndexContext);
        DmfAssert(NT_SUCCESS(ntStatus));
        threadIndexContext->DmfModuleAlertableSleep = moduleContext->DmfModuleAlertableSleepPassiveInput[threadIndex];
        // Reset in case target comes and goes and comes back.
        //
        DMF_AlertableSleep_ResetForReuse(threadIndexContext->DmfModuleAlertableSleep,
                                         0);
    }

#if defined(USE_STREAMING)
    // Start streaming.
    //
    ntStatus = DMF_DeviceInterfaceTarget_StreamStart(DmfModule);
#else
    ntStatus = STATUS_SUCCESS;
#endif
    if (NT_SUCCESS(ntStatus))
    {
        // Start threads.
        //
        Tests_DeviceInterfaceTarget_StartPassiveInput(dmfModuleParent);
    }
    DmfAssert(NT_SUCCESS(ntStatus));
#endif
}
#pragma code_seg()

#pragma code_seg("PAGE")
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
Tests_DeviceInterfaceTarget_OnDeviceArrivalNotification_PassiveOutput(
    _In_ DMFMODULE DmfModule
    )
/*++

Routine Description:

    Callback function for Device Arrival Notification.
    Manually starts the manual DMF_DeviceInterfaceTarget Module.
    This function starts the threads that send asynchronous data to manually started
    DMF_DeviceInterfaceTarget Modules.

Arguments:

    DmfModule - The Child Module from which this callback is called.

Return Value:

    VOID

--*/
{
    NTSTATUS ntStatus;
    DMFMODULE dmfModuleParent;
    DMF_CONTEXT_Tests_DeviceInterfaceTarget* moduleContext;
    PAGED_CODE();

    dmfModuleParent = DMF_ParentModuleGet(DmfModule);
    moduleContext = DMF_CONTEXT_GET(dmfModuleParent);

#if defined(DMF_KERNEL_MODE)
    ntStatus = Tests_DeviceInterfaceTarget_QueryInterface(moduleContext->DmfModuleDeviceInterfaceTargetPassiveOutput);
    DmfAssert(NT_SUCCESS(ntStatus));
#endif // defined(DMF_KERNEL_MODE)

    for (LONG threadIndex = 0; threadIndex < THREAD_COUNT; threadIndex++)
    {
        Tests_DeviceInterfaceTarget_THREAD_INDEX_CONTEXT* threadIndexContext;
        WDF_OBJECT_ATTRIBUTES objectAttributes;

        WDF_OBJECT_ATTRIBUTES_INIT(&objectAttributes);
        WDF_OBJECT_ATTRIBUTES_SET_CONTEXT_TYPE(&objectAttributes,
                                               Tests_DeviceInterfaceTarget_THREAD_INDEX_CONTEXT);
        ntStatus = WdfObjectAllocateContext(moduleContext->DmfModuleThreadPassiveOutput[threadIndex],
                                            &objectAttributes,
                                            (PVOID*)&threadIndexContext);
        DmfAssert(NT_SUCCESS(ntStatus));
        threadIndexContext->DmfModuleAlertableSleep = moduleContext->DmfModuleAlertableSleepPassiveOutput[threadIndex];
        // Reset in case target comes and goes and comes back.
        //
        DMF_AlertableSleep_ResetForReuse(threadIndexContext->DmfModuleAlertableSleep,
                                         0);
    }

#if defined(USE_STREAMING)
    // Start streaming.
    //
    ntStatus = DMF_DeviceInterfaceTarget_StreamStart(DmfModule);
#else
    ntStatus = STATUS_SUCCESS;
#endif
    if (NT_SUCCESS(ntStatus))
    {
        // Start threads.
        //
        Tests_DeviceInterfaceTarget_StartPassiveOutput(dmfModuleParent);
    }
}
#pragma code_seg()

#pragma code_seg("PAGE")
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
Tests_DeviceInterfaceTarget_OnDeviceRemovalNotification_PassiveInput(
    _In_ DMFMODULE DmfModule
    )
/*++

Routine Description:

    Callback function for Device Removal Notification.
    Manually stops the manual DMF_DeviceInterfaceTarget Module.
    This function stops the threads that send asynchronous data to manually started
    DMF_DeviceInterfaceTarget Modules.

Arguments:

    DmfModule - The Child Module from which this callback is called.

Return Value:

    VOID

--*/
{
    DMFMODULE dmfModuleParent;
#if !defined(TEST_SIMPLE)
    NTSTATUS ntStatus;
#endif

    PAGED_CODE();

    dmfModuleParent = DMF_ParentModuleGet(DmfModule);

#if defined(DMF_KERNEL_MODE)
    Tests_DeviceInterfaceTarget_DirectInterfaceDecrement(dmfModuleParent);
#endif // defined(DMF_KERNEL_MODE)

#if !defined(TEST_SIMPLE)
    WDFIOTARGET ioTarget;

    ntStatus = DMF_DeviceInterfaceTarget_Get(DmfModule,
                                             &ioTarget);
    if (NT_SUCCESS(ntStatus))
    {
        WdfIoTargetPurge(ioTarget,
                         WdfIoTargetPurgeIoAndWait);
    }

#if defined(USE_STREAMING)
    // Stop streaming.
    //
    DMF_DeviceInterfaceTarget_StreamStop(DmfModule);
#endif
    // Stop threads.
    //
    Tests_DeviceInterfaceTarget_StopPassiveInput(dmfModuleParent);
#endif
}
#pragma code_seg()

#pragma code_seg("PAGE")
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
Tests_DeviceInterfaceTarget_OnDeviceRemovalNotification_PassiveOutput(
    _In_ DMFMODULE DmfModule
    )
/*++

Routine Description:

    Callback function for Device Removal Notification.
    Manually stops the manual DMF_DeviceInterfaceTarget Module.
    This function stops the threads that send asynchronous data to manually started
    DMF_DeviceInterfaceTarget Modules.

Arguments:

    DmfModule - The Child Module from which this callback is called.

Return Value:

    VOID

--*/
{
    DMFMODULE dmfModuleParent;
    NTSTATUS ntStatus;
    WDFIOTARGET ioTarget;

    PAGED_CODE();

    dmfModuleParent = DMF_ParentModuleGet(DmfModule);

#if defined(DMF_KERNEL_MODE)
    Tests_DeviceInterfaceTarget_DirectInterfaceDecrement(dmfModuleParent);
#endif // defined(DMF_KERNEL_MODE)

    ntStatus = DMF_DeviceInterfaceTarget_Get(DmfModule,
                                             &ioTarget);
    if (NT_SUCCESS(ntStatus))
    {
        WdfIoTargetPurge(ioTarget,
                         WdfIoTargetPurgeIoAndWait);
    }

#if defined(USE_STREAMING)
    // Stop streaming.
    //
    DMF_DeviceInterfaceTarget_StreamStop(DmfModule);
#endif
    // Stop threads.
    //
    Tests_DeviceInterfaceTarget_StopPassiveOutput(dmfModuleParent);
}
#pragma code_seg()

#pragma code_seg("PAGE")
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
Tests_DeviceInterfaceTarget_StartPassiveNonContinuous(
    _In_ DMFMODULE DmfModule
    )
/*++

Routine Description:

    Starts the threads that send asynchronous data to the manually started
    DMF_DeviceInterfaceTarget Modules.

Arguments:

    DmfModule - DMF_Tests_DeviceInterfaceTarget.

Return Value:

    NTSTATUS

--*/
{
    DMF_CONTEXT_Tests_DeviceInterfaceTarget* moduleContext;
    NTSTATUS ntStatus;
    LONG threadIndex;

    PAGED_CODE();

    FuncEntry(DMF_TRACE);

    moduleContext = DMF_CONTEXT_GET(DmfModule);

    ntStatus = STATUS_SUCCESS;

    for (threadIndex = 0; threadIndex < THREAD_COUNT; threadIndex++)
    {
        ntStatus = DMF_Thread_Start(moduleContext->DmfModuleThreadPassiveNonContinuous[threadIndex]);
        if (!NT_SUCCESS(ntStatus))
        {
            goto Exit;
        }
    }

    for (threadIndex = 0; threadIndex < THREAD_COUNT; threadIndex++)
    {
        DMF_Thread_WorkReady(moduleContext->DmfModuleThreadPassiveNonContinuous[threadIndex]);
    }

Exit:
    
    FuncExit(DMF_TRACE, "ntStatus=%!STATUS!", ntStatus);

    return ntStatus;
}
#pragma code_seg()

#pragma code_seg("PAGE")
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
Tests_DeviceInterfaceTarget_StopPassiveNonContinuous(
    _In_ DMFMODULE DmfModule
    )
/*++

Routine Description:

    Stops the threads that send asynchronous data to the manually started
    DMF_DeviceInterfaceTarget Modules.

Arguments:

    DmfModule - DMF_Tests_DeviceInterfaceTarget.

Return Value:

    None

--*/
{
    DMF_CONTEXT_Tests_DeviceInterfaceTarget* moduleContext;
    LONG threadIndex;

    PAGED_CODE();

    FuncEntry(DMF_TRACE);

    moduleContext = DMF_CONTEXT_GET(DmfModule);

    for (threadIndex = 0; threadIndex < THREAD_COUNT; threadIndex++)
    {
        // Interrupt any long sleeps.
        //
        DMF_AlertableSleep_Abort(moduleContext->DmfModuleAlertableSleepPassiveNonContinuous[threadIndex],
                                 0);
        // Stop thread.
        //
        DMF_Thread_Stop(moduleContext->DmfModuleThreadPassiveNonContinuous[threadIndex]);
    }

    FuncExitVoid(DMF_TRACE);
}
#pragma code_seg()

#pragma code_seg("PAGE")
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
Tests_DeviceInterfaceTarget_OnDeviceArrivalNotification_PassiveNonContinuous(
    _In_ DMFMODULE DmfModule
    )
/*++

Routine Description:

    Callback function for Device Arrival Notification.
    Manually starts the manual DMF_DeviceInterfaceTarget Module.
    This function starts the threads that send asynchronous data to manually started
    DMF_DeviceInterfaceTarget Modules.

Arguments:

    DmfModule - The Child Module from which this callback is called.

Return Value:

    VOID

--*/
{
    NTSTATUS ntStatus;
    DMFMODULE dmfModuleParent;
    DMF_CONTEXT_Tests_DeviceInterfaceTarget* moduleContext;
    PAGED_CODE();

    dmfModuleParent = DMF_ParentModuleGet(DmfModule);
    moduleContext = DMF_CONTEXT_GET(dmfModuleParent);

#if defined(DMF_KERNEL_MODE)
    ntStatus = Tests_DeviceInterfaceTarget_QueryInterface(moduleContext->DmfModuleDeviceInterfaceTargetPassiveNonContinuous);
    DmfAssert(NT_SUCCESS(ntStatus));
#endif // defined(DMF_KERNEL_MODE)

    for (LONG threadIndex = 0; threadIndex < THREAD_COUNT; threadIndex++)
    {
        Tests_DeviceInterfaceTarget_THREAD_INDEX_CONTEXT* threadIndexContext;
        WDF_OBJECT_ATTRIBUTES objectAttributes;

        WDF_OBJECT_ATTRIBUTES_INIT(&objectAttributes);
        WDF_OBJECT_ATTRIBUTES_SET_CONTEXT_TYPE(&objectAttributes,
                                               Tests_DeviceInterfaceTarget_THREAD_INDEX_CONTEXT);
        ntStatus = WdfObjectAllocateContext(moduleContext->DmfModuleThreadPassiveNonContinuous[threadIndex],
                                            &objectAttributes,
                                            (PVOID*)&threadIndexContext);
        DmfAssert(NT_SUCCESS(ntStatus));
        threadIndexContext->DmfModuleAlertableSleep = moduleContext->DmfModuleAlertableSleepPassiveNonContinuous[threadIndex];
        // Reset in case target comes and goes and comes back.
        //
        DMF_AlertableSleep_ResetForReuse(threadIndexContext->DmfModuleAlertableSleep,
                                         0);
    }

    // Start threads.
    //
    Tests_DeviceInterfaceTarget_StartPassiveNonContinuous(dmfModuleParent);
}
#pragma code_seg()

#pragma code_seg("PAGE")
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
Tests_DeviceInterfaceTarget_OnDeviceRemovalNotification_PassiveNonContinuous(
    _In_ DMFMODULE DmfModule
    )
/*++

Routine Description:

    Callback function for Device Removal Notification.
    Manually stops the manual DMF_DeviceInterfaceTarget Module.
    This function stops the threads that send asynchronous data to manually started
    DMF_DeviceInterfaceTarget Modules.

Arguments:

    DmfModule - The Child Module from which this callback is called.

Return Value:

    VOID

--*/
{
    DMFMODULE dmfModuleParent;
#if !defined(TEST_SIMPLE)
    NTSTATUS ntStatus;
#endif

    PAGED_CODE();

    dmfModuleParent = DMF_ParentModuleGet(DmfModule);

#if defined(DMF_KERNEL_MODE)
    Tests_DeviceInterfaceTarget_DirectInterfaceDecrement(dmfModuleParent);
#endif // defined(DMF_KERNEL_MODE)

#if !defined(TEST_SIMPLE)
    WDFIOTARGET ioTarget;

    ntStatus = DMF_DeviceInterfaceTarget_Get(DmfModule,
                                             &ioTarget);
    if (NT_SUCCESS(ntStatus))
    {
        WdfIoTargetPurge(ioTarget,
                         WdfIoTargetPurgeIoAndWait);
    }

    // Stop threads.
    //
    Tests_DeviceInterfaceTarget_StopPassiveNonContinuous(dmfModuleParent);
#endif
}
#pragma code_seg()

#pragma code_seg("PAGE")
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
Tests_DeviceInterfaceTarget_StartDispatchNonContinuous(
    _In_ DMFMODULE DmfModule
    )
/*++

Routine Description:

    Starts the threads that send asynchronous data to the manually started
    DMF_DeviceInterfaceTarget Modules.

Arguments:

    DmfModule - DMF_Tests_DeviceInterfaceTarget.

Return Value:

    NTSTATUS

--*/
{
    DMF_CONTEXT_Tests_DeviceInterfaceTarget* moduleContext;
    NTSTATUS ntStatus;
    LONG threadIndex;

    PAGED_CODE();

    FuncEntry(DMF_TRACE);

    moduleContext = DMF_CONTEXT_GET(DmfModule);

    ntStatus = STATUS_SUCCESS;

    for (threadIndex = 0; threadIndex < THREAD_COUNT; threadIndex++)
    {
        ntStatus = DMF_Thread_Start(moduleContext->DmfModuleThreadDispatchNonContinuous[threadIndex]);
        if (!NT_SUCCESS(ntStatus))
        {
            goto Exit;
        }
    }

    for (threadIndex = 0; threadIndex < THREAD_COUNT; threadIndex++)
    {
        DMF_Thread_WorkReady(moduleContext->DmfModuleThreadDispatchNonContinuous[threadIndex]);
    }

Exit:
    
    FuncExit(DMF_TRACE, "ntStatus=%!STATUS!", ntStatus);

    return ntStatus;
}
#pragma code_seg()

#pragma code_seg("PAGE")
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
Tests_DeviceInterfaceTarget_StopDispatchNonContinuous(
    _In_ DMFMODULE DmfModule
    )
/*++

Routine Description:

    Stops the threads that send asynchronous data to the manually started
    DMF_DeviceInterfaceTarget Modules.

Arguments:

    DmfModule - DMF_Tests_DeviceInterfaceTarget.

Return Value:

    None

--*/
{
    DMF_CONTEXT_Tests_DeviceInterfaceTarget* moduleContext;
    LONG threadIndex;

    PAGED_CODE();

    FuncEntry(DMF_TRACE);

    moduleContext = DMF_CONTEXT_GET(DmfModule);

    for (threadIndex = 0; threadIndex < THREAD_COUNT; threadIndex++)
    {
        // Interrupt any long sleeps.
        //
        DMF_AlertableSleep_Abort(moduleContext->DmfModuleAlertableSleepDispatchNonContinuous[threadIndex],
                                 0);
        // Stop thread.
        //
        DMF_Thread_Stop(moduleContext->DmfModuleThreadDispatchNonContinuous[threadIndex]);
    }

    FuncExitVoid(DMF_TRACE);
}
#pragma code_seg()

#pragma code_seg("PAGE")
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
Tests_DeviceInterfaceTarget_OnDeviceArrivalNotification_DispatchNonContinuous(
    _In_ DMFMODULE DmfModule
    )
/*++

Routine Description:

    Callback function for Device Arrival Notification.
    Manually starts the manual DMF_DeviceInterfaceTarget Module.
    This function starts the threads that send asynchronous data to manually started
    DMF_DeviceInterfaceTarget Modules.

Arguments:

    DmfModule - The Child Module from which this callback is called.

Return Value:

    VOID

--*/
{
    NTSTATUS ntStatus;
    DMFMODULE dmfModuleParent;
    DMF_CONTEXT_Tests_DeviceInterfaceTarget* moduleContext;
    PAGED_CODE();

    dmfModuleParent = DMF_ParentModuleGet(DmfModule);
    moduleContext = DMF_CONTEXT_GET(dmfModuleParent);

#if defined(DMF_KERNEL_MODE)
    ntStatus = Tests_DeviceInterfaceTarget_QueryInterface(moduleContext->DmfModuleDeviceInterfaceTargetDispatchNonContinuous);
    DmfAssert(NT_SUCCESS(ntStatus));
#endif // defined(DMF_KERNEL_MODE)

    for (LONG threadIndex = 0; threadIndex < THREAD_COUNT; threadIndex++)
    {
        Tests_DeviceInterfaceTarget_THREAD_INDEX_CONTEXT* threadIndexContext;
        WDF_OBJECT_ATTRIBUTES objectAttributes;

        WDF_OBJECT_ATTRIBUTES_INIT(&objectAttributes);
        WDF_OBJECT_ATTRIBUTES_SET_CONTEXT_TYPE(&objectAttributes,
                                               Tests_DeviceInterfaceTarget_THREAD_INDEX_CONTEXT);
        ntStatus = WdfObjectAllocateContext(moduleContext->DmfModuleThreadDispatchNonContinuous[threadIndex],
                                            &objectAttributes,
                                            (PVOID*)&threadIndexContext);
        DmfAssert(NT_SUCCESS(ntStatus));
        threadIndexContext->DmfModuleAlertableSleep = moduleContext->DmfModuleAlertableSleepDispatchNonContinuous[threadIndex];
        // Reset in case target comes and goes and comes back.
        //
        DMF_AlertableSleep_ResetForReuse(threadIndexContext->DmfModuleAlertableSleep,
                                         0);
    }

    // Start threads.
    //
    Tests_DeviceInterfaceTarget_StartDispatchNonContinuous(dmfModuleParent);
}
#pragma code_seg()

#pragma code_seg("PAGE")
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
Tests_DeviceInterfaceTarget_OnDeviceRemovalNotification_DispatchNonContinuous(
    _In_ DMFMODULE DmfModule
    )
/*++

Routine Description:

    Callback function for Device Removal Notification.
    Manually stops the manual DMF_DeviceInterfaceTarget Module.
    This function stops the threads that send asynchronous data to manually started
    DMF_DeviceInterfaceTarget Modules.

Arguments:

    DmfModule - The Child Module from which this callback is called.

Return Value:

    VOID

--*/
{
    DMFMODULE dmfModuleParent;
#if !defined(TEST_SIMPLE)
    NTSTATUS ntStatus;
#endif

    PAGED_CODE();

    dmfModuleParent = DMF_ParentModuleGet(DmfModule);

#if defined(DMF_KERNEL_MODE)
    Tests_DeviceInterfaceTarget_DirectInterfaceDecrement(dmfModuleParent);
#endif // defined(DMF_KERNEL_MODE)

#if !defined(TEST_SIMPLE)
    WDFIOTARGET ioTarget;

    ntStatus = DMF_DeviceInterfaceTarget_Get(DmfModule,
                                             &ioTarget);
    if (NT_SUCCESS(ntStatus))
    {
        WdfIoTargetPurge(ioTarget,
                         WdfIoTargetPurgeIoAndWait);
    }

    // Stop threads.
    //
    Tests_DeviceInterfaceTarget_StopDispatchNonContinuous(dmfModuleParent);
#endif
}
#pragma code_seg()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// WDF Module Callbacks
///////////////////////////////////////////////////////////////////////////////////////////////////////
//

///////////////////////////////////////////////////////////////////////////////////////////////////////
// DMF Module Callbacks
///////////////////////////////////////////////////////////////////////////////////////////////////////
//

#pragma code_seg("PAGE")
_Function_class_(DMF_ChildModulesAdd)
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
DMF_Tests_DeviceInterfaceTarget_ChildModulesAdd(
    _In_ DMFMODULE DmfModule,
    _In_ DMF_MODULE_ATTRIBUTES* DmfParentModuleAttributes,
    _In_ PDMFMODULE_INIT DmfModuleInit
    )
/*++

Routine Description:

    Configure and add the required Child Modules to the given Parent Module.

Arguments:

    DmfModule - The given Parent Module.
    DmfParentModuleAttributes - Pointer to the parent DMF_MODULE_ATTRIBUTES structure.
    DmfModuleInit - Opaque structure to be passed to DMF_DmfModuleAdd.

Return Value:

    None

--*/
{
    DMF_MODULE_ATTRIBUTES moduleAttributes;
    DMF_CONTEXT_Tests_DeviceInterfaceTarget* moduleContext;
    DMF_CONFIG_Thread moduleConfigThread;
    DMF_CONFIG_DeviceInterfaceTarget moduleConfigDeviceInterfaceTarget;
    DMF_MODULE_EVENT_CALLBACKS moduleEventCallbacks;
    DMF_CONFIG_AlertableSleep moduleConfigAlertableSleep;

    UNREFERENCED_PARAMETER(DmfParentModuleAttributes);

    PAGED_CODE();

    FuncEntry(DMF_TRACE);

    moduleContext = DMF_CONTEXT_GET(DmfModule);

    // General purpose buffers for asynchronous transactions.
    //
    DMF_CONFIG_BufferPool moduleConfigBufferPool;
    DMF_CONFIG_BufferPool_AND_ATTRIBUTES_INIT(&moduleConfigBufferPool,
                                              &moduleAttributes);
    moduleConfigBufferPool.BufferPoolMode = BufferPool_Mode_Source;
    moduleConfigBufferPool.Mode.SourceSettings.BufferCount = 10;
    moduleConfigBufferPool.Mode.SourceSettings.BufferSize = sizeof(Tests_IoctlHandler_Sleep);
    moduleConfigBufferPool.Mode.SourceSettings.EnableLookAside = TRUE;
    moduleConfigBufferPool.Mode.SourceSettings.PoolType = NonPagedPoolNx;
    DMF_DmfModuleAdd(DmfModuleInit,
                     &moduleAttributes,
                     WDF_NO_OBJECT_ATTRIBUTES,
                     &moduleContext->DmfModuleBufferPool);

    // Thread
    // ------
    //
    for (LONG threadIndex = 0; threadIndex < THREAD_COUNT; threadIndex++)
    {
        DMF_CONFIG_Thread_AND_ATTRIBUTES_INIT(&moduleConfigThread,
                                              &moduleAttributes);
        moduleConfigThread.ThreadControlType = ThreadControlType_DmfControl;
        moduleConfigThread.ThreadControl.DmfControl.EvtThreadWork = Tests_DeviceInterfaceTarget_WorkThreadDispatchInput;
        DMF_DmfModuleAdd(DmfModuleInit,
                         &moduleAttributes,
                         WDF_NO_OBJECT_ATTRIBUTES,
                         &moduleContext->DmfModuleThreadDispatchInput[threadIndex]);

#if !defined(TEST_SIMPLE)
        DMF_CONFIG_Thread_AND_ATTRIBUTES_INIT(&moduleConfigThread,
                                              &moduleAttributes);
        moduleConfigThread.ThreadControlType = ThreadControlType_DmfControl;
        moduleConfigThread.ThreadControl.DmfControl.EvtThreadWork = Tests_DeviceInterfaceTarget_WorkThreadPassiveInput;
        DMF_DmfModuleAdd(DmfModuleInit,
                         &moduleAttributes,
                         WDF_NO_OBJECT_ATTRIBUTES,
                         &moduleContext->DmfModuleThreadPassiveInput[threadIndex]);

        DMF_CONFIG_Thread_AND_ATTRIBUTES_INIT(&moduleConfigThread,
                                              &moduleAttributes);
        moduleConfigThread.ThreadControlType = ThreadControlType_DmfControl;
        moduleConfigThread.ThreadControl.DmfControl.EvtThreadWork = Tests_DeviceInterfaceTarget_WorkThreadPassiveOutput;
        DMF_DmfModuleAdd(DmfModuleInit,
                         &moduleAttributes,
                         WDF_NO_OBJECT_ATTRIBUTES,
                         &moduleContext->DmfModuleThreadPassiveOutput[threadIndex]);

        DMF_CONFIG_Thread_AND_ATTRIBUTES_INIT(&moduleConfigThread,
                                              &moduleAttributes);
        moduleConfigThread.ThreadControlType = ThreadControlType_DmfControl;
        moduleConfigThread.ThreadControl.DmfControl.EvtThreadWork = Tests_DeviceInterfaceTarget_WorkThreadPassiveNonContinuous;
        DMF_DmfModuleAdd(DmfModuleInit,
                         &moduleAttributes,
                         WDF_NO_OBJECT_ATTRIBUTES,
                         &moduleContext->DmfModuleThreadPassiveNonContinuous[threadIndex]);

        DMF_CONFIG_Thread_AND_ATTRIBUTES_INIT(&moduleConfigThread,
                                              &moduleAttributes);
        moduleConfigThread.ThreadControlType = ThreadControlType_DmfControl;
        moduleConfigThread.ThreadControl.DmfControl.EvtThreadWork = Tests_DeviceInterfaceTarget_WorkThreadDispatchNonContinuous;
        DMF_DmfModuleAdd(DmfModuleInit,
                         &moduleAttributes,
                         WDF_NO_OBJECT_ATTRIBUTES,
                         &moduleContext->DmfModuleThreadDispatchNonContinuous[threadIndex]);
#endif

        // AlertableSleep Auto
        // -------------------
        //
        DMF_CONFIG_AlertableSleep_AND_ATTRIBUTES_INIT(&moduleConfigAlertableSleep,
                                                      &moduleAttributes);
        moduleConfigAlertableSleep.EventCount = 1;
        moduleAttributes.ClientModuleInstanceName = "AlertableSleep.Auto";
        DMF_DmfModuleAdd(DmfModuleInit,
                            &moduleAttributes,
                            WDF_NO_OBJECT_ATTRIBUTES,
                            &moduleContext->DmfModuleAlertableSleepDispatchInput[threadIndex]);

        // AlertableSleep Manual (Input)
        // -----------------------------
        //
        DMF_CONFIG_AlertableSleep_AND_ATTRIBUTES_INIT(&moduleConfigAlertableSleep,
                                                      &moduleAttributes);
        moduleConfigAlertableSleep.EventCount = 1;
        moduleAttributes.ClientModuleInstanceName = "AlertableSleep.ManualInput";
        DMF_DmfModuleAdd(DmfModuleInit,
                            &moduleAttributes,
                            WDF_NO_OBJECT_ATTRIBUTES,
                            &moduleContext->DmfModuleAlertableSleepPassiveInput[threadIndex]);

        // AlertableSleep Manual (Output)
        // ------------------------------
        //
        DMF_CONFIG_AlertableSleep_AND_ATTRIBUTES_INIT(&moduleConfigAlertableSleep,
                                                      &moduleAttributes);
        moduleConfigAlertableSleep.EventCount = 1;
        moduleAttributes.ClientModuleInstanceName = "AlertableSleep.ManualOutput";
        DMF_DmfModuleAdd(DmfModuleInit,
                            &moduleAttributes,
                            WDF_NO_OBJECT_ATTRIBUTES,
                            &moduleContext->DmfModuleAlertableSleepPassiveOutput[threadIndex]);

        // AlertableSleep Passive NonContinuous
        // ------------------------------------
        //
        DMF_CONFIG_AlertableSleep_AND_ATTRIBUTES_INIT(&moduleConfigAlertableSleep,
                                                      &moduleAttributes);
        moduleConfigAlertableSleep.EventCount = 1;
        moduleAttributes.ClientModuleInstanceName = "AlertableSleep.PassiveNonContinuous";
        DMF_DmfModuleAdd(DmfModuleInit,
                            &moduleAttributes,
                            WDF_NO_OBJECT_ATTRIBUTES,
                            &moduleContext->DmfModuleAlertableSleepPassiveNonContinuous[threadIndex]);

        // AlertableSleep Dispatch NonContinuous
        // ------------------------------------
        //
        DMF_CONFIG_AlertableSleep_AND_ATTRIBUTES_INIT(&moduleConfigAlertableSleep,
                                                      &moduleAttributes);
        moduleConfigAlertableSleep.EventCount = 1;
        moduleAttributes.ClientModuleInstanceName = "AlertableSleep.DispatchNonContinuous";
        DMF_DmfModuleAdd(DmfModuleInit,
                            &moduleAttributes,
                            WDF_NO_OBJECT_ATTRIBUTES,
                            &moduleContext->DmfModuleAlertableSleepDispatchNonContinuous[threadIndex]);
    }

    // DeviceInterfaceTarget (DISPATCH_LEVEL)
    // Processes Input Buffers.
    //
    DMF_CONFIG_DeviceInterfaceTarget_AND_ATTRIBUTES_INIT(&moduleConfigDeviceInterfaceTarget,
                                                         &moduleAttributes);
    moduleConfigDeviceInterfaceTarget.DeviceInterfaceTargetGuid = GUID_DEVINTERFACE_Tests_IoctlHandler;
#if !defined(TEST_SIMPLE)
    moduleConfigDeviceInterfaceTarget.ContinuousRequestTargetModuleConfig.BufferCountInput = 1;
    moduleConfigDeviceInterfaceTarget.ContinuousRequestTargetModuleConfig.BufferCountOutput = 1;
    moduleConfigDeviceInterfaceTarget.ContinuousRequestTargetModuleConfig.BufferInputSize = sizeof(Tests_IoctlHandler_Sleep);
    moduleConfigDeviceInterfaceTarget.ContinuousRequestTargetModuleConfig.BufferOutputSize = sizeof(Tests_IoctlHandler_Sleep);
    moduleConfigDeviceInterfaceTarget.ContinuousRequestTargetModuleConfig.ContinuousRequestCount = 1;
    moduleConfigDeviceInterfaceTarget.ContinuousRequestTargetModuleConfig.PoolTypeInput = NonPagedPoolNx;
    moduleConfigDeviceInterfaceTarget.ContinuousRequestTargetModuleConfig.PurgeAndStartTargetInD0Callbacks = FALSE;
    moduleConfigDeviceInterfaceTarget.ContinuousRequestTargetModuleConfig.ContinuousRequestTargetIoctl = IOCTL_Tests_IoctlHandler_SLEEP;
    moduleConfigDeviceInterfaceTarget.ContinuousRequestTargetModuleConfig.EvtContinuousRequestTargetBufferInput = Tests_DeviceInterfaceTarget_BufferInput;
    moduleConfigDeviceInterfaceTarget.ContinuousRequestTargetModuleConfig.RequestType = ContinuousRequestTarget_RequestType_Ioctl;
    moduleConfigDeviceInterfaceTarget.ContinuousRequestTargetModuleConfig.ContinuousRequestTargetMode = ContinuousRequestTarget_Mode_Automatic;
#endif
    DMF_MODULE_ATTRIBUTES_EVENT_CALLBACKS_INIT(&moduleAttributes,
                                               &moduleEventCallbacks);
    moduleEventCallbacks.EvtModuleOnDeviceNotificationPostOpen = Tests_DeviceInterfaceTarget_OnDeviceArrivalNotification_DispatchInput;
    moduleEventCallbacks.EvtModuleOnDeviceNotificationPreClose = Tests_DeviceInterfaceTarget_OnDeviceRemovalNotification_DispatchInput;
    moduleConfigDeviceInterfaceTarget.EvtDeviceInterfaceTargetOnStateChangeEx = Tests_DeviceInterfaceTarget_OnStateChange;
    DMF_DmfModuleAdd(DmfModuleInit,
                     &moduleAttributes,
                     WDF_NO_OBJECT_ATTRIBUTES,
                     &moduleContext->DmfModuleDeviceInterfaceTargetDispatchInput);

    // DeviceInterfaceTarget (PASSIVE_LEVEL)
    // Processes Input Buffers.
    //
    DMF_CONFIG_DeviceInterfaceTarget_AND_ATTRIBUTES_INIT(&moduleConfigDeviceInterfaceTarget,
                                                         &moduleAttributes);
    moduleConfigDeviceInterfaceTarget.DeviceInterfaceTargetGuid = GUID_DEVINTERFACE_Tests_IoctlHandler;
#if !defined(TEST_SIMPLE)
    moduleConfigDeviceInterfaceTarget.ContinuousRequestTargetModuleConfig.BufferCountInput = 1;
    moduleConfigDeviceInterfaceTarget.ContinuousRequestTargetModuleConfig.BufferCountOutput = 1;
    moduleConfigDeviceInterfaceTarget.ContinuousRequestTargetModuleConfig.BufferInputSize = sizeof(Tests_IoctlHandler_Sleep);
    moduleConfigDeviceInterfaceTarget.ContinuousRequestTargetModuleConfig.BufferOutputSize = sizeof(Tests_IoctlHandler_Sleep);
    moduleConfigDeviceInterfaceTarget.ContinuousRequestTargetModuleConfig.ContinuousRequestCount = 1;
    moduleConfigDeviceInterfaceTarget.ContinuousRequestTargetModuleConfig.PoolTypeInput = NonPagedPoolNx;
    moduleConfigDeviceInterfaceTarget.ContinuousRequestTargetModuleConfig.PurgeAndStartTargetInD0Callbacks = FALSE;
    moduleConfigDeviceInterfaceTarget.ContinuousRequestTargetModuleConfig.ContinuousRequestTargetIoctl = IOCTL_Tests_IoctlHandler_SLEEP;
    moduleConfigDeviceInterfaceTarget.ContinuousRequestTargetModuleConfig.EvtContinuousRequestTargetBufferInput = Tests_DeviceInterfaceTarget_BufferInput;
    moduleConfigDeviceInterfaceTarget.ContinuousRequestTargetModuleConfig.RequestType = ContinuousRequestTarget_RequestType_Ioctl;
    moduleConfigDeviceInterfaceTarget.ContinuousRequestTargetModuleConfig.ContinuousRequestTargetMode = ContinuousRequestTarget_Mode_Manual;
#endif
    DMF_MODULE_ATTRIBUTES_EVENT_CALLBACKS_INIT(&moduleAttributes,
                                               &moduleEventCallbacks);
    moduleEventCallbacks.EvtModuleOnDeviceNotificationPostOpen = Tests_DeviceInterfaceTarget_OnDeviceArrivalNotification_PassiveInput;
    moduleEventCallbacks.EvtModuleOnDeviceNotificationPreClose = Tests_DeviceInterfaceTarget_OnDeviceRemovalNotification_PassiveInput;
    moduleConfigDeviceInterfaceTarget.EvtDeviceInterfaceTargetOnStateChangeEx = Tests_DeviceInterfaceTarget_OnStateChange;
    moduleAttributes.PassiveLevel = TRUE;
    moduleAttributes.ClientCallbacks = &moduleEventCallbacks;
    DMF_DmfModuleAdd(DmfModuleInit,
                     &moduleAttributes,
                     WDF_NO_OBJECT_ATTRIBUTES,
                     &moduleContext->DmfModuleDeviceInterfaceTargetPassiveInput);

#if !defined(TEST_SIMPLE)

    // DeviceInterfaceTarget (PASSIVE_LEVEL)
    // Processes Output Buffers.
    //
    DMF_CONFIG_DeviceInterfaceTarget_AND_ATTRIBUTES_INIT(&moduleConfigDeviceInterfaceTarget,
                                                         &moduleAttributes);
    moduleConfigDeviceInterfaceTarget.DeviceInterfaceTargetGuid = GUID_DEVINTERFACE_Tests_IoctlHandler;
    moduleConfigDeviceInterfaceTarget.ContinuousRequestTargetModuleConfig.BufferCountOutput = NUMBER_OF_CONTINUOUS_REQUESTS;
    moduleConfigDeviceInterfaceTarget.ContinuousRequestTargetModuleConfig.BufferOutputSize = sizeof(DWORD);
    moduleConfigDeviceInterfaceTarget.ContinuousRequestTargetModuleConfig.ContinuousRequestCount = NUMBER_OF_CONTINUOUS_REQUESTS;
    moduleConfigDeviceInterfaceTarget.ContinuousRequestTargetModuleConfig.PoolTypeOutput = NonPagedPoolNx;
    moduleConfigDeviceInterfaceTarget.ContinuousRequestTargetModuleConfig.PurgeAndStartTargetInD0Callbacks = FALSE;
    moduleConfigDeviceInterfaceTarget.ContinuousRequestTargetModuleConfig.ContinuousRequestTargetIoctl = IOCTL_Tests_IoctlHandler_ZEROBUFFER;
    moduleConfigDeviceInterfaceTarget.ContinuousRequestTargetModuleConfig.EvtContinuousRequestTargetBufferOutput = Tests_DeviceInterfaceTarget_BufferOutput;
    moduleConfigDeviceInterfaceTarget.ContinuousRequestTargetModuleConfig.RequestType = ContinuousRequestTarget_RequestType_Ioctl;
    moduleConfigDeviceInterfaceTarget.ContinuousRequestTargetModuleConfig.ContinuousRequestTargetMode = ContinuousRequestTarget_Mode_Manual;
    DMF_MODULE_ATTRIBUTES_EVENT_CALLBACKS_INIT(&moduleAttributes,
                                               &moduleEventCallbacks);
    moduleAttributes.PassiveLevel = TRUE;
    moduleEventCallbacks.EvtModuleOnDeviceNotificationPostOpen = Tests_DeviceInterfaceTarget_OnDeviceArrivalNotification_PassiveOutput;
    moduleEventCallbacks.EvtModuleOnDeviceNotificationPreClose = Tests_DeviceInterfaceTarget_OnDeviceRemovalNotification_PassiveOutput;
    moduleConfigDeviceInterfaceTarget.EvtDeviceInterfaceTargetOnStateChangeEx = Tests_DeviceInterfaceTarget_OnStateChange;
    moduleAttributes.ClientCallbacks = &moduleEventCallbacks;
    DMF_DmfModuleAdd(DmfModuleInit,
                     &moduleAttributes,
                     WDF_NO_OBJECT_ATTRIBUTES,
                     &moduleContext->DmfModuleDeviceInterfaceTargetPassiveOutput);

    // DeviceInterfaceTarget (PASSIVE_LEVEL)
    // No continuous requests.
    //
    DMF_CONFIG_DeviceInterfaceTarget_AND_ATTRIBUTES_INIT(&moduleConfigDeviceInterfaceTarget,
                                                         &moduleAttributes);
    moduleConfigDeviceInterfaceTarget.DeviceInterfaceTargetGuid = GUID_DEVINTERFACE_Tests_IoctlHandler;
    DMF_MODULE_ATTRIBUTES_EVENT_CALLBACKS_INIT(&moduleAttributes,
                                               &moduleEventCallbacks);
    moduleAttributes.PassiveLevel = TRUE;
    moduleEventCallbacks.EvtModuleOnDeviceNotificationPostOpen = Tests_DeviceInterfaceTarget_OnDeviceArrivalNotification_PassiveNonContinuous;
    moduleEventCallbacks.EvtModuleOnDeviceNotificationPreClose = Tests_DeviceInterfaceTarget_OnDeviceRemovalNotification_PassiveNonContinuous;
    moduleConfigDeviceInterfaceTarget.EvtDeviceInterfaceTargetOnStateChangeEx = Tests_DeviceInterfaceTarget_OnStateChange;
    moduleAttributes.ClientCallbacks = &moduleEventCallbacks;
    DMF_DmfModuleAdd(DmfModuleInit,
                     &moduleAttributes,
                     WDF_NO_OBJECT_ATTRIBUTES,
                     &moduleContext->DmfModuleDeviceInterfaceTargetPassiveNonContinuous);

    // DeviceInterfaceTarget (DISPATCH_LEVEL)
    // No continuous requests.
    //
    DMF_CONFIG_DeviceInterfaceTarget_AND_ATTRIBUTES_INIT(&moduleConfigDeviceInterfaceTarget,
                                                         &moduleAttributes);
    moduleConfigDeviceInterfaceTarget.DeviceInterfaceTargetGuid = GUID_DEVINTERFACE_Tests_IoctlHandler;
    DMF_MODULE_ATTRIBUTES_EVENT_CALLBACKS_INIT(&moduleAttributes,
                                               &moduleEventCallbacks);
    moduleEventCallbacks.EvtModuleOnDeviceNotificationPostOpen = Tests_DeviceInterfaceTarget_OnDeviceArrivalNotification_DispatchNonContinuous;
    moduleEventCallbacks.EvtModuleOnDeviceNotificationPreClose = Tests_DeviceInterfaceTarget_OnDeviceRemovalNotification_DispatchNonContinuous;
    moduleConfigDeviceInterfaceTarget.EvtDeviceInterfaceTargetOnStateChangeEx = Tests_DeviceInterfaceTarget_OnStateChange;
    moduleAttributes.ClientCallbacks = &moduleEventCallbacks;
    DMF_DmfModuleAdd(DmfModuleInit,
                     &moduleAttributes,
                     WDF_NO_OBJECT_ATTRIBUTES,
                     &moduleContext->DmfModuleDeviceInterfaceTargetDispatchNonContinuous);

    // Test valid combinations of pool types.
    //

    // BufferPool=Paged
    // DeviceInterfaceTarget=Paged
    // Input
    //
    DMF_CONFIG_DeviceInterfaceTarget_AND_ATTRIBUTES_INIT(&moduleConfigDeviceInterfaceTarget,
                                                         &moduleAttributes);
    moduleConfigDeviceInterfaceTarget.DeviceInterfaceTargetGuid = GUID_DEVINTERFACE_Tests_IoctlHandler;
    moduleConfigDeviceInterfaceTarget.ContinuousRequestTargetModuleConfig.BufferCountInput = 1;
    moduleConfigDeviceInterfaceTarget.ContinuousRequestTargetModuleConfig.BufferCountOutput = 1;
    moduleConfigDeviceInterfaceTarget.ContinuousRequestTargetModuleConfig.BufferInputSize = sizeof(Tests_IoctlHandler_Sleep);
    moduleConfigDeviceInterfaceTarget.ContinuousRequestTargetModuleConfig.BufferOutputSize = sizeof(Tests_IoctlHandler_Sleep);
    moduleConfigDeviceInterfaceTarget.ContinuousRequestTargetModuleConfig.ContinuousRequestCount = 1;
    moduleConfigDeviceInterfaceTarget.ContinuousRequestTargetModuleConfig.PoolTypeInput = PagedPool;
    moduleConfigDeviceInterfaceTarget.ContinuousRequestTargetModuleConfig.PurgeAndStartTargetInD0Callbacks = FALSE;
    moduleConfigDeviceInterfaceTarget.ContinuousRequestTargetModuleConfig.ContinuousRequestTargetIoctl = IOCTL_Tests_IoctlHandler_SLEEP;
    moduleConfigDeviceInterfaceTarget.ContinuousRequestTargetModuleConfig.EvtContinuousRequestTargetBufferInput = Tests_DeviceInterfaceTarget_BufferInput;
    moduleConfigDeviceInterfaceTarget.ContinuousRequestTargetModuleConfig.RequestType = ContinuousRequestTarget_RequestType_Ioctl;
    moduleConfigDeviceInterfaceTarget.ContinuousRequestTargetModuleConfig.ContinuousRequestTargetMode = ContinuousRequestTarget_Mode_Automatic;
    moduleConfigDeviceInterfaceTarget.ContinuousRequestTargetModuleConfig.PurgeAndStartTargetInD0Callbacks = TRUE;
    moduleAttributes.PassiveLevel = TRUE;
    moduleAttributes.ClientModuleInstanceName = "Input/Paged/Paged";
    DMF_DmfModuleAdd(DmfModuleInit,
                     &moduleAttributes,
                     WDF_NO_OBJECT_ATTRIBUTES,
                     NULL);

    // BufferPool=NonPagedNx
    // DeviceInterfaceTarget=Paged
    // Input
    //
    DMF_CONFIG_DeviceInterfaceTarget_AND_ATTRIBUTES_INIT(&moduleConfigDeviceInterfaceTarget,
                                                         &moduleAttributes);
    moduleConfigDeviceInterfaceTarget.DeviceInterfaceTargetGuid = GUID_DEVINTERFACE_Tests_IoctlHandler;
    moduleConfigDeviceInterfaceTarget.ContinuousRequestTargetModuleConfig.BufferCountInput = 1;
    moduleConfigDeviceInterfaceTarget.ContinuousRequestTargetModuleConfig.BufferCountOutput = 1;
    moduleConfigDeviceInterfaceTarget.ContinuousRequestTargetModuleConfig.BufferInputSize = sizeof(Tests_IoctlHandler_Sleep);
    moduleConfigDeviceInterfaceTarget.ContinuousRequestTargetModuleConfig.BufferOutputSize = sizeof(Tests_IoctlHandler_Sleep);
    moduleConfigDeviceInterfaceTarget.ContinuousRequestTargetModuleConfig.ContinuousRequestCount = 1;
    moduleConfigDeviceInterfaceTarget.ContinuousRequestTargetModuleConfig.PoolTypeInput = NonPagedPoolNx;
    moduleConfigDeviceInterfaceTarget.ContinuousRequestTargetModuleConfig.PurgeAndStartTargetInD0Callbacks = FALSE;
    moduleConfigDeviceInterfaceTarget.ContinuousRequestTargetModuleConfig.ContinuousRequestTargetIoctl = IOCTL_Tests_IoctlHandler_SLEEP;
    moduleConfigDeviceInterfaceTarget.ContinuousRequestTargetModuleConfig.EvtContinuousRequestTargetBufferInput = Tests_DeviceInterfaceTarget_BufferInput;
    moduleConfigDeviceInterfaceTarget.ContinuousRequestTargetModuleConfig.RequestType = ContinuousRequestTarget_RequestType_Ioctl;
    moduleConfigDeviceInterfaceTarget.ContinuousRequestTargetModuleConfig.ContinuousRequestTargetMode = ContinuousRequestTarget_Mode_Automatic;
    moduleConfigDeviceInterfaceTarget.ContinuousRequestTargetModuleConfig.PurgeAndStartTargetInD0Callbacks = TRUE;
    moduleAttributes.PassiveLevel = TRUE;
    moduleAttributes.ClientModuleInstanceName = "Input/NonPaged/Paged";
    DMF_DmfModuleAdd(DmfModuleInit,
                     &moduleAttributes,
                     WDF_NO_OBJECT_ATTRIBUTES,
                     NULL);

    // BufferPool=NonPagedNx
    // DeviceInterfaceTarget=NonPaged
    // Input
    //
    DMF_CONFIG_DeviceInterfaceTarget_AND_ATTRIBUTES_INIT(&moduleConfigDeviceInterfaceTarget,
                                                         &moduleAttributes);
    moduleConfigDeviceInterfaceTarget.DeviceInterfaceTargetGuid = GUID_DEVINTERFACE_Tests_IoctlHandler;
    moduleConfigDeviceInterfaceTarget.ContinuousRequestTargetModuleConfig.BufferCountInput = 1;
    moduleConfigDeviceInterfaceTarget.ContinuousRequestTargetModuleConfig.BufferCountOutput = 1;
    moduleConfigDeviceInterfaceTarget.ContinuousRequestTargetModuleConfig.BufferInputSize = sizeof(Tests_IoctlHandler_Sleep);
    moduleConfigDeviceInterfaceTarget.ContinuousRequestTargetModuleConfig.BufferOutputSize = sizeof(Tests_IoctlHandler_Sleep);
    moduleConfigDeviceInterfaceTarget.ContinuousRequestTargetModuleConfig.ContinuousRequestCount = 1;
    moduleConfigDeviceInterfaceTarget.ContinuousRequestTargetModuleConfig.PoolTypeInput = NonPagedPoolNx;
    moduleConfigDeviceInterfaceTarget.ContinuousRequestTargetModuleConfig.PurgeAndStartTargetInD0Callbacks = FALSE;
    moduleConfigDeviceInterfaceTarget.ContinuousRequestTargetModuleConfig.ContinuousRequestTargetIoctl = IOCTL_Tests_IoctlHandler_SLEEP;
    moduleConfigDeviceInterfaceTarget.ContinuousRequestTargetModuleConfig.EvtContinuousRequestTargetBufferInput = Tests_DeviceInterfaceTarget_BufferInput;
    moduleConfigDeviceInterfaceTarget.ContinuousRequestTargetModuleConfig.RequestType = ContinuousRequestTarget_RequestType_Ioctl;
    moduleConfigDeviceInterfaceTarget.ContinuousRequestTargetModuleConfig.ContinuousRequestTargetMode = ContinuousRequestTarget_Mode_Automatic;
    moduleConfigDeviceInterfaceTarget.ContinuousRequestTargetModuleConfig.PurgeAndStartTargetInD0Callbacks = TRUE;
    moduleAttributes.ClientModuleInstanceName = "Input/NonPaged/NonPaged";
    DMF_DmfModuleAdd(DmfModuleInit,
                     &moduleAttributes,
                     WDF_NO_OBJECT_ATTRIBUTES,
                     NULL);

    // BufferPool=Paged
    // DeviceInterfaceTarget=Paged
    // Output
    //
    DMF_CONFIG_DeviceInterfaceTarget_AND_ATTRIBUTES_INIT(&moduleConfigDeviceInterfaceTarget,
                                                         &moduleAttributes);
    moduleConfigDeviceInterfaceTarget.DeviceInterfaceTargetGuid = GUID_DEVINTERFACE_Tests_IoctlHandler;
    moduleConfigDeviceInterfaceTarget.ContinuousRequestTargetModuleConfig.BufferCountOutput = NUMBER_OF_CONTINUOUS_REQUESTS;
    moduleConfigDeviceInterfaceTarget.ContinuousRequestTargetModuleConfig.BufferOutputSize = sizeof(DWORD);
    moduleConfigDeviceInterfaceTarget.ContinuousRequestTargetModuleConfig.ContinuousRequestCount = NUMBER_OF_CONTINUOUS_REQUESTS;
    moduleConfigDeviceInterfaceTarget.ContinuousRequestTargetModuleConfig.PoolTypeOutput = PagedPool;
    moduleConfigDeviceInterfaceTarget.ContinuousRequestTargetModuleConfig.PurgeAndStartTargetInD0Callbacks = FALSE;
    moduleConfigDeviceInterfaceTarget.ContinuousRequestTargetModuleConfig.ContinuousRequestTargetIoctl = IOCTL_Tests_IoctlHandler_ZEROBUFFER;
    moduleConfigDeviceInterfaceTarget.ContinuousRequestTargetModuleConfig.EvtContinuousRequestTargetBufferOutput = Tests_DeviceInterfaceTarget_BufferOutput;
    moduleConfigDeviceInterfaceTarget.ContinuousRequestTargetModuleConfig.RequestType = ContinuousRequestTarget_RequestType_Ioctl;
    moduleConfigDeviceInterfaceTarget.ContinuousRequestTargetModuleConfig.ContinuousRequestTargetMode = ContinuousRequestTarget_Mode_Automatic;
    moduleConfigDeviceInterfaceTarget.ContinuousRequestTargetModuleConfig.PurgeAndStartTargetInD0Callbacks = TRUE;
    moduleAttributes.PassiveLevel = TRUE;
    moduleAttributes.ClientModuleInstanceName = "Output/Paged/Paged";
    DMF_DmfModuleAdd(DmfModuleInit,
                     &moduleAttributes,
                     WDF_NO_OBJECT_ATTRIBUTES,
                     NULL);

    // BufferPool=NonPaged
    // DeviceInterfaceTarget=Paged
    // Output
    //
    DMF_CONFIG_DeviceInterfaceTarget_AND_ATTRIBUTES_INIT(&moduleConfigDeviceInterfaceTarget,
                                                         &moduleAttributes);
    moduleConfigDeviceInterfaceTarget.DeviceInterfaceTargetGuid = GUID_DEVINTERFACE_Tests_IoctlHandler;
    moduleConfigDeviceInterfaceTarget.ContinuousRequestTargetModuleConfig.BufferCountOutput = NUMBER_OF_CONTINUOUS_REQUESTS;
    moduleConfigDeviceInterfaceTarget.ContinuousRequestTargetModuleConfig.BufferOutputSize = sizeof(DWORD);
    moduleConfigDeviceInterfaceTarget.ContinuousRequestTargetModuleConfig.ContinuousRequestCount = NUMBER_OF_CONTINUOUS_REQUESTS;
    moduleConfigDeviceInterfaceTarget.ContinuousRequestTargetModuleConfig.PoolTypeOutput = NonPagedPoolNx;
    moduleConfigDeviceInterfaceTarget.ContinuousRequestTargetModuleConfig.PurgeAndStartTargetInD0Callbacks = FALSE;
    moduleConfigDeviceInterfaceTarget.ContinuousRequestTargetModuleConfig.ContinuousRequestTargetIoctl = IOCTL_Tests_IoctlHandler_ZEROBUFFER;
    moduleConfigDeviceInterfaceTarget.ContinuousRequestTargetModuleConfig.EvtContinuousRequestTargetBufferOutput = Tests_DeviceInterfaceTarget_BufferOutput;
    moduleConfigDeviceInterfaceTarget.ContinuousRequestTargetModuleConfig.RequestType = ContinuousRequestTarget_RequestType_Ioctl;
    moduleConfigDeviceInterfaceTarget.ContinuousRequestTargetModuleConfig.ContinuousRequestTargetMode = ContinuousRequestTarget_Mode_Automatic;
    moduleConfigDeviceInterfaceTarget.ContinuousRequestTargetModuleConfig.PurgeAndStartTargetInD0Callbacks = TRUE;
    moduleAttributes.PassiveLevel = TRUE;
    moduleAttributes.ClientModuleInstanceName = "Output/NonPaged/Paged";
    DMF_DmfModuleAdd(DmfModuleInit,
                     &moduleAttributes,
                     WDF_NO_OBJECT_ATTRIBUTES,
                     NULL);

    // BufferPool=NonPaged
    // DeviceInterfaceTarget=NonPaged
    // Output
    //
    DMF_CONFIG_DeviceInterfaceTarget_AND_ATTRIBUTES_INIT(&moduleConfigDeviceInterfaceTarget,
                                                         &moduleAttributes);
    moduleConfigDeviceInterfaceTarget.DeviceInterfaceTargetGuid = GUID_DEVINTERFACE_Tests_IoctlHandler;
    moduleConfigDeviceInterfaceTarget.ContinuousRequestTargetModuleConfig.BufferCountOutput = NUMBER_OF_CONTINUOUS_REQUESTS;
    moduleConfigDeviceInterfaceTarget.ContinuousRequestTargetModuleConfig.BufferOutputSize = sizeof(DWORD);
    moduleConfigDeviceInterfaceTarget.ContinuousRequestTargetModuleConfig.ContinuousRequestCount = NUMBER_OF_CONTINUOUS_REQUESTS;
    moduleConfigDeviceInterfaceTarget.ContinuousRequestTargetModuleConfig.PoolTypeOutput = NonPagedPoolNx;
    moduleConfigDeviceInterfaceTarget.ContinuousRequestTargetModuleConfig.PurgeAndStartTargetInD0Callbacks = FALSE;
    moduleConfigDeviceInterfaceTarget.ContinuousRequestTargetModuleConfig.ContinuousRequestTargetIoctl = IOCTL_Tests_IoctlHandler_ZEROBUFFER;
    moduleConfigDeviceInterfaceTarget.ContinuousRequestTargetModuleConfig.EvtContinuousRequestTargetBufferOutput = Tests_DeviceInterfaceTarget_BufferOutput;
    moduleConfigDeviceInterfaceTarget.ContinuousRequestTargetModuleConfig.RequestType = ContinuousRequestTarget_RequestType_Ioctl;
    moduleConfigDeviceInterfaceTarget.ContinuousRequestTargetModuleConfig.ContinuousRequestTargetMode = ContinuousRequestTarget_Mode_Automatic;
    moduleConfigDeviceInterfaceTarget.ContinuousRequestTargetModuleConfig.PurgeAndStartTargetInD0Callbacks = TRUE;
    moduleAttributes.ClientModuleInstanceName = "Output/NonPaged/NonPaged";
    DMF_DmfModuleAdd(DmfModuleInit,
                     &moduleAttributes,
                     WDF_NO_OBJECT_ATTRIBUTES,
                     NULL);

    // This instance allows for arrival/removal on demand.
    //
    DMF_CONFIG_DeviceInterfaceTarget_AND_ATTRIBUTES_INIT(&moduleConfigDeviceInterfaceTarget,
                                                         &moduleAttributes);
    moduleConfigDeviceInterfaceTarget.DeviceInterfaceTargetGuid = GUID_DEVINTERFACE_DISK;
    moduleConfigDeviceInterfaceTarget.OpenMode = GENERIC_READ;
    moduleConfigDeviceInterfaceTarget.ShareAccess = FILE_SHARE_READ;
 
    // NOTE: No Module handle is needed since no Methods are called.
    //
    DMF_DmfModuleAdd(DmfModuleInit,
                     &moduleAttributes,
                     WDF_NO_OBJECT_ATTRIBUTES,
                     NULL);

    // This instance tests for no device attached ever.
    //
    DMF_CONFIG_DeviceInterfaceTarget_AND_ATTRIBUTES_INIT(&moduleConfigDeviceInterfaceTarget,
                                                         &moduleAttributes);
    moduleConfigDeviceInterfaceTarget.DeviceInterfaceTargetGuid = GUID_NO_DEVICE;
    moduleConfigDeviceInterfaceTarget.OpenMode = GENERIC_READ;
    moduleConfigDeviceInterfaceTarget.ShareAccess = FILE_SHARE_READ;
 
    // NOTE: No Module handle is needed since no Methods are called.
    //
    DMF_DmfModuleAdd(DmfModuleInit,
                     &moduleAttributes,
                     WDF_NO_OBJECT_ATTRIBUTES,
                     NULL);
#endif

    FuncExitVoid(DMF_TRACE);
}
#pragma code_seg()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// Public Calls by Client
///////////////////////////////////////////////////////////////////////////////////////////////////////
//

#pragma code_seg("PAGE")
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS
DMF_Tests_DeviceInterfaceTarget_Create(
    _In_ WDFDEVICE Device,
    _In_ DMF_MODULE_ATTRIBUTES* DmfModuleAttributes,
    _In_ WDF_OBJECT_ATTRIBUTES* ObjectAttributes,
    _Out_ DMFMODULE* DmfModule
    )
/*++

Routine Description:

    Create an instance of a DMF Module of type Test_DeviceInterfaceTarget.

Arguments:

    Device - Client driver's WDFDEVICE object.
    DmfModuleAttributes - Opaque structure that contains parameters DMF needs to initialize the Module.
    ObjectAttributes - WDF object attributes for DMFMODULE.
    DmfModule - Address of the location where the created DMFMODULE handle is returned.

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS ntStatus;
    DMF_MODULE_DESCRIPTOR dmfModuleDescriptor_Tests_DeviceInterfaceTarget;
    DMF_CALLBACKS_DMF dmfCallbacksDmf_Tests_DeviceInterfaceTarget;

    PAGED_CODE();

    DMF_CALLBACKS_DMF_INIT(&dmfCallbacksDmf_Tests_DeviceInterfaceTarget);
    dmfCallbacksDmf_Tests_DeviceInterfaceTarget.ChildModulesAdd = DMF_Tests_DeviceInterfaceTarget_ChildModulesAdd;

    DMF_MODULE_DESCRIPTOR_INIT_CONTEXT_TYPE(dmfModuleDescriptor_Tests_DeviceInterfaceTarget,
                                            Tests_DeviceInterfaceTarget,
                                            DMF_CONTEXT_Tests_DeviceInterfaceTarget,
                                            DMF_MODULE_OPTIONS_PASSIVE,
                                            DMF_MODULE_OPEN_OPTION_OPEN_PrepareHardware);

    dmfModuleDescriptor_Tests_DeviceInterfaceTarget.CallbacksDmf = &dmfCallbacksDmf_Tests_DeviceInterfaceTarget;

    ntStatus = DMF_ModuleCreate(Device,
                                DmfModuleAttributes,
                                ObjectAttributes,
                                &dmfModuleDescriptor_Tests_DeviceInterfaceTarget,
                                DmfModule);
    if (!NT_SUCCESS(ntStatus))
    {
        TraceEvents(TRACE_LEVEL_ERROR, DMF_TRACE, "DMF_ModuleCreate fails: ntStatus=%!STATUS!", ntStatus);
    }

    return(ntStatus);
}
#pragma code_seg()

// Module Methods
//

// eof: Dmf_Tests_DeviceInterfaceTarget.c
//
