#ifndef APXNODE_TESTNODE1_H
#define APXNODE_TESTNODE1_H

#include "apx_nodeData.h"
#include "Std_Types.h"

//////////////////////////////////////////////////////////////////////////////
// CONSTANTS
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
void ApxNode_Init_TestNode1(void);
apx_nodeData_t * ApxNode_GetNodeData_TestNode1(void);

Std_ReturnType ApxNode_Read_TestNode1_VehicleMode(uint8 *val);
Std_ReturnType ApxNode_Read_TestNode1_GearSelectionMode(uint8 *val);
Std_ReturnType ApxNode_Write_TestNode1_WheelBasedVehicleSpeed(uint16 val);
Std_ReturnType ApxNode_Write_TestNode1_CabTiltLockWarning(uint8 val);
void TestNode1_inPortDataWritten(void *arg, apx_nodeData_t *nodeData, uint32_t offset, uint32_t len);

#endif //APXNODE_TESTNODE1_H
