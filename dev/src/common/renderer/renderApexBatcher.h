/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#include "apexRenderInterface.h"

#ifdef USE_APEX

class CRenderElement_Apex;
class CRenderOcclusionData;

enum ERenderApexBatcherFlags
{
	RABF_Solid = 1,
	RABF_SolidMasked = 2,
	RABF_Particles = 4,
	RABF_All = RABF_Solid | RABF_SolidMasked | RABF_Particles
};

class CRenderApexBatcher : public IDynamicRenderResource
{
protected:
	CApexRenderer m_renderer;

public:
	CRenderApexBatcher();
	~CRenderApexBatcher();

	void RenderApex( const CRenderFrameInfo& info, const RenderingContext& context, CRenderElement_Apex* batchList, Uint32 drawFlags, class MeshDrawingStats& outMeshStats );
	void RenderApex( const CRenderFrameInfo& info, const RenderingContext& context, const TDynArray< CRenderElement_Apex* >& batchList, Uint32 drawFlags, class MeshDrawingStats& outMeshStats );

	virtual void OnDeviceLost();
	virtual void OnDeviceReset();
	virtual CName GetCategory() const;

protected:
	void RenderBatch( const RenderingContext& context, CRenderElement_Apex* batch );

	void SortBackToFront( const RenderingContext& context, CRenderElement_Apex*& batch );
};

#endif
