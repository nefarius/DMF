/*++

    Copyright (c) Microsoft Corporation. All rights reserved.
    Licensed under the MIT license.

Module Name:

    DmfCore.c

Abstract:

    DMF Implementation:

    This Module contains DMF Module create/destroy support.

    NOTE: Make sure to set "compile as C++" option.
    NOTE: Make sure to #define DMF_USER_MODE in UMDF Drivers.
    NOTE: Some, but not all DMF Modules are compatible with both User-mode and Kernel-mode.
          Module authors should "#if defined(DMF_USER_MODE)" in Modules to support both modes.

Environment:

    Kernel-mode Driver Framework
    User-mode Driver Framework

--*/

#include "DmfIncludeInternal.h"

#if defined(DMF_INCLUDE_TMH)
#include "DmfCore.tmh"
#endif

// Internal Callbacks for PASSIVE_LEVEL.
//
DMF_CALLBACKS_DMF
DmfCallbacksDmf_Internal_Passive =
{
    sizeof(DMF_CALLBACKS_DMF),
    DMF_Internal_Destroy,
    DMF_Internal_ResourcesAssign,
    DMF_Internal_NotificationRegister,
    DMF_Internal_NotificationUnregister,
    DMF_Internal_Open,
    DMF_Internal_Close,
};

DMF_CALLBACKS_INTERNAL
DmfCallbacksInternal_Internal_Passive =
{
    sizeof(DMF_CALLBACKS_INTERNAL),
    DMF_Generic_Lock_Passive,
    DMF_Generic_Unlock_Passive,
    DMF_Generic_AuxiliaryLock_Passive,
    DMF_Generic_AuxiliaryUnlock_Passive
};

DMF_CALLBACKS_WDF
DmfCallbacksWdf_Internal_Passive =
{
    sizeof(DMF_CALLBACKS_WDF),
    DMF_Internal_ModulePrepareHardware,
    DMF_Internal_ModuleReleaseHardware,
    DMF_Internal_ModuleD0Entry,
    DMF_Internal_ModuleD0EntryPostInterruptsEnabled,
    DMF_Internal_ModuleD0ExitPreInterruptsDisabled,
    DMF_Internal_ModuleD0Exit,
    DMF_Internal_ModuleQueueIoRead,
    DMF_Internal_ModuleQueueIoWrite,
    DMF_Internal_ModuleDeviceIoControl,
    DMF_Internal_ModuleInternalDeviceIoControl,
    DMF_Internal_ModuleSelfManagedIoCleanup,
    DMF_Internal_ModuleSelfManagedIoFlush,
    DMF_Internal_ModuleSelfManagedIoInit,
    DMF_Internal_ModuleSelfManagedIoSuspend,
    DMF_Internal_ModuleSelfManagedIoRestart,
    DMF_Internal_ModuleSurpriseRemoval,
    DMF_Internal_ModuleQueryRemove,
    DMF_Internal_ModuleQueryStop,
    DMF_Internal_ModuleRelationsQuery,
    DMF_Internal_ModuleUsageNotificationEx,
    DMF_Internal_ModuleArmWakeFromS0,
    DMF_Internal_ModuleDisarmWakeFromS0,
    DMF_Internal_ModuleWakeFromS0Triggered,
    DMF_Internal_ModuleArmWakeFromSxWithReason,
    DMF_Internal_ModuleDisarmWakeFromSx,
    DMF_Internal_ModuleWakeFromSxTriggered,
    DMF_Internal_ModuleFileCreate,
    DMF_Internal_ModuleFileCleanup,
    DMF_Internal_ModuleFileClose,
};

DMF_CALLBACKS_DMF
DmfCallbacksDmf_Internal_Dispatch =
{
    sizeof(DMF_CALLBACKS_DMF),
    DMF_Internal_Destroy,
    DMF_Internal_ResourcesAssign,
    DMF_Internal_NotificationRegister,
    DMF_Internal_NotificationUnregister,
    DMF_Internal_Open,
    DMF_Internal_Close,
};

DMF_CALLBACKS_INTERNAL
DmfCallbacksInternal_Internal_Dispatch =
{
    sizeof(DMF_CALLBACKS_INTERNAL),
    DMF_Generic_Lock_Dispatch,
    DMF_Generic_Unlock_Dispatch,
    DMF_Generic_AuxiliaryLock_Dispatch,
    DMF_Generic_AuxiliaryUnlock_Dispatch
};

DMF_CALLBACKS_WDF
DmfCallbacksWdf_Internal_Dispatch =
{
    sizeof(DMF_CALLBACKS_WDF),
    DMF_Internal_ModulePrepareHardware,
    DMF_Internal_ModuleReleaseHardware,
    DMF_Internal_ModuleD0Entry,
    DMF_Internal_ModuleD0EntryPostInterruptsEnabled,
    DMF_Internal_ModuleD0ExitPreInterruptsDisabled,
    DMF_Internal_ModuleD0Exit,
    DMF_Internal_ModuleQueueIoRead,
    DMF_Internal_ModuleQueueIoWrite,
    DMF_Internal_ModuleDeviceIoControl,
    DMF_Internal_ModuleInternalDeviceIoControl,
    DMF_Internal_ModuleSelfManagedIoCleanup,
    DMF_Internal_ModuleSelfManagedIoFlush,
    DMF_Internal_ModuleSelfManagedIoInit,
    DMF_Internal_ModuleSelfManagedIoSuspend,
    DMF_Internal_ModuleSelfManagedIoRestart,
    DMF_Internal_ModuleSurpriseRemoval,
    DMF_Internal_ModuleQueryRemove,
    DMF_Internal_ModuleQueryStop,
    DMF_Internal_ModuleRelationsQuery,
    DMF_Internal_ModuleUsageNotificationEx,
    DMF_Internal_ModuleArmWakeFromS0,
    DMF_Internal_ModuleDisarmWakeFromS0,
    DMF_Internal_ModuleWakeFromS0Triggered,
    DMF_Internal_ModuleArmWakeFromSxWithReason,
    DMF_Internal_ModuleDisarmWakeFromSx,
    DMF_Internal_ModuleWakeFromSxTriggered,
    DMF_Internal_ModuleFileCreate,
    DMF_Internal_ModuleFileCleanup,
    DMF_Internal_ModuleFileClose,
};

#pragma code_seg("PAGE")
static
VOID
DmfCallbacksDmfInitialize(
    _Out_ DMF_CALLBACKS_DMF* DmfCallbacksDmf
    )
/*++

Routine Description:

    Populate a given DMF_CALLBACKS_DMF structure with generic callbacks.

Arguments:

    DmfCallbacksDmf - The given DMF_CALLBACKS_DMF structure.

Return Value:

    None

--*/
{
    PAGED_CODE();

    RtlZeroMemory(DmfCallbacksDmf,
                  sizeof(DMF_CALLBACKS_DMF));

    DmfCallbacksDmf->Size = sizeof(DMF_CALLBACKS_DMF);
    DmfCallbacksDmf->ModuleInstanceDestroy = DMF_Generic_Destroy;
    DmfCallbacksDmf->DeviceResourcesAssign = DMF_Generic_ResourcesAssign;
    DmfCallbacksDmf->DeviceNotificationRegister = DMF_Generic_NotificationRegister;
    DmfCallbacksDmf->DeviceNotificationUnregister = DMF_Generic_NotificationUnregister;
    DmfCallbacksDmf->DeviceOpen = DMF_Generic_Open;
    DmfCallbacksDmf->DeviceClose = DMF_Generic_Close;
    DmfCallbacksDmf->ChildModulesAdd = DMF_Generic_ChildModulesAdd;
}
#pragma code_seg()

#pragma code_seg("PAGE")
static
VOID
DmfCallbacksWdfInitialize(
    _Out_ DMF_CALLBACKS_WDF* DmfCallbacksWdf
    )
/*++

Routine Description:

    Populate a given DMF_CALLBACKS_WDF structure with generic callbacks.

Arguments:

    DmfCallbacksDmf - The given DMF_CALLBACKS_WDF structure.

Return Value:

    None

--*/
{
    PAGED_CODE();

    RtlZeroMemory(DmfCallbacksWdf,
                  sizeof(DMF_CALLBACKS_WDF));

    DmfCallbacksWdf->Size = sizeof(DMF_CALLBACKS_WDF);
    DmfCallbacksWdf->ModulePrepareHardware = DMF_Generic_ModulePrepareHardware;
    DmfCallbacksWdf->ModuleReleaseHardware = DMF_Generic_ModuleReleaseHardware;
    DmfCallbacksWdf->ModuleD0Entry = DMF_Generic_ModuleD0Entry;
    DmfCallbacksWdf->ModuleD0EntryPostInterruptsEnabled = DMF_Generic_ModuleD0EntryPostInterruptsEnabled;
    DmfCallbacksWdf->ModuleD0ExitPreInterruptsDisabled = DMF_Generic_ModuleD0ExitPreInterruptsDisabled;
    DmfCallbacksWdf->ModuleD0Exit = DMF_Generic_ModuleD0Exit;
    DmfCallbacksWdf->ModuleQueueIoRead = DMF_Generic_ModuleQueueIoRead;
    DmfCallbacksWdf->ModuleQueueIoWrite = DMF_Generic_ModuleQueueIoWrite;
    DmfCallbacksWdf->ModuleDeviceIoControl = DMF_Generic_ModuleDeviceIoControl;
    DmfCallbacksWdf->ModuleInternalDeviceIoControl = DMF_Generic_ModuleInternalDeviceIoControl;
    DmfCallbacksWdf->ModuleSelfManagedIoCleanup = DMF_Generic_ModuleSelfManagedIoCleanup;
    DmfCallbacksWdf->ModuleSelfManagedIoFlush = DMF_Generic_ModuleSelfManagedIoFlush;
    DmfCallbacksWdf->ModuleSelfManagedIoInit = DMF_Generic_ModuleSelfManagedIoInit;
    DmfCallbacksWdf->ModuleSelfManagedIoSuspend = DMF_Generic_ModuleSelfManagedIoSuspend;
    DmfCallbacksWdf->ModuleSelfManagedIoRestart = DMF_Generic_ModuleSelfManagedIoRestart;
    DmfCallbacksWdf->ModuleSurpriseRemoval = DMF_Generic_ModuleSurpriseRemoval;
    DmfCallbacksWdf->ModuleQueryRemove = DMF_Generic_ModuleQueryRemove;
    DmfCallbacksWdf->ModuleQueryStop = DMF_Generic_ModuleQueryStop;
    DmfCallbacksWdf->ModuleRelationsQuery = DMF_Generic_ModuleRelationsQuery;
    DmfCallbacksWdf->ModuleUsageNotificationEx = DMF_Generic_ModuleUsageNotificationEx;
    DmfCallbacksWdf->ModuleArmWakeFromS0 = DMF_Generic_ModuleArmWakeFromS0;
    DmfCallbacksWdf->ModuleDisarmWakeFromS0 = DMF_Generic_ModuleDisarmWakeFromS0;
    DmfCallbacksWdf->ModuleWakeFromS0Triggered = DMF_Generic_ModuleWakeFromS0Triggered;
    DmfCallbacksWdf->ModuleArmWakeFromSxWithReason = DMF_Generic_ModuleArmWakeFromSxWithReason;
    DmfCallbacksWdf->ModuleDisarmWakeFromSx = DMF_Generic_ModuleDisarmWakeFromSx;
    DmfCallbacksWdf->ModuleWakeFromSxTriggered = DMF_Generic_ModuleWakeFromSxTriggered;
    DmfCallbacksWdf->ModuleFileCreate = DMF_Generic_ModuleFileCreate;
    DmfCallbacksWdf->ModuleFileCleanup = DMF_Generic_ModuleFileCleanup;
    DmfCallbacksWdf->ModuleFileClose = DMF_Generic_ModuleFileClose;
}
#pragma code_seg()

#pragma code_seg("PAGE")
_Must_inspect_result_
static
NTSTATUS
DmfModuleInstanceNameInitialize(
    _Inout_ DMF_OBJECT* DmfObject,
    _In_ WDFMEMORY MemoryDmfObject,
    _In_ DMF_MODULE_ATTRIBUTES* DmfModuleAttributes
    )
/*++

Routine Description:

    Populate a given DMF_OBJECT structure with Client Module Instance Name.

Arguments:

    DmfObject - The given DMF_OBJECT structure.
    MemoryDmfObject - The corresponding WDFMEMORY object for DmfObject.
    DmfModuleAttributes - Pointer to the initialized DMF_MODULE_ATTRIBUTES structure.

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS ntStatus;
    CONST CHAR* clientModuleInstanceName;

    UNREFERENCED_PARAMETER(MemoryDmfObject);

    PAGED_CODE();

    ntStatus = STATUS_SUCCESS;

    // Create space for the Client Module Instance Name. It needs to be allocated because
    // the name that is passed in may not be statically allocated. A copy needs to be made
    // in case the Client Driver has allocated the passed in name on the stack. We don't 
    // want to force the Client Driver to maintain space for the name in cases where the 
    // name is generated.
    //
    DmfAssert(DmfModuleAttributes->ClientModuleInstanceName != NULL);
    if (*DmfModuleAttributes->ClientModuleInstanceName == '\0')
    {
        // If Client Driver has passed "", then use the Module Name as the Module Instance Name.
        // (Client Driver only needs to set this string in cases where multiple instances of a 
        // DMF Module are instantiated.)
        //
        clientModuleInstanceName = DmfObject->ModuleName;
    }
    else
    {
        // Use the name the Client passed in. 
        //
        clientModuleInstanceName = DmfModuleAttributes->ClientModuleInstanceName;
    }
    DmfObject->ClientModuleInstanceNameSizeBytes = strlen(clientModuleInstanceName) + sizeof(CHAR);
    DmfAssert(DmfObject->ClientModuleInstanceNameSizeBytes > 1);

    // Allocate space for the instance name. This name is useful during debugging.
    // NOTE: Don't use WDFMEMORY to reduce number of use WDFOBJECT.
    //
    DmfObject->ClientModuleInstanceName = (CHAR*)DMF_GenericMemoryAllocate(POOL_FLAG_NON_PAGED,
                                                                           DmfObject->ClientModuleInstanceNameSizeBytes,
                                                                           DMF_TAG0);
    if (DmfObject->ClientModuleInstanceName == NULL)
    {
        TraceEvents(TRACE_LEVEL_ERROR, DMF_TRACE, "Unable to allocate ClientModuleInstanceName");
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }
    RtlZeroMemory(DmfObject->ClientModuleInstanceName,
                  DmfObject->ClientModuleInstanceNameSizeBytes);
    // Copy the string. The length passed is one more than the length but that
    // extra byte is terminating zero which will not be copied.
    //
    strncpy_s(DmfObject->ClientModuleInstanceName,
              DmfObject->ClientModuleInstanceNameSizeBytes,
              clientModuleInstanceName,
              DmfObject->ClientModuleInstanceNameSizeBytes);
    DmfAssert(DmfObject->ClientModuleInstanceNameSizeBytes > 0);
    DmfAssert(DmfObject->ClientModuleInstanceName[DmfObject->ClientModuleInstanceNameSizeBytes - 1] == '\0');
    DmfAssert(DmfObject->ClientModuleInstanceName[0] != '\0');

Exit:

    return ntStatus;
}
#pragma code_seg()

#pragma code_seg("PAGE")
_Must_inspect_result_
static
NTSTATUS
DmfModuleChildObjectsInitialize(
    _Inout_ DMF_OBJECT* DmfObject,
    _In_ WDFMEMORY MemoryDmfObject,
    _In_ DMF_MODULE_ATTRIBUTES* DmfModuleAttributes,
    _In_ DMF_MODULE_DESCRIPTOR* ModuleDescriptor
    )
/*++

Routine Description:

    Initialize various child objects in a given DMF_OBJECT structure.

Arguments:

    DmfObject - The given DMF_OBJECT structure.
    MemoryDmfObject - The corresponding WDFMEMORY object for DmfObject.
    DmfModuleAttributes - Pointer to the initialized DMF_MODULE_ATTRIBUTES structure.
    ModuleDescriptor - Pointer to the DMF_MODULE_DESCRIPTOR structure providing information about the Module.

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS ntStatus;
    WDF_OBJECT_ATTRIBUTES objectAttributes;

    PAGED_CODE();

    // Create the area for Module Config, if any.
    //
    DmfAssert(NULL == DmfObject->ModuleConfig);
    if (DmfModuleAttributes->SizeOfModuleSpecificConfig != NULL)
    {
        DmfAssert(ModuleDescriptor->ModuleConfigSize == DmfModuleAttributes->SizeOfModuleSpecificConfig);
        DmfObject->ModuleConfig = DMF_GenericMemoryAllocate(POOL_FLAG_NON_PAGED,
                                                            ModuleDescriptor->ModuleConfigSize,
                                                            DMF_TAG1);
        if (DmfObject->ModuleConfig == NULL)
        {
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
            TraceEvents(TRACE_LEVEL_ERROR, DMF_TRACE, "Unable to allocate Module Config");
            goto Exit;
        }
        DmfObject->ModuleConfigSize = ModuleDescriptor->ModuleConfigSize;

        // Save off the Module Config information for when the Open happens later.
        // If Client calls DMF_##ModuleName##_ATTRIBUTES_INIT instead of
        // DMF_CONFIG_##ModuleName##_AND_ATTRIBUTES_INIT, then Client does not 
        // provide any configuration information. 
        // Drivers that use Modules that do not have a Config use this path when
        // a Config is added afterward. It enables the existing drivers to compile
        // and run.
        // In this case, when the Module retrieves its config pointer the contents
        // will be zero.
        //
        if (DmfModuleAttributes->ModuleConfigPointer != NULL)
        {
            RtlCopyMemory(DmfObject->ModuleConfig,
                          DmfModuleAttributes->ModuleConfigPointer,
                          ModuleDescriptor->ModuleConfigSize);
        }
        else
        {
            RtlZeroMemory(DmfObject->ModuleConfig,
                          ModuleDescriptor->ModuleConfigSize);
        }
    }
    else
    {
        // NOTE: Because only proper Config initialization macros are exposed, there is no way for the 
        //       Client to improperly initialize the Config (as it was in the past). It means, that if 
        //       this path executes, the Module author has not defined a Config.
        //
    }

    // Create a spin lock to protect the Module's reference count.
    // (Reference count must be accessible from both PASSIVE_LEVEL and DISPATCH_LEVEL
    // code so the Module's synchronization locks cannot be used because they can 
    // be either waitlocks or spinlocks.)
    //
    WDF_OBJECT_ATTRIBUTES_INIT(&objectAttributes);
    objectAttributes.ParentObject = MemoryDmfObject;
    ntStatus = DMF_GenericSpinLockCreate(&objectAttributes,
                                         &DmfObject->ReferenceCountLock);
    if (!NT_SUCCESS(ntStatus))
    {
        TraceEvents(TRACE_LEVEL_ERROR, DMF_TRACE, "ReferenceCountLock create fails.");
        goto Exit;
    }

Exit:

    return ntStatus;
}
#pragma code_seg()

#pragma code_seg("PAGE")
_Must_inspect_result_
static
NTSTATUS
DmfModuleCallbacksInitialize(
    _Inout_ DMF_OBJECT* DmfObject,
    _In_ DMF_MODULE_ATTRIBUTES* DmfModuleAttributes,
    _In_ DMF_MODULE_DESCRIPTOR* ModuleDescriptor
    )
/*++

Routine Description:

    Populate callback functions pointers in a given DMF_CALLBACKS_DMF structure.

Arguments:

    DmfObject - The given DMF_OBJECT structure.
    MemoryDmfObject - The corresponding WDFMEMORY object for DmfObject.
    DmfModuleAttributes - Pointer to the initialized DMF_MODULE_ATTRIBUTES structure.
    ModuleDescriptor - Pointer to the DMF_MODULE_DESCRIPTOR structure providing information about the Module.

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS ntStatus;
    DMF_MODULE_EVENT_CALLBACKS* callbacks;

    PAGED_CODE();

    DmfObject->ModuleDescriptor.CallbacksDmf = (DMF_CALLBACKS_DMF*)DMF_GenericMemoryAllocate(POOL_FLAG_NON_PAGED,
                                                                                             sizeof(DMF_CALLBACKS_DMF),
                                                                                             DMF_TAG2);
    if (NULL == DmfObject->ModuleDescriptor.CallbacksDmf)
    {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        TraceEvents(TRACE_LEVEL_ERROR, DMF_TRACE, "WdfMemoryCreate fails: ntStatus=%!STATUS!", ntStatus);
        goto Exit;
    }

    RtlZeroMemory(DmfObject->ModuleDescriptor.CallbacksDmf,
                  sizeof(DMF_CALLBACKS_DMF));

    DmfObject->ModuleDescriptor.CallbacksWdf = (DMF_CALLBACKS_WDF*)DMF_GenericMemoryAllocate(POOL_FLAG_NON_PAGED,
                                                                                             sizeof(DMF_CALLBACKS_WDF),
                                                                                             DMF_TAG3);
    if (NULL == DmfObject->ModuleDescriptor.CallbacksWdf)
    {
        DMF_GenericMemoryFree(DmfObject->ModuleDescriptor.CallbacksDmf,
                              DMF_TAG2);
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        TraceEvents(TRACE_LEVEL_ERROR, DMF_TRACE, "WdfMemoryCreate fails: ntStatus=%!STATUS!", ntStatus);
        DmfObject->ModuleDescriptor.CallbacksDmf = NULL;
        goto Exit;
    }

    ntStatus = STATUS_SUCCESS;

    RtlZeroMemory(DmfObject->ModuleDescriptor.CallbacksWdf,
                  sizeof(DMF_CALLBACKS_WDF));

    DmfCallbacksDmfInitialize(DmfObject->ModuleDescriptor.CallbacksDmf);
    DmfCallbacksWdfInitialize(DmfObject->ModuleDescriptor.CallbacksWdf);

    // Set the optional callbacks.
    //
    callbacks = DmfModuleAttributes->ClientCallbacks;
    if (callbacks != NULL)
    {
        // Copy the Client Driver's Asynchronous callbacks.
        //
        DmfObject->Callbacks = *callbacks;
    }
    else
    {
        DmfObject->Callbacks.EvtModuleOnDeviceNotificationPreClose = USE_GENERIC_CALLBACK;
        DmfObject->Callbacks.EvtModuleOnDeviceNotificationPostOpen = USE_GENERIC_CALLBACK;
    }

    // Set Internal Callbacks.
    // NOTE: Use updated options, not global options.
    //
    if (DmfObject->ModuleDescriptor.ModuleOptions & DMF_MODULE_OPTIONS_DISPATCH)
    {
        // For Modules that run at Dispatch Level.
        //
        DmfAssert(! (DmfObject->ModuleDescriptor.ModuleOptions & DMF_MODULE_OPTIONS_PASSIVE));
        DmfObject->InternalCallbacksDmf = DmfCallbacksDmf_Internal_Dispatch;
        DmfObject->InternalCallbacksWdf = DmfCallbacksWdf_Internal_Dispatch;
        DmfObject->InternalCallbacksInternal = DmfCallbacksInternal_Internal_Dispatch;
    }
    else if (DmfObject->ModuleDescriptor.ModuleOptions & DMF_MODULE_OPTIONS_PASSIVE)
    {
        // For Modules that run at Passive Level.
        //
        DmfAssert(! (DmfObject->ModuleDescriptor.ModuleOptions & DMF_MODULE_OPTIONS_DISPATCH));
        DmfObject->InternalCallbacksDmf = DmfCallbacksDmf_Internal_Passive;
        DmfObject->InternalCallbacksWdf = DmfCallbacksWdf_Internal_Passive;
        DmfObject->InternalCallbacksInternal = DmfCallbacksInternal_Internal_Passive;
    }
    else
    {
        DmfAssert(FALSE);
    }

    // Set default callbacks.
    //
    if (USE_GENERIC_CALLBACK == DmfObject->Callbacks.EvtModuleOnDeviceNotificationPostOpen)
    {
        DmfObject->Callbacks.EvtModuleOnDeviceNotificationPostOpen = EVT_DMF_MODULE_Generic_OnDeviceNotificationPostOpen;
    }
    if (USE_GENERIC_CALLBACK == DmfObject->Callbacks.EvtModuleOnDeviceNotificationPreClose)
    {
        DmfObject->Callbacks.EvtModuleOnDeviceNotificationPreClose = EVT_DMF_MODULE_Generic_OnDeviceNotificationPreClose;
    }

    // Allow client to override default behavior of each handler.
    //
    if (ModuleDescriptor->CallbacksWdf != NULL)
    {
        if (ModuleDescriptor->CallbacksWdf->ModulePrepareHardware != USE_GENERIC_ENTRYPOINT)
        {
            DmfObject->ModuleDescriptor.CallbacksWdf->ModulePrepareHardware = ModuleDescriptor->CallbacksWdf->ModulePrepareHardware;
        }
        if (ModuleDescriptor->CallbacksWdf->ModuleReleaseHardware != USE_GENERIC_ENTRYPOINT)
        {
            DmfObject->ModuleDescriptor.CallbacksWdf->ModuleReleaseHardware = ModuleDescriptor->CallbacksWdf->ModuleReleaseHardware;
        }
        if (ModuleDescriptor->CallbacksWdf->ModuleD0Entry != USE_GENERIC_ENTRYPOINT)
        {
            DmfObject->ModuleDescriptor.CallbacksWdf->ModuleD0Entry = ModuleDescriptor->CallbacksWdf->ModuleD0Entry;
        }
        if (ModuleDescriptor->CallbacksWdf->ModuleD0EntryPostInterruptsEnabled != USE_GENERIC_ENTRYPOINT)
        {
            DmfObject->ModuleDescriptor.CallbacksWdf->ModuleD0EntryPostInterruptsEnabled = ModuleDescriptor->CallbacksWdf->ModuleD0EntryPostInterruptsEnabled;
        }
        if (ModuleDescriptor->CallbacksWdf->ModuleD0ExitPreInterruptsDisabled != USE_GENERIC_ENTRYPOINT)
        {
            DmfObject->ModuleDescriptor.CallbacksWdf->ModuleD0ExitPreInterruptsDisabled = ModuleDescriptor->CallbacksWdf->ModuleD0ExitPreInterruptsDisabled;
        }
        if (ModuleDescriptor->CallbacksWdf->ModuleD0Exit != USE_GENERIC_ENTRYPOINT)
        {
            DmfObject->ModuleDescriptor.CallbacksWdf->ModuleD0Exit = ModuleDescriptor->CallbacksWdf->ModuleD0Exit;
        }
        if (ModuleDescriptor->CallbacksWdf->ModuleQueueIoRead != USE_GENERIC_ENTRYPOINT)
        {
            DmfObject->ModuleDescriptor.CallbacksWdf->ModuleQueueIoRead = ModuleDescriptor->CallbacksWdf->ModuleQueueIoRead;
        }
        if (ModuleDescriptor->CallbacksWdf->ModuleQueueIoWrite != USE_GENERIC_ENTRYPOINT)
        {
            DmfObject->ModuleDescriptor.CallbacksWdf->ModuleQueueIoWrite = ModuleDescriptor->CallbacksWdf->ModuleQueueIoWrite;
        }
        if (ModuleDescriptor->CallbacksWdf->ModuleDeviceIoControl != USE_GENERIC_ENTRYPOINT)
        {
            DmfObject->ModuleDescriptor.CallbacksWdf->ModuleDeviceIoControl = ModuleDescriptor->CallbacksWdf->ModuleDeviceIoControl;
        }
        if (ModuleDescriptor->CallbacksWdf->ModuleInternalDeviceIoControl != USE_GENERIC_ENTRYPOINT)
        {
            DmfObject->ModuleDescriptor.CallbacksWdf->ModuleInternalDeviceIoControl = ModuleDescriptor->CallbacksWdf->ModuleInternalDeviceIoControl;
        }
        if (ModuleDescriptor->CallbacksWdf->ModuleSelfManagedIoCleanup != USE_GENERIC_ENTRYPOINT)
        {
            DmfObject->ModuleDescriptor.CallbacksWdf->ModuleSelfManagedIoCleanup = ModuleDescriptor->CallbacksWdf->ModuleSelfManagedIoCleanup;
        }
        if (ModuleDescriptor->CallbacksWdf->ModuleSelfManagedIoFlush != USE_GENERIC_ENTRYPOINT)
        {
            DmfObject->ModuleDescriptor.CallbacksWdf->ModuleSelfManagedIoFlush = ModuleDescriptor->CallbacksWdf->ModuleSelfManagedIoFlush;
        }
        if (ModuleDescriptor->CallbacksWdf->ModuleSelfManagedIoInit != USE_GENERIC_ENTRYPOINT)
        {
            DmfObject->ModuleDescriptor.CallbacksWdf->ModuleSelfManagedIoInit = ModuleDescriptor->CallbacksWdf->ModuleSelfManagedIoInit;
        }
        if (ModuleDescriptor->CallbacksWdf->ModuleSelfManagedIoSuspend != USE_GENERIC_ENTRYPOINT)
        {
            DmfObject->ModuleDescriptor.CallbacksWdf->ModuleSelfManagedIoSuspend = ModuleDescriptor->CallbacksWdf->ModuleSelfManagedIoSuspend;
        }
        if (ModuleDescriptor->CallbacksWdf->ModuleSelfManagedIoRestart != USE_GENERIC_ENTRYPOINT)
        {
            DmfObject->ModuleDescriptor.CallbacksWdf->ModuleSelfManagedIoRestart = ModuleDescriptor->CallbacksWdf->ModuleSelfManagedIoRestart;
        }
        if (ModuleDescriptor->CallbacksWdf->ModuleSurpriseRemoval != USE_GENERIC_ENTRYPOINT)
        {
            DmfObject->ModuleDescriptor.CallbacksWdf->ModuleSurpriseRemoval = ModuleDescriptor->CallbacksWdf->ModuleSurpriseRemoval;
        }
        if (ModuleDescriptor->CallbacksWdf->ModuleQueryRemove != USE_GENERIC_ENTRYPOINT)
        {
            DmfObject->ModuleDescriptor.CallbacksWdf->ModuleQueryRemove = ModuleDescriptor->CallbacksWdf->ModuleQueryRemove;
        }
        if (ModuleDescriptor->CallbacksWdf->ModuleQueryStop != USE_GENERIC_ENTRYPOINT)
        {
            DmfObject->ModuleDescriptor.CallbacksWdf->ModuleQueryStop = ModuleDescriptor->CallbacksWdf->ModuleQueryStop;
        }
        if (ModuleDescriptor->CallbacksWdf->ModuleRelationsQuery != USE_GENERIC_ENTRYPOINT)
        {
            DmfObject->ModuleDescriptor.CallbacksWdf->ModuleRelationsQuery = ModuleDescriptor->CallbacksWdf->ModuleRelationsQuery;
        }
        if (ModuleDescriptor->CallbacksWdf->ModuleUsageNotificationEx != USE_GENERIC_ENTRYPOINT)
        {
            DmfObject->ModuleDescriptor.CallbacksWdf->ModuleUsageNotificationEx = ModuleDescriptor->CallbacksWdf->ModuleUsageNotificationEx;
        }
        if (ModuleDescriptor->CallbacksWdf->ModuleArmWakeFromS0 != USE_GENERIC_ENTRYPOINT)
        {
            DmfObject->ModuleDescriptor.CallbacksWdf->ModuleArmWakeFromS0 = ModuleDescriptor->CallbacksWdf->ModuleArmWakeFromS0;
        }
        if (ModuleDescriptor->CallbacksWdf->ModuleDisarmWakeFromS0 != USE_GENERIC_ENTRYPOINT)
        {
            DmfObject->ModuleDescriptor.CallbacksWdf->ModuleDisarmWakeFromS0 = ModuleDescriptor->CallbacksWdf->ModuleDisarmWakeFromS0;
        }
        if (ModuleDescriptor->CallbacksWdf->ModuleWakeFromS0Triggered != USE_GENERIC_ENTRYPOINT)
        {
            DmfObject->ModuleDescriptor.CallbacksWdf->ModuleWakeFromS0Triggered = ModuleDescriptor->CallbacksWdf->ModuleWakeFromS0Triggered;
        }
        if (ModuleDescriptor->CallbacksWdf->ModuleArmWakeFromSxWithReason != USE_GENERIC_ENTRYPOINT)
        {
            DmfObject->ModuleDescriptor.CallbacksWdf->ModuleArmWakeFromSxWithReason = ModuleDescriptor->CallbacksWdf->ModuleArmWakeFromSxWithReason;
        }
        if (ModuleDescriptor->CallbacksWdf->ModuleDisarmWakeFromSx != USE_GENERIC_ENTRYPOINT)
        {
            DmfObject->ModuleDescriptor.CallbacksWdf->ModuleDisarmWakeFromSx = ModuleDescriptor->CallbacksWdf->ModuleDisarmWakeFromSx;
        }
        if (ModuleDescriptor->CallbacksWdf->ModuleWakeFromSxTriggered != USE_GENERIC_ENTRYPOINT)
        {
            DmfObject->ModuleDescriptor.CallbacksWdf->ModuleWakeFromSxTriggered = ModuleDescriptor->CallbacksWdf->ModuleWakeFromSxTriggered;
        }
        if (ModuleDescriptor->CallbacksWdf->ModuleFileCreate != USE_GENERIC_ENTRYPOINT)
        {
            DmfObject->ModuleDescriptor.CallbacksWdf->ModuleFileCreate = ModuleDescriptor->CallbacksWdf->ModuleFileCreate;
        }
        if (ModuleDescriptor->CallbacksWdf->ModuleFileCleanup != USE_GENERIC_ENTRYPOINT)
        {
            DmfObject->ModuleDescriptor.CallbacksWdf->ModuleFileCleanup = ModuleDescriptor->CallbacksWdf->ModuleFileCleanup;
        }
        if (ModuleDescriptor->CallbacksWdf->ModuleFileClose != USE_GENERIC_ENTRYPOINT)
        {
            DmfObject->ModuleDescriptor.CallbacksWdf->ModuleFileClose = ModuleDescriptor->CallbacksWdf->ModuleFileClose;
        }
    }

    if (ModuleDescriptor->CallbacksDmf != NULL)
    {
        if (ModuleDescriptor->CallbacksDmf->ModuleInstanceDestroy != USE_GENERIC_ENTRYPOINT)
        {
            DmfObject->ModuleDescriptor.CallbacksDmf->ModuleInstanceDestroy = ModuleDescriptor->CallbacksDmf->ModuleInstanceDestroy;
        }
        if (ModuleDescriptor->CallbacksDmf->DeviceResourcesAssign != USE_GENERIC_ENTRYPOINT)
        {
            DmfObject->ModuleDescriptor.CallbacksDmf->DeviceResourcesAssign = ModuleDescriptor->CallbacksDmf->DeviceResourcesAssign;
        }
        if (ModuleDescriptor->CallbacksDmf->DeviceNotificationRegister != USE_GENERIC_ENTRYPOINT)
        {
            DmfObject->ModuleDescriptor.CallbacksDmf->DeviceNotificationRegister = ModuleDescriptor->CallbacksDmf->DeviceNotificationRegister;
        }
        if (ModuleDescriptor->CallbacksDmf->DeviceNotificationUnregister != USE_GENERIC_ENTRYPOINT)
        {
            DmfObject->ModuleDescriptor.CallbacksDmf->DeviceNotificationUnregister = ModuleDescriptor->CallbacksDmf->DeviceNotificationUnregister;
        }
        if (ModuleDescriptor->CallbacksDmf->DeviceOpen != USE_GENERIC_ENTRYPOINT)
        {
            DmfObject->ModuleDescriptor.CallbacksDmf->DeviceOpen = ModuleDescriptor->CallbacksDmf->DeviceOpen;
        }
        if (ModuleDescriptor->CallbacksDmf->DeviceClose != USE_GENERIC_ENTRYPOINT)
        {
            DmfObject->ModuleDescriptor.CallbacksDmf->DeviceClose = ModuleDescriptor->CallbacksDmf->DeviceClose;
        }
        if (ModuleDescriptor->CallbacksDmf->ChildModulesAdd != USE_GENERIC_ENTRYPOINT)
        {
            DmfObject->ModuleDescriptor.CallbacksDmf->ChildModulesAdd = ModuleDescriptor->CallbacksDmf->ChildModulesAdd;
        }
        // NOTE: Lock and Unlock callbacks may not be overridden.
        //
    }

    DmfObject->ModuleDescriptor.WdfAddCustomType = ModuleDescriptor->WdfAddCustomType;
    DmfAssert(DmfObject->ModuleDescriptor.WdfAddCustomType != NULL);

    // Handlers are always set. We don't need to check pointers everywhere.
    //
    DmfAssert(DmfObject->Callbacks.EvtModuleOnDeviceNotificationPostOpen != NULL);
    DmfAssert(DmfObject->Callbacks.EvtModuleOnDeviceNotificationPreClose != NULL);

    DmfAssert(DmfObject->ModuleDescriptor.CallbacksDmf->ModuleInstanceDestroy != NULL);
    DmfAssert(DmfObject->ModuleDescriptor.CallbacksWdf->ModulePrepareHardware != NULL);
    DmfAssert(DmfObject->ModuleDescriptor.CallbacksWdf->ModuleReleaseHardware != NULL);
    DmfAssert(DmfObject->ModuleDescriptor.CallbacksWdf->ModuleD0Entry != NULL);
    DmfAssert(DmfObject->ModuleDescriptor.CallbacksWdf->ModuleD0EntryPostInterruptsEnabled != NULL);
    DmfAssert(DmfObject->ModuleDescriptor.CallbacksWdf->ModuleD0ExitPreInterruptsDisabled != NULL);
    DmfAssert(DmfObject->ModuleDescriptor.CallbacksWdf->ModuleD0Exit != NULL);
    DmfAssert(DmfObject->ModuleDescriptor.CallbacksWdf->ModuleQueueIoRead != NULL);
    DmfAssert(DmfObject->ModuleDescriptor.CallbacksWdf->ModuleQueueIoWrite != NULL);
    DmfAssert(DmfObject->ModuleDescriptor.CallbacksWdf->ModuleDeviceIoControl != NULL);
    DmfAssert(DmfObject->ModuleDescriptor.CallbacksWdf->ModuleInternalDeviceIoControl != NULL);
    DmfAssert(DmfObject->ModuleDescriptor.CallbacksWdf->ModuleSelfManagedIoCleanup != NULL);
    DmfAssert(DmfObject->ModuleDescriptor.CallbacksWdf->ModuleSelfManagedIoFlush != NULL);
    DmfAssert(DmfObject->ModuleDescriptor.CallbacksWdf->ModuleSelfManagedIoInit != NULL);
    DmfAssert(DmfObject->ModuleDescriptor.CallbacksWdf->ModuleSelfManagedIoSuspend != NULL);
    DmfAssert(DmfObject->ModuleDescriptor.CallbacksWdf->ModuleSelfManagedIoRestart != NULL);
    DmfAssert(DmfObject->ModuleDescriptor.CallbacksWdf->ModuleSurpriseRemoval != NULL);
    DmfAssert(DmfObject->ModuleDescriptor.CallbacksWdf->ModuleQueryRemove != NULL);
    DmfAssert(DmfObject->ModuleDescriptor.CallbacksWdf->ModuleQueryStop != NULL);
    DmfAssert(DmfObject->ModuleDescriptor.CallbacksWdf->ModuleRelationsQuery != NULL);
    DmfAssert(DmfObject->ModuleDescriptor.CallbacksWdf->ModuleUsageNotificationEx != NULL);
    DmfAssert(DmfObject->ModuleDescriptor.CallbacksWdf->ModuleArmWakeFromS0 != NULL);
    DmfAssert(DmfObject->ModuleDescriptor.CallbacksWdf->ModuleDisarmWakeFromS0 != NULL);
    DmfAssert(DmfObject->ModuleDescriptor.CallbacksWdf->ModuleWakeFromS0Triggered != NULL);
    DmfAssert(DmfObject->ModuleDescriptor.CallbacksWdf->ModuleArmWakeFromSxWithReason != NULL);
    DmfAssert(DmfObject->ModuleDescriptor.CallbacksWdf->ModuleDisarmWakeFromSx != NULL);
    DmfAssert(DmfObject->ModuleDescriptor.CallbacksWdf->ModuleWakeFromSxTriggered != NULL);
    DmfAssert(DmfObject->ModuleDescriptor.CallbacksWdf->ModuleFileCreate != NULL);
    DmfAssert(DmfObject->ModuleDescriptor.CallbacksWdf->ModuleFileCleanup != NULL);
    DmfAssert(DmfObject->ModuleDescriptor.CallbacksWdf->ModuleFileClose != NULL);

    DmfAssert(DmfObject->ModuleDescriptor.CallbacksDmf->DeviceResourcesAssign != NULL);
    DmfAssert(DmfObject->ModuleDescriptor.CallbacksDmf->DeviceNotificationRegister != NULL);
    DmfAssert(DmfObject->ModuleDescriptor.CallbacksDmf->DeviceNotificationUnregister != NULL);
    DmfAssert(DmfObject->ModuleDescriptor.CallbacksDmf->DeviceOpen != NULL);
    DmfAssert(DmfObject->ModuleDescriptor.CallbacksDmf->DeviceClose != NULL);

    DmfAssert(DmfObject->InternalCallbacksDmf.ModuleInstanceDestroy != NULL);
    DmfAssert(DmfObject->InternalCallbacksWdf.ModulePrepareHardware != NULL);
    DmfAssert(DmfObject->InternalCallbacksWdf.ModuleReleaseHardware != NULL);
    DmfAssert(DmfObject->InternalCallbacksWdf.ModuleD0Entry != NULL);
    DmfAssert(DmfObject->InternalCallbacksWdf.ModuleD0EntryPostInterruptsEnabled != NULL);
    DmfAssert(DmfObject->InternalCallbacksWdf.ModuleD0ExitPreInterruptsDisabled != NULL);
    DmfAssert(DmfObject->InternalCallbacksWdf.ModuleD0Exit != NULL);
    DmfAssert(DmfObject->InternalCallbacksWdf.ModuleQueueIoRead != NULL);
    DmfAssert(DmfObject->InternalCallbacksWdf.ModuleQueueIoWrite != NULL);
    DmfAssert(DmfObject->InternalCallbacksWdf.ModuleDeviceIoControl != NULL);
    DmfAssert(DmfObject->InternalCallbacksWdf.ModuleInternalDeviceIoControl != NULL);
    DmfAssert(DmfObject->InternalCallbacksWdf.ModuleSelfManagedIoCleanup != NULL);
    DmfAssert(DmfObject->InternalCallbacksWdf.ModuleSelfManagedIoFlush != NULL);
    DmfAssert(DmfObject->InternalCallbacksWdf.ModuleSelfManagedIoInit != NULL);
    DmfAssert(DmfObject->InternalCallbacksWdf.ModuleSelfManagedIoSuspend != NULL);
    DmfAssert(DmfObject->InternalCallbacksWdf.ModuleSelfManagedIoRestart != NULL);
    DmfAssert(DmfObject->InternalCallbacksWdf.ModuleSurpriseRemoval != NULL);
    DmfAssert(DmfObject->InternalCallbacksWdf.ModuleQueryRemove != NULL);
    DmfAssert(DmfObject->InternalCallbacksWdf.ModuleQueryStop != NULL);
    DmfAssert(DmfObject->InternalCallbacksWdf.ModuleRelationsQuery != NULL);
    DmfAssert(DmfObject->InternalCallbacksWdf.ModuleUsageNotificationEx != NULL);
    DmfAssert(DmfObject->InternalCallbacksWdf.ModuleArmWakeFromS0 != NULL);
    DmfAssert(DmfObject->InternalCallbacksWdf.ModuleDisarmWakeFromS0 != NULL);
    DmfAssert(DmfObject->InternalCallbacksWdf.ModuleWakeFromS0Triggered != NULL);
    DmfAssert(DmfObject->InternalCallbacksWdf.ModuleArmWakeFromSxWithReason != NULL);
    DmfAssert(DmfObject->InternalCallbacksWdf.ModuleDisarmWakeFromSx != NULL);
    DmfAssert(DmfObject->InternalCallbacksWdf.ModuleWakeFromSxTriggered != NULL);
    DmfAssert(DmfObject->InternalCallbacksWdf.ModuleFileCreate != NULL);
    DmfAssert(DmfObject->InternalCallbacksWdf.ModuleFileCleanup != NULL);
    DmfAssert(DmfObject->InternalCallbacksWdf.ModuleFileClose != NULL);

    DmfAssert(DmfObject->InternalCallbacksDmf.DeviceResourcesAssign != NULL);
    DmfAssert(DmfObject->InternalCallbacksDmf.DeviceNotificationRegister != NULL);
    DmfAssert(DmfObject->InternalCallbacksDmf.DeviceNotificationUnregister != NULL);
    DmfAssert(DmfObject->InternalCallbacksDmf.DeviceOpen != NULL);
    DmfAssert(DmfObject->InternalCallbacksDmf.DeviceClose != NULL);
    DmfAssert(DmfObject->InternalCallbacksInternal.DefaultLock != NULL);
    DmfAssert(DmfObject->InternalCallbacksInternal.DefaultUnlock != NULL);
    DmfAssert(DmfObject->InternalCallbacksInternal.AuxiliaryLock != NULL);
    DmfAssert(DmfObject->InternalCallbacksInternal.AuxiliaryUnlock != NULL);

Exit:

    return ntStatus;
}
#pragma code_seg()

#pragma code_seg("PAGE")
static
_Must_inspect_result_
NTSTATUS
DmfModuleParentUpdate(
    _In_ WDFDEVICE Device,
    _Inout_ WDFOBJECT ParentObject,
    _Inout_ DMF_OBJECT* DmfObject,
    _In_ DMF_MODULE_ATTRIBUTES* DmfModuleAttributes,
    _Out_ BOOLEAN* AddedToParentChildModuleList
    )
/*++

Routine Description:

    Updates Parent-Child references when a Child Module is created.

Arguments:

    Device - The given WDFDEVICE object.
    ParentObject - The given Parent Object.
    DmfObject - The given DMF_OBJECT structure of a child DMF Module.
    DmfModuleAttributes - Pointer to the initialized DMF_MODULE_ATTRIBUTES structure.
    AddedToParentChildModuleList - Tells caller if Module has been added to Parent's Child Module list.

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS ntStatus;
    DMFMODULE dmfModuleParent;
    DMF_OBJECT* dmfObjectParent;

    PAGED_CODE();

    UNREFERENCED_PARAMETER(Device);
    
    ntStatus = STATUS_SUCCESS;
    *AddedToParentChildModuleList = FALSE;

    dmfModuleParent = (DMFMODULE)ParentObject;
    DmfAssert(dmfModuleParent != NULL);

    dmfObjectParent = DMF_ModuleToObject(dmfModuleParent);
    DmfAssert(dmfObjectParent != NULL);
    
    DmfAssert(Device == DMF_ParentDeviceGet(dmfModuleParent));

    // Add the Child Module to the list of the Parent Module's children
    // if its not a Dynamic Module. The lifetime of the Dynamic Module is
    // managed by the Client. 
    //
    if (!DmfModuleAttributes->DynamicModuleImmediate)
    {
        // NOTE: These values are expected to be NULL because the Parent
        //       has not initialized the ModuleCollection yet. (It cannot
        //       because that pointer is not passed to the Instance Creation
        //       function. Perhaps later we modify the Instance Creation 
        //       function to accept it. It is not necessary for proper
        //       functioning of the drivers, however. These asserts are 
        //       here to ensure that we all know this is "by design".
        //
        DmfAssert(NULL == DmfObject->ModuleCollection);
        DmfAssert(NULL == dmfObjectParent->ModuleCollection);

        InsertTailList(&dmfObjectParent->ChildObjectList,
                       &DmfObject->ChildListEntry);

        // Increment the Number of Child Modules.
        //
        dmfObjectParent->NumberOfChildModules += 1;

        // Tell caller Module has been added to list so if rest of Module
        // initialization fails, it can be immediately removed.
        //
        *AddedToParentChildModuleList = TRUE;

        // Indicate Module has been added to list so it can be removed from the
        // list when the Module is destroyed.
        //
        DmfObject->AddedToParentChildModuleList = TRUE;

        TraceEvents(TRACE_LEVEL_INFORMATION, DMF_TRACE, "Add Child=0x%p Parent=0x%p NumberOfChildModules=%d", DmfObject->MemoryDmfObject, dmfObjectParent->MemoryDmfObject, dmfObjectParent->NumberOfChildModules);
    }

    // Save the Parent in the Child.
    //
    DmfAssert(NULL == DmfObject->DmfObjectParent);
    DmfObject->DmfObjectParent = dmfObjectParent;

    // Perform operations when this Module is instantiated as a Transport Module.
    //
    if (DmfObject->IsTransport)
    {
        DmfAssert(dmfObjectParent->ModuleDescriptor.ModuleOptions & DMF_MODULE_OPTIONS_TRANSPORT_REQUIRED);
#if DBG
        GUID zeroGuid;

        RtlZeroMemory(&zeroGuid,
                      sizeof(zeroGuid));
        DmfAssert(!DMF_Utility_IsEqualGUID(&zeroGuid,
                                           &DmfObject->ModuleDescriptor.SupportedTransportInterfaceGuid));
        DmfAssert(!DMF_Utility_IsEqualGUID(&zeroGuid,
                                           &dmfObjectParent->ModuleDescriptor.RequiredTransportInterfaceGuid));
#endif
        // The Child's supported interface GUID must match the Parent's desired interface GUID.
        //
        if (DMF_Utility_IsEqualGUID(&DmfObject->ModuleDescriptor.SupportedTransportInterfaceGuid,
                                    &dmfObjectParent->ModuleDescriptor.RequiredTransportInterfaceGuid))
        {
            DMFMODULE parentDmfModule = DMF_ObjectToModule(dmfObjectParent);
            DMFMODULE childDmfModule = DMF_ObjectToModule(DmfObject);

            // Set the Parent's Transport Module to this Child Module.
            //
            DMF_ModuleTransportSet(parentDmfModule,
                                   childDmfModule);
        }
        else
        {
            // Attempted to connect incompatible transport interface.
            //
            DmfAssert(FALSE);
            ntStatus = STATUS_UNSUCCESSFUL;
            goto Exit;
        }
    }

Exit:

    return ntStatus;
}

#pragma code_seg("PAGE")
static
VOID
DmfModuleInFlightRecorderInitialize(
    _Inout_ DMF_OBJECT* DmfObject
    )
/*++

Routine Description:

    Populate InFlight Recorder data in a given DMF_CALLBACKS_DMF structure.

Arguments:

    DmfObject - The given DMF_OBJECT structure.
    
Return Value:

    None.

--*/
{
    PAGED_CODE();

#if defined(DMF_KERNEL_MODE)

    RECORDER_LOG recorder;
    BOOLEAN setDefaultWppRecorder;

    recorder = NULL;
    setDefaultWppRecorder = FALSE;

    if (DmfObject->ModuleDescriptor.InFlightRecorderSize > 0)
    {
        RECORDER_LOG_CREATE_PARAMS recorderCreateParams;
        NTSTATUS recorderStatus;
        
        RECORDER_LOG_CREATE_PARAMS_INIT(&recorderCreateParams,
                                        NULL);

        recorderCreateParams.TotalBufferSize = DmfObject->ModuleDescriptor.InFlightRecorderSize;

        RtlStringCbPrintfA(recorderCreateParams.LogIdentifier,
                           RECORDER_LOG_IDENTIFIER_MAX_CHARS,
                           "%s",
                           DmfObject->ClientModuleInstanceName);
        
        recorderStatus = WppRecorderLogCreate(&recorderCreateParams,
                                              &recorder);
        if (!NT_SUCCESS(recorderStatus))
        {
            TraceEvents(TRACE_LEVEL_ERROR, DMF_TRACE, "WppRecorderLogCreate fails: ntStatus=%!STATUS!", recorderStatus);

            // A new buffer could not be created. Check if the default log is available and set the Module's recorder handle to it 
            // to not miss capturing logs from this Module.
            //
            setDefaultWppRecorder = TRUE;
        }
    }
    else
    {
        // The Module's logs will be part of the default log if the Module chose to not have a separate custom buffer.
        //
        setDefaultWppRecorder = TRUE;
    }

    if (setDefaultWppRecorder)
    {
        if (!DMF_DmfDeviceInitIsWppRecorderDisabled())
        {
            if (WppRecorderIsDefaultLogAvailable())
            {
                recorder = WppRecorderLogGetDefault();
                // WppRecorder is in use.
                //
                DmfObject->UsingDefaultInFlightRecorder = TRUE;
            }
        }
    }

    DmfObject->InFlightRecorder = recorder;

#else

    UNREFERENCED_PARAMETER(DmfObject);

#endif
}

// This table defines the Generic Callbacks, some of which will be overridden by the
// DMF Modules.
//
static
DMF_MODULE_DESCRIPTOR
DmfModuleDescriptor_Generic =
{
    // Size of this structure.
    //
    sizeof(DMF_MODULE_DESCRIPTOR),
    // Module Name.
    //
    "Generic",
    // Options.
    //
    DMF_MODULE_OPTIONS_PASSIVE,
    // Open Option.
    //
    DMF_MODULE_OPEN_OPTION_OPEN_Create,
    // Module Config Size.
    //
    0,
    // DMF Callbacks.
    //
    NULL,
    // WDF Callbacks.
    //
    NULL,
    // BranchTrack Initialize Function.
    //
    NULL,
    // liveKernelDump Initialize Function.
    //
    NULL,
    // Number Of Auxiliary Locks.
    //
    0
};

///////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Module Creation/Destruction
//
///////////////////////////////////////////////////////////////////////////////////////////////////////
//

EVT_WDF_OBJECT_CONTEXT_CLEANUP DmfEvtDynamicModuleCleanupCallback;

#pragma code_seg("PAGE")
_Use_decl_annotations_
NTSTATUS
DMF_ModuleCreate(
    _In_ WDFDEVICE Device,
    _In_ DMF_MODULE_ATTRIBUTES* DmfModuleAttributes,
    _In_ PWDF_OBJECT_ATTRIBUTES DmfModuleObjectAttributes,
    _In_ DMF_MODULE_DESCRIPTOR* ModuleDescriptor,
    _Out_ DMFMODULE* DmfModule
    )
/*++

Routine Description:

    Creates an instance of a DMF Module. This code creates the DRIECT_HANDLE that will be
    subsequently used as an opaque handle by the Client Driver. This handle contains all the information
    needed by the Module to do its work because this function populates the handle here.

Arguments:

    Device - The given WDFDEVICE object.
    DmfModuleAttributes - Pointer to the initialized DMF_MODULE_ATTRIBUTES structure.
    DmfModuleObjectAttributes - Pointer to caller initialized WDF_OBJECT_ATTRIBUTES structure.
    ModuleDescriptor - Pointer to the DMF_MODULE_DESCRIPTOR structure providing information about the Module.
    DmfModule - (Output) Pointer to memory where this function will return the created DMF Module.

Return Value:

    DMF Module upon Success, or NULL upon failure.

--*/
{
    NTSTATUS ntStatus;
    WDFMEMORY memoryDmfObject;
    DMF_OBJECT* dmfObject;
    BOOLEAN isParentObjectDmfModule;
    DMFMODULE dmfModule;
    DMF_MODULE_COLLECTION_CONFIG moduleCollectionConfig;
    DMFCOLLECTION childModuleCollection;
    PFN_WDF_OBJECT_CONTEXT_CLEANUP clientEvtCleanupCallback;
    WDF_OBJECT_ATTRIBUTES copyOfDmfModuleObjectAttributes;
    WDFOBJECT parentObject;
    BOOLEAN addedToParentChildModuleList;

    PAGED_CODE();

    FuncEntry(DMF_TRACE);

    addedToParentChildModuleList = FALSE;

    // Ensure returned handle is set only if this call is successful.
    //
    if (DmfModule != NULL)
    {
        *DmfModule = NULL;
    }

    memoryDmfObject = NULL;
    dmfModule = NULL;
    dmfObject = NULL;
    childModuleCollection = NULL;
    clientEvtCleanupCallback = NULL;

    // Parent object of the DMFMODULE to create should always be set to one of the following:
    //
    //     1. WDFOBJECT (or inherited object)
    //     2. DMFMODULE (for Child Module)
    //     3. DMFCOLLECTION (for Module created as part of DMF Collection).
    //

    if ((WDF_NO_OBJECT_ATTRIBUTES == DmfModuleObjectAttributes) ||
        (NULL == DmfModuleObjectAttributes->ParentObject))
    {
        // Assign the default parent (Client Driver's WDFDEVICE) if no parent is specified.
        //
        parentObject = Device;
    }
    else
    {
        // Allow Client to specify any parent.
        // IMPORTANT: ParentObject should be set in a way that Object Clean Up callbacks happen
        //            in PASSIVE_LEVEL.
        //
        parentObject = DmfModuleObjectAttributes->ParentObject;
    }

    // In the case where Client Clean Up callback function is chained, it is necessary
    // to override the callers data. In order to not modify the copy the caller uses,
    // copy to local and override it there.
    // NOTE: Modifying Client's pointer can cause infinite recursion when clean up callbacks
    //       are called if the Client does not initialize WDF_OBJECT_ATTRIBUTES before
    //       every call. Copying here prevents that possibility, regardless of what caller does.
    //
    RtlCopyMemory(&copyOfDmfModuleObjectAttributes,
                  DmfModuleObjectAttributes,
                  sizeof(WDF_OBJECT_ATTRIBUTES));
    DmfModuleObjectAttributes = &copyOfDmfModuleObjectAttributes;

    // Don't create Dynamic Module if the Module supports WDF callbacks since those
    // callbacks might not happen and the Module will not execute as originally planned.
    //
    if ((DmfModuleAttributes->DynamicModule) &&
        (NULL != ModuleDescriptor->CallbacksWdf))
    {
        // If the Module supports any WDF callbacks that may have already happened, then prevent this Module from loading dynamically.
        // Allow Dynamic Modules that have WDF callbacks that are legitimate to process after the Module is loaded.
        //
        if ((NULL == ModuleDescriptor->CallbacksWdf->ModulePrepareHardware) &&
            (NULL == ModuleDescriptor->CallbacksWdf->ModuleReleaseHardware) &&
            (NULL == ModuleDescriptor->CallbacksWdf->ModuleD0Entry) &&
            (NULL == ModuleDescriptor->CallbacksWdf->ModuleD0Exit) &&
            (NULL == ModuleDescriptor->CallbacksWdf->ModuleD0EntryPostInterruptsEnabled) &&
            (NULL == ModuleDescriptor->CallbacksWdf->ModuleD0ExitPreInterruptsDisabled) &&
            (NULL == ModuleDescriptor->CallbacksWdf->ModuleSelfManagedIoCleanup) &&
            (NULL == ModuleDescriptor->CallbacksWdf->ModuleSelfManagedIoFlush) &&
            (NULL == ModuleDescriptor->CallbacksWdf->ModuleSelfManagedIoInit) &&
            (NULL == ModuleDescriptor->CallbacksWdf->ModuleSelfManagedIoSuspend) &&
            (NULL == ModuleDescriptor->CallbacksWdf->ModuleSelfManagedIoRestart))
        {
            // Allow this Module to load dynamically.
            // This path is necessary for Modules like IoctlHandler to load Dynamically (or Dynamic Modules that have IoctlHanlder
            // as a Child Module).
            //
        }
        else
        {
            // One of the above callbacks may have already happened. Since the Module assumes it will handle those callbacks,
            // and it is unable to, don't allow it to be created.
            //
            DmfAssert(FALSE);
            ntStatus = STATUS_UNSUCCESSFUL;
            goto Exit;
        }
    }

    // Don't create Dynamic Module if the Module's Open Option depends on WDF callbacks since
    // those callbacks might not happen and the Module will not execute as originally planned.
    //
    if ((DmfModuleAttributes->DynamicModule) &&
        (ModuleDescriptor->OpenOption != DMF_MODULE_OPEN_OPTION_OPEN_Create &&
         ModuleDescriptor->OpenOption != DMF_MODULE_OPEN_OPTION_NOTIFY_Create))
    {
        DmfAssert(FALSE);
        ntStatus = STATUS_UNSUCCESSFUL;
        goto Exit;
    }

    // Chain the Client's clean up callback if the Module is created dynamically.
    // so that its Close callback is called when the Module is deleted.
    //
    if (DmfModuleAttributes->DynamicModuleImmediate)
    {
        clientEvtCleanupCallback = DmfModuleObjectAttributes->EvtCleanupCallback;
        DmfModuleObjectAttributes->EvtCleanupCallback = DmfEvtDynamicModuleCleanupCallback;
    }

    ntStatus = WdfMemoryCreate(DmfModuleObjectAttributes,
                               NonPagedPoolNx,
                               DMF_TAG4,
                               sizeof(DMF_OBJECT),
                               &memoryDmfObject,
                               (VOID**)&dmfObject);
    if (! NT_SUCCESS(ntStatus))
    {
        TraceEvents(TRACE_LEVEL_ERROR, DMF_TRACE, "Unable to allocate DMF_OBJECT");
        dmfObject = NULL;
        memoryDmfObject = NULL;
        goto Exit;
    }

    TraceEvents(TRACE_LEVEL_INFORMATION, DMF_TRACE, "Creating Module: Handle=0x%p Object=0x%p", memoryDmfObject, dmfObject);

    RtlZeroMemory(dmfObject,
                  sizeof(DMF_OBJECT));

    // IMPORTANT: Make sure the object is initialized before exiting so that deletion code
    //            will work properly in case subsequent allocations fail during stress.
    // NOTE: This is particularly an issue for Dynamic Modules because DmfEvtDynamicModuleCleanupCallback
    //       executes when the DMFOBJECT is deleted.
    //

    InitializeListHead(&dmfObject->ChildObjectList);
    dmfObject->MemoryDmfObject = memoryDmfObject;
    dmfObject->ParentDevice = Device;
    dmfObject->Signature = DMF_OBJECT_SIGNATURE;
    dmfObject->ModuleName = ModuleDescriptor->ModuleName;
    dmfObject->IsClosePending = FALSE;
    dmfObject->NeedToCallPreClose = FALSE;
    dmfObject->ClientEvtCleanupCallback = clientEvtCleanupCallback;
    dmfObject->IsTransport = DmfModuleAttributes->IsTransportModule;

    // Initialize the Module State.
    // This state is to prevent asserts during fault-injection or low memory conditions.
    //
    DmfAssert(ModuleState_Invalid == dmfObject->ModuleState);
    dmfObject->ModuleState = ModuleState_PreCreate;

    // Prevent Module's Close() handler from being called if it was never opened.
    //
    dmfObject->ModuleClosed = TRUE;

    RtlCopyMemory(&dmfObject->ModuleAttributes,
                  DmfModuleAttributes,
                  sizeof(DMF_MODULE_ATTRIBUTES));

    // Initialize the callbacks to generic handlers.
    //
    dmfObject->ModuleDescriptor = DmfModuleDescriptor_Generic;

    // Copy over the context sizes for debugging purposes. These values are not reused.
    //
    dmfObject->ModuleDescriptor.ModuleConfigSize = ModuleDescriptor->ModuleConfigSize;
    dmfObject->ModuleDescriptor.ModuleOptions = ModuleDescriptor->ModuleOptions;
    dmfObject->ModuleDescriptor.OpenOption = ModuleDescriptor->OpenOption;

    // Overwrite the BranchTrack Initialization function.
    //
    dmfObject->ModuleDescriptor.ModuleBranchTrackInitialize = ModuleDescriptor->ModuleBranchTrackInitialize;

    // Overwrite the LiveKernelDump Initialization function.
    //
    dmfObject->ModuleDescriptor.ModuleLiveKernelDumpInitialize = ModuleDescriptor->ModuleLiveKernelDumpInitialize;

    // Copy over the Module Transport Method and GUID.
    //
    dmfObject->ModuleDescriptor.ModuleTransportMethod = ModuleDescriptor->ModuleTransportMethod;

    // Copy the Protocol-Transport GUIDs.
    //
    dmfObject->ModuleDescriptor.RequiredTransportInterfaceGuid = ModuleDescriptor->RequiredTransportInterfaceGuid;
    dmfObject->ModuleDescriptor.SupportedTransportInterfaceGuid = ModuleDescriptor->SupportedTransportInterfaceGuid;

    // Overwrite the number of Auxiliary Locks needed.
    //
    dmfObject->ModuleDescriptor.NumberOfAuxiliaryLocks = ModuleDescriptor->NumberOfAuxiliaryLocks;

    // IMPORTANT: The object has now been initialized so deletion code will work in case there is a 
    //            subsequent failure.
    //

    if (ModuleDescriptor->ModuleContextAttributes != WDF_NO_OBJECT_ATTRIBUTES)
    {
        // Allocate Module Context.
        // NOTE: This (ModuleContext) pointer is used only for debugging purposes.
        //
        ntStatus = WdfObjectAllocateContext(memoryDmfObject,
                                            ModuleDescriptor->ModuleContextAttributes,
                                            &(dmfObject->ModuleContext));
        if (! NT_SUCCESS(ntStatus))
        {
            TraceEvents(TRACE_LEVEL_ERROR, DMF_TRACE, "WdfObjectAllocateContext fails: ntStatus=%!STATUS!", ntStatus);
            goto Exit;
        }
    }

    // Initialize Client Module Instance Name.
    //
    ntStatus = DmfModuleInstanceNameInitialize(dmfObject,
                                               memoryDmfObject,
                                               DmfModuleAttributes);
    if (!NT_SUCCESS(ntStatus))
    {
        TraceEvents(TRACE_LEVEL_ERROR, DMF_TRACE, "DmfModuleInstanceNameInitialize fails: ntStatus=%!STATUS!", ntStatus);
        goto Exit;
    }

    // Initialize child objects.
    //
    ntStatus = DmfModuleChildObjectsInitialize(dmfObject, 
                                               memoryDmfObject,
                                               DmfModuleAttributes,
                                               ModuleDescriptor);
    if (!NT_SUCCESS(ntStatus))
    {
        TraceEvents(TRACE_LEVEL_ERROR, DMF_TRACE, "DmfModuleChildObjectsInitialize fails: ntStatus=%!STATUS!", ntStatus);
        goto Exit;
    }

    // Create the auxiliary locks based on Module Options.
    //
    ntStatus = DMF_SynchronizationCreate(dmfObject,
                                         DmfModuleAttributes->PassiveLevel);
    if (!NT_SUCCESS(ntStatus))
    {
        TraceEvents(TRACE_LEVEL_ERROR, DMF_TRACE, "DMF_SynchronizationCreate fails: ntStatus=%!STATUS!", ntStatus);
        goto Exit;
    }

    // Create the event that tells DMF it is OK to destroy the Module because it is either "created" or "closed".
    //
    ntStatus = DMF_Portable_EventCreate(&dmfObject->ModuleCanBeDeletedEvent,
                                        NotificationEvent,
                                        TRUE);
    if (!NT_SUCCESS(ntStatus))
    {
        TraceEvents(TRACE_LEVEL_ERROR, DMF_TRACE, "DMF_Portable_EventCreate fails: ntStatus=%!STATUS!", ntStatus);
        goto Exit;
    }

    // Copy the In Flight Recorder size.
    //
    dmfObject->ModuleDescriptor.InFlightRecorderSize = ModuleDescriptor->InFlightRecorderSize;

    // Initialize Callbacks.
    //
    ntStatus = DmfModuleCallbacksInitialize(dmfObject,
                                            DmfModuleAttributes,
                                            ModuleDescriptor);
    if (!NT_SUCCESS(ntStatus))
    {
        TraceEvents(TRACE_LEVEL_ERROR, DMF_TRACE, "DmfModuleObjectCallbacksInitialize fails: ntStatus=%!STATUS!", ntStatus);
        goto Exit;
    }

    // Initialize the Module State as "created".
    //
    DmfAssert(ModuleState_PreCreate == dmfObject->ModuleState);
    dmfObject->ModuleState = ModuleState_Created;

    // Check if ParentObject is of type DMFMODULE.
    // If it is and it is not a Dynamic Module, add this Module
    // to the Parent Module's list of children and update its Parent.
    // Otherwise, just set the Module's Parent.
    //
    isParentObjectDmfModule = WdfObjectIsCustomType(parentObject,
                                                    DMFMODULE_TYPE);
    if (isParentObjectDmfModule)
    {
        ntStatus = DmfModuleParentUpdate(Device,
                                         parentObject,
                                         dmfObject,
                                         DmfModuleAttributes,
                                         &addedToParentChildModuleList);
        if (!NT_SUCCESS(ntStatus))
        {
            TraceEvents(TRACE_LEVEL_ERROR, DMF_TRACE, "DmfModuleParentUpdate fails: ntStatus=%!STATUS!", ntStatus);
            goto Exit;
        }
    }

    // Ensure that Parent DMFMODULE is accessible until after
    // Child is deleted so that Child can access Parent Module's
    // WDFOBJECT context while running down.
    // 
    // NOTE: This is necessary only for a Dynamic Module since it
    // can be deleted any time by WDF when its parent object is
    // deleted.
    //
    if (DmfModuleAttributes->DynamicModuleImmediate)
    {
        // Remember the object that needs to be dereferenced when this Module's
        // memory is deleted.
        //
        dmfObject->ObjectToDereference = parentObject;
        // This is the call where the dereference happens (the object clean up callback).
        //
        DmfAssert(DmfModuleObjectAttributes->EvtCleanupCallback == DmfEvtDynamicModuleCleanupCallback);
        // Add the reference now. Tag it so that the exact dereference can be verified.
        //
        WdfObjectReferenceWithTag(parentObject,
                                  DMF_TAG_DYNAMIC_MODULE_REFERENCE);
    }

    dmfModule = (DMFMODULE)memoryDmfObject;
    ntStatus = WdfObjectAddCustomType(dmfModule,
                                      DMFMODULE_TYPE);
    if (! NT_SUCCESS(ntStatus))
    {
        TraceEvents(TRACE_LEVEL_ERROR, DMF_TRACE, "WdfObjectAddCustomType fails: ntStatus=%!STATUS!", ntStatus);
        goto Exit;
    }

    // Initialize InFlight recorder.
    //
    DmfModuleInFlightRecorderInitialize(dmfObject);

    // Create child Modules
    // Prepare to create a Module Collection.
    //
    DMF_MODULE_COLLECTION_CONFIG_INIT(&moduleCollectionConfig,
                                      NULL,
                                      NULL,
                                      Device);
    moduleCollectionConfig.DmfPrivate.ParentDmfModule = dmfModule;
    dmfObject->ModuleDescriptor.CallbacksDmf->ChildModulesAdd(dmfModule,
                                                              DmfModuleAttributes,
                                                              (PDMFMODULE_INIT)&moduleCollectionConfig);
    // Check for failure to allocate memory inside the above call, which, by design
    // returns nothing.
    //
    if (!NT_SUCCESS(moduleCollectionConfig.DmfPrivate.ErrorCodeNtStatus))
    {
        ntStatus = moduleCollectionConfig.DmfPrivate.ErrorCodeNtStatus;
        TraceEvents(TRACE_LEVEL_ERROR, DMF_TRACE, "ChildModulesAdd fails: ntStatus=%!STATUS!", ntStatus);
        goto Exit;
    }

    // Allow the Client to set a Transport if it is required.
    //
    if (dmfObject->ModuleDescriptor.ModuleOptions & DMF_MODULE_OPTIONS_TRANSPORT_REQUIRED)
    {
        DmfAssert(DmfModuleAttributes->TransportModuleAdd != NULL);
        // Indicate that all Modules added here are Transport Modules.
        //
        moduleCollectionConfig.DmfPrivate.IsTransportModule = TRUE;
        DmfModuleAttributes->TransportModuleAdd(dmfModule,
                                                DmfModuleAttributes,
                                                (PDMFMODULE_INIT)&moduleCollectionConfig);
    }

    if (moduleCollectionConfig.DmfPrivate.ListOfConfigs != NULL)
    {
        // 'local variable is initialized but not referenced'
        // Lets keep this assert. Also, do not call functions in ASSERTs in case it causes a 
        // difference between Debug/Release builds.
        //
        #pragma warning(suppress:4189)
        ULONG numberOfClientModulesToCreate = WdfCollectionGetCount(moduleCollectionConfig.DmfPrivate.ListOfConfigs);
        DmfAssert(numberOfClientModulesToCreate > 0);
        // The attributes for all the Modules have been set. Create the Modules.
        //
        ntStatus = DMF_ModuleCollectionCreate(NULL,
                                              &moduleCollectionConfig,
                                              &childModuleCollection);
        if (!NT_SUCCESS(ntStatus))
        {
            TraceEvents(TRACE_LEVEL_ERROR, DMF_TRACE, "DMF_ModuleCollectionCreate fails: ntStatus=%!STATUS!", ntStatus);
            goto Exit;
        }

        // The childModuleCollection is transient just for the creation of the Child Modules
        // to use the existing Collection APIs. It is not required to store the collection since
        // the list of children is already maintained as part of Parent Module's DMF_OBJECT.
        //
        if (childModuleCollection != NULL)
        {
            WdfObjectDelete(childModuleCollection);
            childModuleCollection = NULL;
        }
    }

Exit:

    if (!NT_SUCCESS(ntStatus))
    {
        if (memoryDmfObject != NULL)
        {
            // If this Module is a Child Module remove it now from the list of Parent's Child Modules
            // because the Module will become invalid since the memory is about to be destroyed.
            // This makes it impossible for the Module to be used after being destroyed. This is 
            // especially important in the case of partial initialization of a Module Tree.
            //
            if (addedToParentChildModuleList)
            {
                // Decrement the Number of Child Modules.
                //
                DmfAssert(dmfObject->DmfObjectParent->NumberOfChildModules >= 1);
                dmfObject->DmfObjectParent->NumberOfChildModules -= 1;
                TraceEvents(TRACE_LEVEL_INFORMATION, DMF_TRACE,
                            "Remove Child=0x%p Parent=0x%p NumberOfChildModules=%d",
                            memoryDmfObject,
                            dmfObject->DmfObjectParent->MemoryDmfObject,
                            dmfObject->DmfObjectParent->NumberOfChildModules);
                RemoveEntryList(&dmfObject->ChildListEntry);
                dmfObject->AddedToParentChildModuleList = FALSE;
            }

            if (DmfModuleAttributes->DynamicModuleImmediate)
            {
                // This is necessary to prevent assert in case of fault-injection or low memory scenarios.
                //
                dmfObject->DynamicModuleImmediate = TRUE;
            }
            else
            {
                // Need to delete all non-WDF allocations for the non-Dynamic Module case.
                // This code runs in the case of fault-injection or low memory scenarios.
                // 
                // In Dynamic Module case, this clean up is handled in the cleanup callback
                // by DMF_ModuleDestroy.
                //
                if (dmfObject->ClientModuleInstanceName != NULL)
                {
                    DMF_GenericMemoryFree(dmfObject->ClientModuleInstanceName,
                                          DMF_TAG0);
                    dmfObject->ClientModuleInstanceName = NULL;
                }
                if (dmfObject->ModuleDescriptor.CallbacksDmf != NULL)
                {
                    DMF_GenericMemoryFree(dmfObject->ModuleDescriptor.CallbacksDmf,
                                          DMF_TAG2);
                    dmfObject->ModuleDescriptor.CallbacksDmf = NULL;
                }
                if (dmfObject->ModuleDescriptor.CallbacksWdf != NULL)
                {
                    DMF_GenericMemoryFree(dmfObject->ModuleDescriptor.CallbacksWdf,
                                          DMF_TAG3);
                    dmfObject->ModuleDescriptor.CallbacksWdf = NULL;
                }
                if (dmfObject->ModuleConfig != NULL)
                {
                    DMF_GenericMemoryFree(dmfObject->ModuleConfig,
                                          DMF_TAG1);
                    dmfObject->ModuleConfig = NULL;
                }
            }

            // All subsequent allocations after memoryDmfObject use memoryDmfObject as parent. So, this
            // call deletes all the allocations made.
            //
            TraceEvents(TRACE_LEVEL_INFORMATION, DMF_TRACE, "Cleanup Failed Module=0x%p", memoryDmfObject);
            WdfObjectDelete(memoryDmfObject);
            memoryDmfObject = NULL;
        }
        dmfObject = NULL;
    }

    // If the Module has been successfully created, perform final operations prior to returning.
    //
    if (dmfObject != NULL)
    {
        // Add Module name as a custom type for the newly created Module handle.
        // Data and callbacks are not used as part of the custom type. 
        //
        dmfObject->ModuleDescriptor.WdfAddCustomType(dmfModule,
                                                     0,
                                                     NULL,
                                                     NULL);
        DmfAssert(! dmfObject->DynamicModuleImmediate);
        // If this Module is a Dynamic Module or it is an immediate or non-immediate Child
        // of a Dynamic Module, open it now if it should be opened during Create.
        //
        if (DmfModuleAttributes->DynamicModuleImmediate)
        {
            // Dynamic Module Path:
            // Remember it is Dynamic Module so it can be automatically closed prior to destruction.
            //
            dmfObject->DynamicModuleImmediate = TRUE;

            // Give Client the resultant Module Handle:
            // PostOpen callback may need to compare contents of the address of the Module it
            // passed with the Module handle passed in the callback. So, set this now before 
            // PostOpen callback happens. It is unlikely that a Client may pass NULL when creating
            // a Dynamic Module, but it is possible so allow for that possibility.
            //
            if (DmfModule != NULL)
            {
                *DmfModule = dmfModule;
            }

            // Since it is a Dynamic Module, Open or register for Notification as specified by the Module's
            // Open Option.
            //
            ntStatus = DMF_Module_OpenOrRegisterNotificationOnCreate(dmfModule);
            if (!NT_SUCCESS(ntStatus))
            {
                TraceEvents(TRACE_LEVEL_ERROR, DMF_TRACE, "DMF_Module_OpenOrRegisterNotificationOnCreate fails: ntStatus=%!STATUS!", ntStatus);
                if (DmfModule != NULL)
                {
                    // Ensure returned handle is set only if this call is successful.
                    //
                    *DmfModule = NULL;
                }
                goto Exit;
            }
        }
        else
        {
            // Static Module path:
            // Return the Module handle if requested by Client.
            //
            if (DmfModule != NULL)
            {
                // Give Client the resultant Module Handle.
                //
                *DmfModule = dmfModule;
            }
        }
    }

    FuncExit(DMF_TRACE, "dmfObject=0x%p [%s]", dmfObject, (dmfObject != NULL) ? dmfObject->ClientModuleInstanceName : "");

    return ntStatus;
}
#pragma code_seg()

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
DMF_ModuleDestroy(
    _In_ DMFMODULE DmfModule,
    _In_ BOOLEAN DeleteMemory
    )
/*++

Routine Description:

    Destroys an instance of a DMF Module.

Arguments:

    DmfModule - This Module's handle.
    DeleteMemory - Indicates if this function is called from clean up callback. In that
                   case only the handles in the context should be deleted because WDF will
                   actually delete DmfModule.

Return Value:

    None

--*/
{
    DMF_OBJECT* dmfObject;

    dmfObject = DMF_ModuleToObject(DmfModule);

    FuncEntryArguments(DMF_TRACE, "DmfObject=0x%p [%s]", dmfObject, dmfObject->ClientModuleInstanceName);

    // Unbind all Interface Binding of this Module.
    //
    DMF_ModuleInterfacesUnbind(DmfModule);

    DMF_HandleValidate_Destroy(dmfObject);
    dmfObject->ModuleState = ModuleState_Destroying;

    DmfAssert(dmfObject->MemoryDmfObject != NULL);

    // NOTE: It can be NULL in cases of fault-injection or low memory.
    //
    if (dmfObject->ClientModuleInstanceName != NULL)
    {
        DMF_GenericMemoryFree(dmfObject->ClientModuleInstanceName,
                              DMF_TAG0);
        dmfObject->ClientModuleInstanceName = NULL;
    }

    // NOTE: It can be NULL in cases of fault-injection or low memory.
    //
    if (dmfObject->ModuleDescriptor.CallbacksDmf != NULL)
    {
        DMF_GenericMemoryFree(dmfObject->ModuleDescriptor.CallbacksDmf,
                              DMF_TAG2);
        dmfObject->ModuleDescriptor.CallbacksDmf = NULL;
    }

    // NOTE: It can be NULL in cases of fault-injection or low memory.
    //
    if (dmfObject->ModuleDescriptor.CallbacksWdf)
    {
        DMF_GenericMemoryFree(dmfObject->ModuleDescriptor.CallbacksWdf,
                              DMF_TAG3);
        dmfObject->ModuleDescriptor.CallbacksWdf = NULL;
    }

#if defined(DMF_KERNEL_MODE)
    if (dmfObject->InFlightRecorder != NULL)
    {
        if (! dmfObject->UsingDefaultInFlightRecorder)
        {
            WppRecorderLogDelete(dmfObject->InFlightRecorder);
        }
        dmfObject->InFlightRecorder = NULL;
    }
#endif // defined(DMF_KERNEL_MODE)

    if (dmfObject->ModuleConfig != NULL)
    {
        DMF_GenericMemoryFree(dmfObject->ModuleConfig,
                              DMF_TAG1);
        dmfObject->ModuleConfig = NULL;
    }
    else
    {
        // NOTE: Module Config Memory is optional.
        //
    }

    // This event must be manually deleted for User-mode.
    //
    DMF_Portable_EventClose(&dmfObject->ModuleCanBeDeletedEvent);

    // User-mode non-WDF versions need to clean up.
    //
    DMF_GenericSpinLockDestroy(&dmfObject->InterfaceBindingsLock);
    DMF_GenericSpinLockDestroy(&dmfObject->ReferenceCountLock);

    // Remove the Module from its Parent Module's Child Module list so that
    // there is no chance it can be used when then Parent is deleted (in the
    // Cleanup handler which looks at the Child Module list).
    //
    if (dmfObject->AddedToParentChildModuleList)
    {
        // Decrement the Number of Child Modules.
        //
        DmfAssert(dmfObject->DmfObjectParent->NumberOfChildModules >= 1);
        dmfObject->DmfObjectParent->NumberOfChildModules -= 1;
        TraceEvents(TRACE_LEVEL_INFORMATION, DMF_TRACE,
                    "Remove Child=0x%p Parent=0x%p NumberOfChildModules=%d",
                    dmfObject->MemoryDmfObject,
                    dmfObject->DmfObjectParent->MemoryDmfObject,
                    dmfObject->DmfObjectParent->NumberOfChildModules);
        RemoveEntryList(&dmfObject->ChildListEntry);
        dmfObject->AddedToParentChildModuleList = FALSE;
    }

    if (DeleteMemory)
    {
        WdfObjectDelete(dmfObject->MemoryDmfObject);
        // Memory associated with dmfObject is deleted here.
        // Thus, dmfObject->MemoryDmfObject is not set to NULL.
        //
        dmfObject = NULL;
    }

    FuncExitVoid(DMF_TRACE);
}

VOID
DMF_ModuleTransportSet(
    _In_ DMFMODULE DmfModule,
    _In_ DMFMODULE TransportDmfModule
    )
/*++

Routine Description:

    Given a Module, set its Transport Module to the given Transport Module.
    NOTE: For Legacy Protocol-Transport support only.

Arguments:

    DmfModule - The given Module.
    TransportDmfModule - The given Transport Module.

Return Value:

    None

--*/
{
    DMF_OBJECT* dmfObject;
    DMF_OBJECT* dmfObjectTransport;

    dmfObject = DMF_ModuleToObject(DmfModule);
    dmfObjectTransport = DMF_ModuleToObject(TransportDmfModule);
    DmfAssert(NULL == dmfObject->TransportModule);
    dmfObject->TransportModule = dmfObjectTransport;
}

DMFMODULE
DMF_ModuleTransportGet(
    _In_ DMFMODULE DmfModule
    )
/*++

Routine Description:

    Given a Module, get its Transport Module.

Arguments:

    DmfModule - The given Module.

Return Value:

    The given Module's Transport Module.

--*/
{
    DMFMODULE transportModule;
    DMF_OBJECT* dmfObject;

    dmfObject = DMF_ModuleToObject(DmfModule);
    DmfAssert(dmfObject->TransportModule != NULL);
    transportModule = DMF_ObjectToModule(dmfObject->TransportModule);

    return transportModule;
}

_Must_inspect_result_
BOOLEAN
DMF_ModuleRequestCompleteOrForward(
    _In_ DMFMODULE DmfModule,
    _In_ WDFREQUEST Request,
    _In_ NTSTATUS NtStatus
    )
/*++

Routine Description:

    Given a Module, a WDFREQUEST and the NTSTATUS to set in the WDFREQUEST by caller,
    complete or forward the WDFREQUEST as needed:
    If the caller wants to return STATUS_SUCCESS:
        If the Client Driver is a filter driver, tell DMF to pass the request down the stack.
        If the Client Driver is not a filter drier, then complete the request (by falling through).
    If the caller wants does not want to return STATUS_SUCCESS, then the request is just completed.

Arguments:

    DmfModule - This Module's handle.
    Request - The given File Create request.
    NtStatus - The NTSTATUS the caller wants to return.

Return Value:

    TRUE if the given request is completed.

--*/
{
    BOOLEAN completed;

    if (NT_SUCCESS(NtStatus))
    {
        // If this Module wants to return STATUS_SUCCESS, then:
        // If the Client Driver is a filter driver, tell DMF to pass the request down the stack.
        // If the Client Driver is not a filter drier, then complete the request (by falling through).
        //
        if (DMF_ModuleIsInFilterDriver(DmfModule))
        {
            // Tell DMF to pass the request down the stack.
            //
            completed = FALSE;
            goto Exit;
        }
    }
    // Either one of two cases are true:
    // 1. This Module wants to fail the request so it gets completed immediately.
    // 2. This Module wants to succeed the request and the Client Driver is not a filter driver.
    //
    WdfRequestComplete(Request,
                       NtStatus);
    completed = TRUE;

Exit:

    return completed;
}

#if defined(DMF_KERNEL_MODE)
RECORDER_LOG
DMF_InFlightRecorderGet(
    _In_ DMFMODULE DmfModule
    )
/*++

Routine Description:

    Given a Module, get its WPP In-flight Recorder handle.

Arguments:

    DmfModule - The given Module.

Return Value:

    The given Module's WPP In-flight Recorder handle.

--*/
{
    DMF_OBJECT* dmfObject = DMF_ModuleToObject(DmfModule);

    return dmfObject->InFlightRecorder;
}
#endif

// eof: DmfCore.c
//
