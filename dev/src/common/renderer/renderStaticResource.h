/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

/// D3D9 resource that is not recreated during device reset
class IStaticRenderResource : public IDynamicRenderResource
{
public:
	IStaticRenderResource();
	virtual void OnDeviceLost();
	virtual void OnDeviceReset();
};