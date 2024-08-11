/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "renderer.h"
#include "materialGraph.h"
#include "renderFrame.h"
#include "../../common/core/gatheredResource.h"

// Render
IRender* GRender = NULL;

CGatheredResource resMaterialFallbackShader( TXT("engine\\materials\\render\\fallback.w2mg"), RGF_Startup ); 

CMaterialGraph* IRender::GetFallbackShader()
{
	return resMaterialFallbackShader.LoadAndGet< CMaterialGraph >();
}

void IRender::ForceFakeDeviceReset()
{
}

void IRender::ForceFakeDeviceLost()
{
}

void IRender::ForceFakeDeviceUnlost()
{
}
