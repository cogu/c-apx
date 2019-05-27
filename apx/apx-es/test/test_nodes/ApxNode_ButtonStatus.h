#ifndef APXNODE_BUTTONSTATUS_H
#define APXNODE_BUTTONSTATUS_H

#include "apx_nodeData.h"
#include "ApxTypeDefs.h"

//////////////////////////////////////////////////////////////////////////////
// CONSTANTS
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
void ApxNode_Init_ButtonStatus(void);
apx_nodeData_t * ApxNode_GetNodeData_ButtonStatus(void);

Std_ReturnType ApxNode_Read_ButtonStatus_VehicleMode(VehicleMode_T *val);
Std_ReturnType ApxNode_Write_ButtonStatus_SWS_PushbuttonStatus_Back(PushButtonStatus_T val);
Std_ReturnType ApxNode_Write_ButtonStatus_SWS_PushbuttonStatus_Down(PushButtonStatus_T val);
Std_ReturnType ApxNode_Write_ButtonStatus_SWS_PushbuttonStatus_Enter(PushButtonStatus_T val);
Std_ReturnType ApxNode_Write_ButtonStatus_SWS_PushbuttonStatus_Home(PushButtonStatus_T val);
Std_ReturnType ApxNode_Write_ButtonStatus_SWS_PushbuttonStatus_Left(PushButtonStatus_T val);
Std_ReturnType ApxNode_Write_ButtonStatus_SWS_PushbuttonStatus_Right(PushButtonStatus_T val);
Std_ReturnType ApxNode_Write_ButtonStatus_SWS_PushbuttonStatus_Up(PushButtonStatus_T val);
void ButtonStatus_inPortDataWritten(void *arg, apx_nodeData_t *nodeData, uint32_t offset, uint32_t len);

#endif //APXNODE_BUTTONSTATUS_H
