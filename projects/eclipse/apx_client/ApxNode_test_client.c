//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <string.h>
#include <stdio.h>
#include "ApxNode_test_client.h"
#include "pack.h"

//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
#define APX_DEFINITON_LEN 223u
#define APX_IN_PORT_DATA_LEN 4u
#define APX_OUT_PORT_DATA_LEN 0u
//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
// LOCAL VARIABLES
//////////////////////////////////////////////////////////////////////////////
static const uint8_t m_inPortInitData[APX_IN_PORT_DATA_LEN]= {
   3, 255, 255, 255
};

static uint8 m_inPortdata[APX_IN_PORT_DATA_LEN];
static uint8_t m_inPortDirtyFlags[APX_OUT_PORT_DATA_LEN];
static apx_nodeData_t m_nodeData;
static const char *m_apxDefinitionData=
"APX/1.2\n"
"N\"test_client\"\n"
"T\"OffOn_T\"C(0,3):VT(\"OffOn_Off\",\"OffOn_On\",\"OffOn_Error\",\"OffOn_NotAvailable\")\n"
"T\"Percent_T\"C\n"
"T\"VehicleSpeed_T\"S\n"
"R\"EngineRunningStatus\"T[0]:=3\n"
"R\"FuelLevelPercent\"T[1]:=255\n"
"R\"VehicleSpeed\"T[2]:=0xFFFF\n"
"\n";

//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
void ApxNode_Init_test_client(void)
{
   memcpy(&m_inPortdata[0], &m_inPortInitData[0], APX_IN_PORT_DATA_LEN);
   memset(&m_inPortDirtyFlags[0], 0, sizeof(m_inPortDirtyFlags));
   apx_nodeData_create(&m_nodeData, "test_client", (uint8_t*) &m_apxDefinitionData[0], APX_DEFINITON_LEN, &m_inPortdata[0], &m_inPortDirtyFlags[0], APX_IN_PORT_DATA_LEN, 0, 0, 0);
#ifdef APX_POLLED_DATA_MODE
   rbfs_create(&m_outPortDataCmdQueue, &m_outPortDataCmdBuf[0], APX_NUM_OUT_PORTS, APX_DATA_WRITE_CMD_SIZE);
#endif
}

apx_nodeData_t * ApxNode_GetNodeData_test_client(void)
{
   return &m_nodeData;
}

Std_ReturnType ApxNode_Read_test_client_EngineRunningStatus(OffOn_T *val)
{
   apx_nodeData_lockInPortData(&m_nodeData);
   *val = (uint8) m_inPortdata[0];
   apx_nodeData_unlockInPortData(&m_nodeData);
   return E_OK;
}

Std_ReturnType ApxNode_Read_test_client_FuelLevelPercent(Percent_T *val)
{
   apx_nodeData_lockInPortData(&m_nodeData);
   *val = (uint8) m_inPortdata[1];
   apx_nodeData_unlockInPortData(&m_nodeData);
   return E_OK;
}

Std_ReturnType ApxNode_Read_test_client_VehicleSpeed(VehicleSpeed_T *val)
{
   apx_nodeData_lockInPortData(&m_nodeData);
   *val = (uint16) unpackLE(&m_inPortdata[2],(uint8) 2u);
   apx_nodeData_unlockInPortData(&m_nodeData);
   return E_OK;
}

void test_client_inPortDataWritten(void *arg, apx_nodeData_t *nodeData, uint32_t offset, uint32_t len)
{

}
//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
