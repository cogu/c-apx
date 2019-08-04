#!/usr/bin/env python3
import apx
        
if __name__ == '__main__':

    node1 = apx.Node('TestNode1')
    node1.append(apx.ProvidePort('WheelBasedVehicleSpeed','S'))
    node1.append(apx.ProvidePort('CabTiltLockWarning','C(0,7)'))
    node1.append(apx.RequirePort('VehicleMode','C(0,15)'))
    node1.append(apx.RequirePort('GearSelectionMode','C(0,7)'))
    apx.NodeGenerator().generate('.', node1, includes=['Std_Types.h'])
    
    node2 = apx.Node('TestNode2')
    node2.append(apx.ProvidePort('ParkBrakeAlert','C(0,3)','=3'))
    apx.NodeGenerator().generate('.', node2, includes=['Std_Types.h'])
    
