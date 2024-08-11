/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "renderResource.h"

IRenderResource::IRenderResource()
{
}

IRenderResource::~IRenderResource()
{
}

CName IRenderResource::GetCategory() const
{
	return CName::NONE;
}

Uint32 IRenderResource::GetUsedVideoMemory() const
{
	return 0;
}
