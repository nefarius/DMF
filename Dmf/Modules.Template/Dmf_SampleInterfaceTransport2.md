## DMF_SampleInterfaceTransport2

-----------------------------------------------------------------------------------------------------------------------------------

#### Module Summary

-----------------------------------------------------------------------------------------------------------------------------------

A summary of the Module is documented here. More details are in the "[Module Remarks]" section below.

Sample Transport Module to demonstrate Protocol - Transport feature and usage of Interfaces in DMF.

-----------------------------------------------------------------------------------------------------------------------------------

#### Module Configuration

-----------------------------------------------------------------------------------------------------------------------------------
##### DMF_CONFIG_SampleInterfaceTransport2
````
typedef struct
{
    // This Module's ID.
    //
    ULONG ModuleId;
    // This Module's Name.
    //
    PSTR ModuleName;
} DMF_CONFIG_SampleInterfaceTransport2;
````

##### Remarks

This Module implements the Transport part of the Dmf_Interface_SampleInterface Interface.

-----------------------------------------------------------------------------------------------------------------------------------

#### Module Enumeration Types

-----------------------------------------------------------------------------------------------------------------------------------

##### Remarks

This section lists all the Enumeration Types specific to this Module that are accessible to Clients.

-----------------------------------------------------------------------------------------------------------------------------------

#### Module Callbacks

* None

-----------------------------------------------------------------------------------------------------------------------------------

#### Module Methods

-----------------------------------------------------------------------------------------------------------------------------------

Refer to Dmf_Interface_SampleInterface for a list of methods implemented.

-----------------------------------------------------------------------------------------------------------------------------------

#### Module IOCTLs

* None

-----------------------------------------------------------------------------------------------------------------------------------

#### Module Remarks

* Detailed explanation about using the Module that Clients need to consider.

-----------------------------------------------------------------------------------------------------------------------------------

#### Module Children

* List of any Child Modules instantiated by this Module.

-----------------------------------------------------------------------------------------------------------------------------------

#### Module Implementation Details

-----------------------------------------------------------------------------------------------------------------------------------

#### Examples

* InterfaceSample1

-----------------------------------------------------------------------------------------------------------------------------------

#### To Do

-----------------------------------------------------------------------------------------------------------------------------------

* List possible future work for this Module.

-----------------------------------------------------------------------------------------------------------------------------------
#### Module Category

-----------------------------------------------------------------------------------------------------------------------------------

Template

-----------------------------------------------------------------------------------------------------------------------------------
