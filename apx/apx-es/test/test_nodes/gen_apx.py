import apx

if __name__ == '__main__':
    node = apx.Node('ButtonStatus')
    node.append(apx.DataType('PushButtonStatus_T', 'C(0,1)'))
    node.append(apx.DataType('VehicleMode_T', 'C(0,15)'))
    node.append(apx.ProvidePort('SWS_PushbuttonStatus_Back', 'T["PushButtonStatus_T"]', '=0'))
    node.append(apx.ProvidePort('SWS_PushbuttonStatus_Down', 'T["PushButtonStatus_T"]', '=0'))
    node.append(apx.ProvidePort('SWS_PushbuttonStatus_Enter', 'T["PushButtonStatus_T"]', '=0'))
    node.append(apx.ProvidePort('SWS_PushbuttonStatus_Home', 'T["PushButtonStatus_T"]', '=0'))
    node.append(apx.ProvidePort('SWS_PushbuttonStatus_Left', 'T["PushButtonStatus_T"]', '=0'))
    node.append(apx.ProvidePort('SWS_PushbuttonStatus_Right', 'T["PushButtonStatus_T"]', '=0'))
    node.append(apx.ProvidePort('SWS_PushbuttonStatus_Up', 'T["PushButtonStatus_T"]', '=0'))
    node.append(apx.RequirePort('VehicleMode', 'T["VehicleMode_T"]', '=15'))
    node.finalize()
    apx.NodeGenerator().generate('.', node, includes=['ApxTypeDefs.h'])
    apx.NodeGenerator().generate('.', node, name='ButtonStatusDirect', includes=['ApxTypeDefs.h'], direct_write=[
        'SWS_PushbuttonStatus_Back',
        'SWS_PushbuttonStatus_Down',
        'SWS_PushbuttonStatus_Back',
        'SWS_PushbuttonStatus_Enter',
        'SWS_PushbuttonStatus_Home',
        'SWS_PushbuttonStatus_Left',
        'SWS_PushbuttonStatus_Back',
        'SWS_PushbuttonStatus_Right',
        'SWS_PushbuttonStatus_Up',
        ])