#ifndef APXNODE_BUTTONSTATUSDIRECT_H
#define APXNODE_BUTTONSTATUSDIRECT_H

#include <stdbool.h>
#include "apx_nodeData.h"
#include "ApxTypeDefs.h"

//////////////////////////////////////////////////////////////////////////////
// CONSTANTS
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
apx_nodeData_t * ApxNode_Init_ButtonStatusDirect(void);
apx_nodeData_t * ApxNode_GetNodeData_ButtonStatusDirect(void);
bool ApxNode_IsConnected_ButtonStatusDirect(void);

Std_ReturnType ApxNode_Read_ButtonStatusDirect_VehicleMode(VehicleMode_T *val);
Std_ReturnType ApxNode_Write_ButtonStatusDirect_SWS_PushbuttonStatus_Back(PushButtonStatus_T val);
Std_ReturnType ApxNode_Write_ButtonStatusDirect_SWS_PushbuttonStatus_Down(PushButtonStatus_T val);
Std_ReturnType ApxNode_Write_ButtonStatusDirect_SWS_PushbuttonStatus_Enter(PushButtonStatus_T val);
Std_ReturnType ApxNode_Write_ButtonStatusDirect_SWS_PushbuttonStatus_Home(PushButtonStatus_T val);
Std_ReturnType ApxNode_Write_ButtonStatusDirect_SWS_PushbuttonStatus_Left(PushButtonStatus_T val);
Std_ReturnType ApxNode_Write_ButtonStatusDirect_SWS_PushbuttonStatus_Right(PushButtonStatus_T val);
Std_ReturnType ApxNode_Write_ButtonStatusDirect_SWS_PushbuttonStatus_Up(PushButtonStatus_T val);
void ButtonStatusDirect_inPortDataWritten(void *arg, apx_nodeData_t *nodeData, uint32_t offset, uint32_t len);

#endif //APXNODE_BUTTONSTATUSDIRECT_H
