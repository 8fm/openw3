/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

/// Forward declarations
class CRenderProxy_Flare;

/// Terrain renderer
class CRenderFlaresBatcher : public IDynamicRenderResource
{
protected:
	struct SFlareSortData
	{
		Float sortKey;
		CRenderProxy_Flare *flare;
	};
	struct FlareSortPred
	{
	public:
		Bool operator()( const SFlareSortData& a, const SFlareSortData& b ) const
		{
			return a.sortKey > b.sortKey;
		}
	};

protected:
	TDynArray< SFlareSortData, MC_RenderData >	m_sortTable;

	GpuApi::BufferRef							m_vertexBuffer;
	GpuApi::BufferRef							m_indexBuffer;
	Bool										m_isInitialized;

public:
	CRenderFlaresBatcher();
	~CRenderFlaresBatcher();

	//! Grab transparency helpers
	void GrabTransparencyHelpers( CRenderCollector& collector, const TDynArray< CRenderProxy_Flare* >& flares, const CRenderFrameInfo& info, const GpuApi::TextureRef &texGrabSource );

	//! Update flares occlusion
	void UpdateOcclusion( CRenderCollector& collector, const TDynArray< CRenderProxy_Flare* >& flares, const CRenderFrameInfo& info, const GpuApi::TextureRef &texTransparencyHelperOpaque, const GpuApi::TextureRef &texTransparencyHelperTransparent );

	//! Update flares occlusion
	void DrawDebugOcclusion( CRenderCollector& collector, const TDynArray< CRenderProxy_Flare* >& flares, const CRenderFrameInfo& info );

	//! Draw flares
	void DrawFlares( CRenderCollector& collector, const RenderingContext& context, const TDynArray< CRenderProxy_Flare* >& flares );

	//! Draw lens flares
	void DrawLensFlares( CRenderCollector& collector, const RenderingContext& context, const TDynArray< CRenderProxy_Flare* >& flares );

protected:
	void Initialize();

	// Device Reset/Lost handling
	virtual void OnDeviceLost();
	virtual void OnDeviceReset();
	virtual CName GetCategory() const;
	virtual Uint32 GetUsedVideoMemory() const;
};
