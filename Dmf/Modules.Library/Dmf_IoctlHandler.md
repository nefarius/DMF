## DMF_IoctlHandler

-----------------------------------------------------------------------------------------------------------------------------------

#### Module Summary

Implements a driver pattern that allows the Client to create a table of supported IOCTLs along with information about each IOCTL
so that the Module can perform automatic validation.

-----------------------------------------------------------------------------------------------------------------------------------

#### Module Configuration

##### DMF_CONFIG_IoctlHandler
````
typedef struct
{
  // GUID of the device interface that allows User-mode access.
  //
  GUID DeviceInterfaceGuid;
  // Device Interface Reference String (optional).
  //
  WCHAR* ReferenceString;
  // Allows Client to filter access to IOCTLs.
  //
  IoctlHandler_AccessModeFilterType AccessModeFilter;
  // This is only set if the AccessModeFilter == IoctlHandler_AccessModeFilterClientCallback.
  //
  EVT_DMF_IoctlHandler_AccessModeFilter* EvtIoctlHandlerAccessModeFilter;
  // This is a pointer to a static table in the Client.
  //
  IoctlHandler_IoctlRecord* IoctlRecords;
  // The number of records in the above table.
  //
  ULONG IoctlRecordCount;
  // FALSE (Default) means that the corresponding device interface is created when this Module opens.
  // TRUE requires that the Client call DMF_IoctlHandler_IoctlsEnable() to enable the corresponding device interface.
  //
  BOOLEAN ManualMode;
  // FALSE (Default) means that the corresponding device interface will handle all IOCTL types.
  // TRUE means that the Module allows only requests from kernel mode clients.
  //
  BOOLEAN KernelModeRequestsOnly;
  // Do not use. (For backward compatibility purposes only.)
  //
  WCHAR* CustomCapabilities;
  DEVPROP_BOOLEAN IsRestricted;
  // Allows Client to perform actions after the Device Interface is created.
  //
  EVT_DMF_IoctlHandler_PostDeviceInterfaceCreate* PostDeviceInterfaceCreate;
  // Allows request forwarding for IOCTLs not handled by this Module.
  //
  BOOLEAN ForwardUnhandledRequests;
} DMF_CONFIG_IoctlHandler;
````
Member | Description
----|----
DeviceInterfaceGuid | The GUID of the Device Interface to expose.
AccessModeFilter | Indicates what kind of access control mechanism is used, if any. Use IoctlHandler_AccessModeDefault in most cases.
EvtIoctlHandlerAccessModeFilter | A callback that allows the Client to filter the IOCTL with Client specific logic.
IoctlRecords | A table of records that specify information about each supported IOCTL.
ManualMode | Module open configuration.
KernelModeRequestsOnly | This allows the Module to handle only requests from kernel mode clients.
CustomCapabilities | Do not use. This field is only present for backward compatibility purposes.
IsRestricted | Do not use. This field is only present for backward compatibility purposes.
PostDeviceInterfaceCreate | Allows Client to perform actions after the Device Interface is created.
ForwardUnhandledRequests | Allows request forwarding for IOCTLs not handled by this Module.
ReferenceString | Optional device interface instance reference string. It must remain in memory while for the lifetime of the driver.

-----------------------------------------------------------------------------------------------------------------------------------

#### Module Enumeration Types

##### IoctlHandler_AccessModeFilterType
````
typedef enum
{
  // Do what WDF would normally do (allow User-mode).
  //
  IoctlHandler_AccessModeDefault,
  // Call a Client Callback function that will decide.
  //
  IoctlHandler_AccessModeFilterClientCallback,
  // NOTE: This is currently not implemented.
  //
  IoctlHandler_AccessModeFilterDoNotAllowUserMode,
  // Only allows "Run as Administrator".
  //
  IoctlHandler_AccessModeFilterAdministratorOnly,
  // Allow access to Administrator on a per-IOCTL basis.
  //
  IoctlHandler_AccessModeFilterAdministratorOnlyPerIoctl,
  // Restrict to Kernel-mode access only.
  //
  IoctlHandler_AccessModeFilterKernelModeOnly
} IoctlHandler_AccessModeFilterType;
````
Member | Description
----|----
IoctlHandler_AccessModeDefault | Indicates that the IOCTL has no restrictions.
IoctlHandler_AccessModeFilterClientCallback | Indicates that a Client callback should be called which will determine access to the IOCTL.
IoctlHandler_AccessModeFilterDoNotAllowUserMode | Not supported.
IoctlHandler_AccessModeFilterAdministratorOnly | Indicates that only a process running "As Administrator" has access to all the IOCTLs in the table.
IoctlHandler_AccessModeFilterAdministratorOnlyPerIoctl | Indicates that the IOCTL's table entry indicates if only processes running "As Administrator" have access to the IOCTLs in the table.
IoctlHandler_AccessModeFilterKernelModeOnly | Restrict to Kernel-mode access only.

-----------------------------------------------------------------------------------------------------------------------------------

#### Module Structures

##### IoctlHandler_IoctlRecord
````
typedef struct
{
  // The IOCTL code.
  // NOTE: At this time only METHOD_BUFFERED or METHOD_DIRECT buffers are supported.
  //
  ULONG IoctlCode;
  // Minimum input buffer size which is automatically validated by this Module.
  //
  ULONG InputBufferMinimumSize;
  // Minimum out buffer size which is automatically validated by this Module.
  //
  ULONG OutputBufferMinimumSize;
  // IOCTL handler callback after buffer size validation.
  //
  EVT_DMF_IoctlHandler_Function* EvtIoctlHandlerFunction;
  // Administrator access only. This flag is used with IoctlHandler_AccessModeFilterAdministratorOnlyPerIoctl
  // to allow Administrator access on a per-IOCTL basis.
  //
  BOOLEAN AdministratorAccessOnly;
} IoctlHandler_IoctlRecord;
````

-----------------------------------------------------------------------------------------------------------------------------------

#### Module Callbacks

##### EVT_DMF_IoctlHandler_Callback
````
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
NTSTATUS
EVT_DMF_IoctlHandler_Callback(
    _In_ DMFMODULE DmfModule,
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request,
    _In_ ULONG IoctlCode,
    _In_reads_(InputBufferSize) VOID* InputBuffer,
    _In_ size_t InputBufferSize,
    _Out_writes_(OutputBufferSize) VOID* OutputBuffer,
    _In_ size_t OutputBufferSize,
    _Out_ size_t* BytesReturned
    );
````

This callback is called for every IOCTL that is allowed access and has properly sized input and output buffers as
specified in the IOCTL table passed in during Module instantiation.

##### Returns

If STATUS_PENDING is returned, then the Request is not completed. If STATUS_SUCCESS or any other NTSTATUS is returned,
then the Request is completed with that NTSTATUS.

##### Parameters
Parameter | Description
----|----
DmfModule | An open DMF_IoctlHandler Module handle.
Queue | The WDFQUEUE handle associated with Request.
Request | The WDFREQUEST that contains the IOCTL parameters and buffers.
IoctlCode | The IOCTL code passed by the caller.
InputBuffer | The Request's input buffer, if any.
InputBufferSize | The size of the Request's input buffer.
OutputBuffer | The Request's output buffer, if any.
OutputBufferSize | The size of the Request's output buffer.
BytesReturned | The number of bytes returned to the caller. Indicates the number of bytes transferred usually. **IMPORTANT**: Set *BytesReturned to zero when the Request is NOT completed by this callback. If the callback returns STATUS_PENDING, the number of bytes returned can be set when the Client completes the Request using `WdfRequestCompleteWithInformation()`. If STATUS_PENDING is returned, do not store BytesReturned and write to it when Request is completed.__

##### Remarks

* Request is passed to the callback, but it is rarely needed because the Module has already extracted the commonly used parameters and passed them to the callback.

##### EVT_DMF_IoctlHandler_AccessModeFilter
````
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
BOOLEAN
EVT_DMF_IoctlHandler_AccessModeFilter(
    _In_ DMFMODULE DmfModule,
    _In_ WDFDEVICE Device,
    _In_ WDFREQUEST Request,
    _In_ WDFFILEOBJECT FileObject
    );
````

Allows Client to filter access to the IOCTLs. Client can use the parameters to decide if the connection to User-mode should be
allowed. It is provided in case default handler does not provide needed support. Use default handler has a guide for how to
implement the logic in this callback.

##### Returns

Return value of TRUE indicates that this callback completed the Request.

##### Parameters
Parameter | Description
----|----
DmfModule | An open DMF_IoctlHandler Module handle.
Device | Wdf Device object associated with Queue.
Request | Incoming request.
FileObject | File object associated with the given Request.

##### EVT_DMF_IoctlHandler_PostDeviceInterfaceCreate
````
typedef
_Function_class_(EVT_DMF_IoctlHandler_PostDeviceInterfaceCreate)
_IRQL_requires_max_(PASSIVE_LEVEL)
_IRQL_requires_same_
NTSTATUS
EVT_DMF_IoctlHandler_PostDeviceInterfaceCreate(_In_ DMFMODULE DmfModule,
                                               _In_ GUID* DeviceInterfaceGuid,
                                               _In_opt_ UNICODE_STRING* ReferenceStringUnicodePointer);
````
Allows the Client to perform Client specific tasks after the device interface is created.

##### Returns

NTSTATUS. If an error is returned, the Module will not open.

##### Parameters
Parameter | Description
----|----
DmfModule | An open DMF_IoctlHandler Module handle.
DeviceInterfaceGuid | The GUID passed in the Module's Config when the Module is instantiated.
ReferenceStringUnicodePointer | Indicates the Reference String corresponding to the Device Interface for which this callback is called.

-----------------------------------------------------------------------------------------------------------------------------------

#### Module Methods

##### DMF_IoctlHandler_IoctlChain

````
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
DMF_IoctlHandler_IoctlChain(
    _In_ DMFMODULE DmfModule,
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request,
    _In_ ULONG IoctlCode,
    _In_reads_(InputBufferSize) VOID* InputBuffer,
    _In_ size_t InputBufferSize,
    _Out_writes_(OutputBufferSize) VOID* OutputBuffer,
    _In_ size_t OutputBufferSize,
    _Out_ size_t* BytesReturned
    )
````
Allows Client to send an IOCTL to a Dynamic Module. Dynamic Modules do not receive WDF calls directly. In rare cases, a Dynamic Module may have a child IoctlHandler. In that case,
this Method is used to route calls from a Parent Module to the Child Module.

##### Returns

None

##### Parameters
Parameter | Description
----|----
DmfModule | An open DMF_IoctlHandler Module handle.
Queue | Parameter passed to this Method's caller that must be passed to target Module.
Request |  Parameter passed to this Method's caller that must be passed to target Module.
IoctlCode | Parameter passed to this Method's caller that must be passed to target Module.
InputBuffer | Parameter passed to this Method's caller that must be passed to target Module.
InputBufferSize | Parameter passed to this Method's caller that must be passed to target Module.
OutputBuffer | Parameter passed to this Method's caller that must be passed to target Module.
OutputBufferSize | Parameter passed to this Method's caller that must be passed to target Module.
BytesReturned | Parameter passed to this Method's caller that must be passed to target Module.

##### Remarks

* **IMPORTANT:** Table entries for Child Module must be in Parent Module for buffer size validation. Use the exact same entries but with a different callback to the Parent Module which then calls this Method.

-----------------------------------------------------------------------------------------------------------------------------------

##### DMF_IoctlHandler_IoctlStateSet

````
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
DMF_IoctlHandler_IoctlStateSet(
    _In_ DMFMODULE DmfModule,
    _In_ BOOLEAN Enable
    )
````
Allows Client to enable / disable the device interface set in the Module's Config.

##### Returns

NTSTATUS

##### Parameters
Parameter | Description
----|----
DmfModule | An open DMF_IoctlHandler Module handle.
Enable | If true, enable the device interface. Otherwise, disable the device interface.

##### Remarks

-----------------------------------------------------------------------------------------------------------------------------------

#### Module IOCTLs

-----------------------------------------------------------------------------------------------------------------------------------

#### Module Remarks

* This Module removes the need for the programmer to validate the input and output buffers for all IOCTLs since the Module does this work.
* In case, validation by the Module is not desired, simply pass zero as the minimum size for the input and output buffers. Then, the IOCTL callback can perform its own Client specific validation.
* This Module optionally allows Clients to forward the unhandled requests down. For forwarding all requests, simply add this Module with empty IoctlRecords and ForwardUnhandledRequests set to TRUE.
* IMPORTANT: When this Module is used the Client Driver must not set `QueueConfig` to NULL if the Client calls `DMF_DmfDeviceInitHookQueueConfig()` (to customize the default queue) because the default queue will not be created. In this case, DMF_IoctlHandler will not see any IOCTL that is sent to it.
* Multiple instances of this Module can be instantiated using the same IOCTL table as long as each instance sets a unique ReferenceString. The Module will route the requests from the default queue to the instance of the Module corresponding to the ReferenceString of the WDFIOTARGET.

-----------------------------------------------------------------------------------------------------------------------------------

#### Module Implementation Details

-----------------------------------------------------------------------------------------------------------------------------------

#### Examples

* DMF_ResourceHub

-----------------------------------------------------------------------------------------------------------------------------------

#### To Do

-----------------------------------------------------------------------------------------------------------------------------------

#### Module Category

Driver Patterns

-----------------------------------------------------------------------------------------------------------------------------------

