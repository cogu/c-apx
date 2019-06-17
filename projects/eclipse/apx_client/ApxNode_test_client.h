#ifndef APXNODE_TEST_CLIENT_H
#define APXNODE_TEST_CLIENT_H

#include <stdbool.h>
#include "apx_nodeData.h"
#include "Rte_Type.h"

//////////////////////////////////////////////////////////////////////////////
// CONSTANTS
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
apx_nodeData_t * ApxNode_Init_test_client(void);
apx_nodeData_t * ApxNode_GetNodeData_test_client(void);
bool ApxNode_IsConnected_test_client(void);

Std_ReturnType ApxNode_Read_test_client_EngineRunningStatus(OffOn_T *val);
Std_ReturnType ApxNode_Read_test_client_FuelLevelPercent(Percent_T *val);
Std_ReturnType ApxNode_Read_test_client_VehicleSpeed(VehicleSpeed_T *val);
Std_ReturnType ApxNode_Write_test_client_VehicleMode(uint8 val);
void test_client_inPortDataWritten(void *arg, apx_nodeData_t *nodeData, uint32_t offset, uint32_t len);

#endif //APXNODE_TEST_CLIENT_H
