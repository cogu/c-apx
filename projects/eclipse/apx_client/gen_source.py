import autosar
import apx

ws = autosar.workspace()
dataTypes = ws.getDataTypePackage()
dataTypes.createIntegerDataType('OffOn_T', valueTable=[
       "OffOn_Off",
       "OffOn_On",
       "OffOn_Error",
       "OffOn_NotAvailable"])
dataTypes.createIntegerDataType('Percent_T', min=0, max=255, offset=0, scaling=0.4, unit='Percent')
dataTypes.createIntegerDataType('VehicleSpeed_T', min=0, max=65535, offset=0, scaling=1/64, unit='km/h')
constants = ws.getConstantPackage()
constants.createConstant('C_EngineRunningStatus_IV', 'OffOn_T', 3)
constants.createConstant('C_FuelLevelPercent_IV', 'Percent_T', 255)
constants.createConstant('C_VehicleSpeed_IV', 'VehicleSpeed_T', 65535)
portInterfaces = ws.getPortInterfacePackage()
portInterfaces.createSenderReceiverInterface('EngineRunningStatus_I',
                                             autosar.DataElement('EngineRunningStatus', 'OffOn_T'))
portInterfaces.createSenderReceiverInterface('FuelLevelPercent_I',
                                             autosar.DataElement('FuelLevelPercent', 'Percent_T'))
portInterfaces.createSenderReceiverInterface('VehicleSpeed_I',
                                             autosar.DataElement('VehicleSpeed', 'VehicleSpeed_T'))
components = ws.getComponentTypePackage()
swc = components.createApplicationSoftwareComponent('test_client')
swc.createRequirePort('EngineRunningStatus', 'EngineRunningStatus_I', initValueRef=constants['C_EngineRunningStatus_IV'].ref)
swc.createRequirePort('FuelLevelPercent', 'FuelLevelPercent_I', initValueRef=constants['C_FuelLevelPercent_IV'].ref)
swc.createRequirePort('VehicleSpeed', 'VehicleSpeed_I', initValueRef=constants['C_VehicleSpeed_IV'].ref)
swc.behavior.createRunnable(swc.name+'_Init', portAccess=[x.name for x in swc.providePorts])
swc.behavior.createRunnable(swc.name+'_Run', portAccess=[x.name for x in swc.requirePorts+swc.providePorts])

partition = autosar.rte.Partition()
partition.addComponent(swc)
typeGenerator = autosar.rte.TypeGenerator(partition)
typeGenerator.generate()

node = apx.Node()
node.import_autosar_swc(swc)
apx.NodeGenerator().generate('.', node, includes=['Rte_Type.h'])
apx.Context().append(node).generateAPX('.')