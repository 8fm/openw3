/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "renderDynamicResource.h"

class CRenderDynamicDecalChunk;
class CRenderOcclusionData;
class CRenderFrameInfo;
class RenderingContext;
class MeshDrawingStats;

class CRenderDynamicDecalBatcher : public IDynamicRenderResource
{
public:
	CRenderDynamicDecalBatcher();
	~CRenderDynamicDecalBatcher();

	void RenderDecalChunks( const CRenderFrameInfo& frameInfo, const RenderingContext& context, const TDynArray< CRenderDynamicDecalChunk* >& chunks );

	virtual void OnDeviceLost();
	virtual void OnDeviceReset();
	virtual CName GetCategory() const;
};
