//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <string.h>
#include <stdio.h>
#include "ApxNode_TestNode1.h"
#include "pack.h"

//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
#define APX_DEFINITON_LEN 126u
#define APX_IN_PORT_DATA_LEN 2u
#define APX_OUT_PORT_DATA_LEN 3u
//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static void outPortData_writeCmd(apx_offset_t offset, apx_size_t len );
//////////////////////////////////////////////////////////////////////////////
// LOCAL VARIABLES
//////////////////////////////////////////////////////////////////////////////
static const uint8_t m_outPortInitData[APX_OUT_PORT_DATA_LEN]= {
   0, 0, 0
};

static uint8 m_outPortdata[APX_OUT_PORT_DATA_LEN];
static uint8_t m_outPortDirtyFlags[APX_OUT_PORT_DATA_LEN];
static const uint8_t m_inPortInitData[APX_IN_PORT_DATA_LEN]= {
   0, 0
};

static uint8 m_inPortdata[APX_IN_PORT_DATA_LEN];
static uint8_t m_inPortDirtyFlags[APX_IN_PORT_DATA_LEN];
static apx_nodeData_t m_nodeData;
static const char *m_apxDefinitionData=
"APX/1.2\n"
"N\"TestNode1\"\n"
"P\"WheelBasedVehicleSpeed\"S\n"
"P\"CabTiltLockWarning\"C(0,7)\n"
"R\"VehicleMode\"C(0,15)\n"
"R\"GearSelectionMode\"C(0,7)\n"
"\n";

//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
void ApxNode_Init_TestNode1(void)
{
   memcpy(&m_inPortdata[0], &m_inPortInitData[0], APX_IN_PORT_DATA_LEN);
   memset(&m_inPortDirtyFlags[0], 0, sizeof(m_inPortDirtyFlags));
   memcpy(&m_outPortdata[0], &m_outPortInitData[0], APX_OUT_PORT_DATA_LEN);
   memset(&m_outPortDirtyFlags[0], 0, sizeof(m_outPortDirtyFlags));
   apx_nodeData_create(&m_nodeData, "TestNode1", (uint8_t*) &m_apxDefinitionData[0], APX_DEFINITON_LEN, &m_inPortdata[0], &m_inPortDirtyFlags[0], APX_IN_PORT_DATA_LEN, &m_outPortdata[0], &m_outPortDirtyFlags[0], APX_OUT_PORT_DATA_LEN);
#ifdef APX_POLLED_DATA_MODE
   rbfs_create(&m_outPortDataCmdQueue, &m_outPortDataCmdBuf[0], APX_NUM_OUT_PORTS, APX_DATA_WRITE_CMD_SIZE);
#endif
}

apx_nodeData_t * ApxNode_GetNodeData_TestNode1(void)
{
   return &m_nodeData;
}

Std_ReturnType ApxNode_Read_TestNode1_VehicleMode(uint8 *val)
{
   apx_nodeData_lockInPortData(&m_nodeData);
   *val = (uint8) m_inPortdata[0];
   apx_nodeData_unlockInPortData(&m_nodeData);
   return E_OK;
}

Std_ReturnType ApxNode_Read_TestNode1_GearSelectionMode(uint8 *val)
{
   apx_nodeData_lockInPortData(&m_nodeData);
   *val = (uint8) m_inPortdata[1];
   apx_nodeData_unlockInPortData(&m_nodeData);
   return E_OK;
}

Std_ReturnType ApxNode_Write_TestNode1_WheelBasedVehicleSpeed(uint16 val)
{
   apx_nodeData_lockOutPortData(&m_nodeData);
   packLE(&m_outPortdata[0],(uint32) val,(uint8) 2u);
   outPortData_writeCmd(0, 2);
   return E_OK;
}

Std_ReturnType ApxNode_Write_TestNode1_CabTiltLockWarning(uint8 val)
{
   apx_nodeData_lockOutPortData(&m_nodeData);
   m_outPortdata[2]=(uint8) val;
   outPortData_writeCmd(2, 1);
   return E_OK;
}

void TestNode1_inPortDataWritten(void *arg, apx_nodeData_t *nodeData, uint32_t offset, uint32_t len)
{

}
//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static void outPortData_writeCmd(apx_offset_t offset, apx_size_t len )
{
   if ( (m_outPortDirtyFlags[offset] == 0) && (true == apx_nodeData_isOutPortDataOpen(&m_nodeData) ) )
   {
      m_outPortDirtyFlags[offset] = (uint8_t) 1u;
      apx_nodeData_unlockOutPortData(&m_nodeData);
      apx_nodeData_outPortDataNotify(&m_nodeData, (uint32_t) offset, (uint32_t) len);
      return;
   }
   apx_nodeData_unlockOutPortData(&m_nodeData);
}
