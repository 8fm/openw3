/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

IStaticRenderResource::IStaticRenderResource()
{

}

void IStaticRenderResource::OnDeviceLost()
{
	// Static resources are not lost
}

void IStaticRenderResource::OnDeviceReset()
{
	// Static resources are not restored
}