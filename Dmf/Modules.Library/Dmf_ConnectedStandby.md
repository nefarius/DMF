## DMF_ConnectedStandby

-----------------------------------------------------------------------------------------------------------------------------------

#### Module Summary

This Module allows the Client to request and receive notifications when the system enters or exit Connected (a.k.a. Modern) Standby.
NOTE: In this Module "Connected Standby" also means "Modern Standby".

-----------------------------------------------------------------------------------------------------------------------------------

#### Module Configuration

##### DMF_CONFIG_ConnectedStandby
````
typedef struct
{
    // Client's callback when Connected Standby state changes.
    //
    EVT_DMF_ConnectedStandbyStateChangedCallback* ConnectedStandbyStateChangedCallback;
} DMF_CONFIG_ConnectedStandby;
````

Member | Description
----|----
ConnectedStandbyStateChangedCallback | Client callback that receives notifications when the Connected Standby state changes.

-----------------------------------------------------------------------------------------------------------------------------------

#### Module Enumeration Types

-----------------------------------------------------------------------------------------------------------------------------------

#### Module Structures

-----------------------------------------------------------------------------------------------------------------------------------

#### Module Callbacks

##### EVT_DMF_ConnectedStandbyStateChangedCallback
````
_IRQL_requires_max_(PASSIVE_LEVEL)
_IRQL_requires_same_
VOID
EVT_DMF_ConnectedStandbyStateChangedCallback(
    _In_ DMFMODULE DmfModule,
    _In_ BOOL SystemInConnectedStandby
    );
````

Client callback that receives notifications when Connected Standby state changes.

##### Returns

None

##### Parameters
Parameter | Description
----|----
DmfModule | An open DMF_ConnectedStandby Module handle.

-----------------------------------------------------------------------------------------------------------------------------------

#### Module Methods
-----------------------------------------------------------------------------------------------------------------------------------

#### Module IOCTLs

-----------------------------------------------------------------------------------------------------------------------------------

#### Module Remarks

-----------------------------------------------------------------------------------------------------------------------------------

#### Module Implementation Details

-----------------------------------------------------------------------------------------------------------------------------------

#### Examples

-----------------------------------------------------------------------------------------------------------------------------------

#### To Do

-----------------------------------------------------------------------------------------------------------------------------------

#### Module Category

Driver Patterns

-----------------------------------------------------------------------------------------------------------------------------------

