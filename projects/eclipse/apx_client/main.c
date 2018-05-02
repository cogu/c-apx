#include <stdio.h>
#include "ApxNode_test_client.h"
#include "apx_client.h"
#include "unistd.h"
#include "osmacro.h"

static apx_client_t m_client;
static apx_nodeData_t *m_nodeData;
int8_t g_debug;


static void print_values(void)
{
   OffOn_T EngineRunningStatus;
   Percent_T FuelLevelPercent;
   VehicleSpeed_T  VehicleSpeed;
   ApxNode_Read_test_client_EngineRunningStatus(&EngineRunningStatus);
   ApxNode_Read_test_client_FuelLevelPercent(&FuelLevelPercent);
   ApxNode_Read_test_client_VehicleSpeed(&VehicleSpeed);
   printf("EngineRunningStatus=%d FuelLevelPercent=%.1f (%d)", (int) EngineRunningStatus, ((double) FuelLevelPercent)*0.4, (int) FuelLevelPercent);
   printf(" VehicleSpeed=%.1f (%d)\n",((double)VehicleSpeed)/64.0, (int) VehicleSpeed );
}

static void run(void)
{
   print_values();
}

int main(int argc, char **argv)
{
   ApxNode_Init_test_client();
   m_nodeData = ApxNode_GetNodeData_test_client();
   apx_client_create(&m_client);
   apx_client_attachLocalNode(&m_client, m_nodeData);
   apx_client_connect_tcp(&m_client, "127.0.0.1", 5000);
   for(;;)
   {
      SLEEP(1000);
      run();
   }
   return 0;
}
