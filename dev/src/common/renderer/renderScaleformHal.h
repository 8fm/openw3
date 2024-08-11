/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#pragma once

/////////////////////////////////////////////////////////////////////////
#ifdef USE_SCALEFORM
//////////////////////////////////////////////////////////////////////////

#include "renderScaleformMeshCache.h"
#include "renderScaleformShader.h"
#include "renderScaleformTexture.h"
#include <Render/Render_ShaderHAL.h>    // Must be included after platform specific shader includes.
#include "Render/Render_Profiler.h"

//------------------------------------------------------------------------
enum CRenderScaleformHALConfigFlags
{
};

// D3D1x::HALInitParems provides D3D10/D3D11 specific rendering initialization
// parameters for HAL::InitHAL.

struct CRenderScaleformHALInitParams : public SF::Render::HALInitParams
{
	CRenderScaleformHALInitParams( SF::UInt32 halConfigFlags = 0, SF::ThreadId renderThreadId = SF::ThreadId() )
		: SF::Render::HALInitParams(0, halConfigFlags, renderThreadId)
	{ }

	// GpuApi TextureManager accessors for correct type.
	void SetTextureManager( CRenderScaleformTextureManager* manager ) { pTextureManager = manager; }
	CRenderScaleformTextureManager* GetTextureManager() const       { return (CRenderScaleformTextureManager*) pTextureManager.GetPtr(); }
};

//------------------------------------------------------------------------

//forward declare
class CRenderScaleformHAL;

class CRenderScaleformRenderSync : public SF::Render::RenderSync
{
public:
	// Methods that must be overridden per hardware implementation.
	virtual void KickOffFences(SF::Render::FenceType waitType);

	CRenderScaleformRenderSync( CRenderScaleformHAL* hal )
		: m_hal( hal )
	{
	}

protected:

	// Methods that must be overridden per hardware implementation.
	virtual bool   IsPending( SF::Render::FenceType waitType, SF::UInt64 handle, const SF::Render::FenceFrame& parent );
	virtual void   WaitFence( SF::Render::FenceType waitType, SF::UInt64 handle, const SF::Render::FenceFrame& parent );
	virtual SF::UInt64 SetFence();
	virtual void   ReleaseFence( SF::UInt64 apiHandle );

	CRenderScaleformHAL* m_hal;
};

//------------------------------------------------------------------------

class CRenderScaleformHAL : public SF::Render::ShaderHAL<CRenderScaleformShaderManager, CRenderScaleformShaderInterface>
{
private:
	GpuApi::eGUIBlendStateType	m_gpuBlendMode;
	GpuApi::eGUIStencilModeType	m_gpuStencilMode;
	Uint32						m_gpuStencilSet;
	GpuApi::ViewportDesc		m_viewportDesc;
	Bool						m_hackWillSetViewportAfterwardsDuringRenderTargetSetup; // Or maybe just never set it during setviewport...

public:
	typedef SF::Render::ShaderHAL<CRenderScaleformShaderManager, CRenderScaleformShaderInterface> BaseHAL;

public:	
	CRenderScaleformMeshCache						Cache;
	CRenderScaleformRenderSync						RSync;
	SF::Ptr< CRenderScaleformTextureManager >		m_textureManager;

	// Previous batching mode
	SF::Render::PrimitiveBatch::BatchType PrevBatchType;

	// Self-accessor used to avoid constructor warning.
	SF::Render::HAL*      getThis() { return this; }

public:    

	CRenderScaleformHAL( SF::Render::ThreadCommandQueue* commandQueue );
	virtual ~CRenderScaleformHAL();

	// *** HAL Initialization / Shutdown Logic

	// Initializes HAL for rendering.
	virtual SFBool        InitHAL(const CRenderScaleformHALInitParams& params);
	// ShutdownHAL shuts down rendering, releasing resources allocated in InitHAL.
	virtual SFBool        ShutdownHAL();

	// - PrepareForReset should be called before the device's SwapChain is resized.
	// The HAL holds a reference to the default back and depthstencil buffers, which need
	// to be released and reobtained.
	void                PrepareForReset();
	// - RestoreAfterReset called after reset to restore needed variables.
	bool                RestoreAfterReset();


	// *** Rendering
	virtual SFBool        BeginScene();
	virtual SFBool        EndScene();

	// This submits the current context's buffers and resets the states currently set within the HAL.
	// This is used when the context buffers are full, or when a sync requires a wait on queued commands.
	void                SubmitAndResetStates();

	// Updates D3D HW Viewport and ViewportMatrix based on the current
	// values of VP, ViewRect and ViewportValid.
	virtual void        updateViewport();

	virtual SF::UPInt   setVertexArray(SF::Render::PrimitiveBatch* pbatch, SF::Render::MeshCacheItem* pmesh);
	virtual SF::UPInt   setVertexArray(const SF::Render::ComplexMesh::FillRecord& fr, unsigned formatIndex, SF::Render::MeshCacheItem* pmesh);

	// *** Mask Support
	virtual void        applyDepthStencilMode(DepthStencilMode mode, unsigned stencilRef);
	virtual SFBool      checkDepthStencilBufferCaps();

	// *** Rasterization
	virtual SFBool      IsRasterModeSupported(RasterModeType mode) const;
	virtual void        applyRasterModeImpl(RasterModeType mode);

	// *** BlendMode
	void            applyBlendModeImpl(SF::Render::BlendMode mode, SFBool sourceAc = false, SFBool forceAc = false);

	// *** Device States
	enum ColorWriteMode
	{
		Write_All,
		Write_None,
		Write_Count
	};

	static const unsigned BlendTypeCount = SF::Render::Blend_Count*2 + 1;
	static unsigned GetBlendType( SF::Render::BlendMode blendMode, ColorWriteMode writeMode, SFBool sourceAc = false) 
	{ 
		Uint32 base = 0;
		if ( writeMode == Write_None )
		{
			base = SF::Render::Blend_Count*2;
			blendMode = (SF::Render::BlendMode)0;
		}
		else if ( sourceAc )
			base += SF::Render::Blend_Count;
		return base + (Uint32)blendMode;
	}

//	ID3D11BlendState* BlendStates[BlendTypeCount];

//	ID3D1x(Device)*   GetDevice() const { return pDevice; }

	static const Uint32   ConstantBufferCount = 8;
	GpuApi::BufferRef	ConstantBuffers[ConstantBufferCount];
	Uint32				CurrentConstantBuffer;

	SFBool				createConstantBuffers();
	void                destroyConstantBuffers();
	const GpuApi::BufferRef&    getNextConstantBuffer();

	virtual SF::Render::TextureManager* GetTextureManager() const
	{
		return m_textureManager.GetPtr();
	}

	virtual SF::Render::RenderTarget*   CreateRenderTarget( GpuApi::TextureRef& renderTarget, GpuApi::TextureRef& depthStencil );
	virtual SF::Render::RenderTarget*   CreateRenderTarget(SF::Render::Texture* texture, SFBool needsStencil);
	virtual SF::Render::RenderTarget*   CreateTempRenderTarget(const SF::Render::ImageSize& size, SFBool needsStencil);
	virtual SFBool          SetRenderTarget(SF::Render::RenderTarget* target, bool setState = 1);
	virtual void            PushRenderTarget(const SF::Render::RectF& frameRect, SF::Render::RenderTarget* prt, SFUInt flags =0, SF::Render::Color clearColor =0);
	virtual void            PopRenderTarget(SFUInt flags = 0);

	virtual SFBool            createDefaultRenderBuffer();

	virtual class CRenderScaleformMeshCache&       GetMeshCache()        { return Cache; }
	virtual SF::Render::RenderSync*				   GetRenderSync()       { return &RSync; }

	virtual void    MapVertexFormat(SF::Render::PrimitiveFillType fill, const SF::Render::VertexFormat* sourceFormat,
		const SF::Render::VertexFormat** single,
		const SF::Render::VertexFormat** batch, const SF::Render::VertexFormat** instanced, SFUInt = 0)
	{
		SManager.MapVertexFormat(fill, sourceFormat, single, batch, instanced);
	}

protected:

	virtual void setBatchUnitSquareVertexStream();
	virtual void drawPrimitive(SFUInt indexCount, SFUInt meshCount);
	virtual void drawIndexedPrimitive(SFUInt indexCount, SFUInt vertexCount, SFUInt meshCount, SF::UPInt indexPtr, SF::UPInt vertexOffset );
	virtual void drawIndexedInstanced(SFUInt indexCount, SFUInt vertexCount, SFUInt meshCount, SF::UPInt indexPtr, SF::UPInt vertexOffset );

	// Returns whether the profile can render any of the filters contained in the FilterPrimitive.
	// If a profile does not support dynamic looping (eg. using SM2.0), no blur/shadow type filters
	// can be rendered, in which case this may return false, however, ColorMatrix filters can still be rendered.
	SFBool        shouldRenderFilters(const SF::Render::FilterPrimitive* prim) const;

	// *** Events
	virtual SF::Render::RenderEvent& GetEvent(SF::Render::EventType eventType);

	void drawScreenQuad();
};

// Use this HAL if you want to use profile modes.
class ProfilerHAL : public SF::Render::ProfilerHAL<CRenderScaleformHAL>
{
public:
	ProfilerHAL( SF::Render::ThreadCommandQueue* commandQueue )
		: SF::Render::ProfilerHAL<CRenderScaleformHAL>(commandQueue)
	{
	}
};

//--------------------------------------------------------------------
// RenderTargetData, used for both RenderTargets and DepthStencilSurface implementations.
class CRenderScaleformRenderTargetData : public SF::Render::RenderBuffer::RenderTargetData
{
public:
	friend class CRenderScaleformHAL;
	GpuApi::TextureRef		m_renderTarget;
	GpuApi::TextureRef		m_depthStencil;
	//ID3D1x(View)*           pRenderSurface;         // View of the render target.
	//ID3D1x(View)*           pDSSurface;             // View of the depth stencil surface (0 if not required).

	static void UpdateData( SF::Render::RenderBuffer* buffer, GpuApi::TextureRef& renderTarget, 
		SF::Render::DepthStencilBuffer* pdsb, GpuApi::TextureRef& depthStencil )
	{
		if ( !buffer )
			return;

		CRenderScaleformRenderTargetData* poldHD = (CRenderScaleformRenderTargetData*)buffer->GetRenderTargetData();
		if ( !poldHD )
		{
			poldHD = SF_NEW CRenderScaleformRenderTargetData(buffer, renderTarget, pdsb, depthStencil);
			buffer->SetRenderTargetData(poldHD);
			return;
		}

		GpuApi::AddRefIfValid( renderTarget );
		GpuApi::AddRefIfValid( depthStencil );
		
		if ( poldHD->m_renderTarget )
		{
			GpuApi::Release( poldHD->m_renderTarget );
		}
		if ( poldHD->m_depthStencil )
		{
			GpuApi::Release( poldHD->m_depthStencil );
		}

		poldHD->pDepthStencilBuffer = pdsb;
		poldHD->m_depthStencil = depthStencil;
		poldHD->m_renderTarget = renderTarget;
	}

	virtual ~CRenderScaleformRenderTargetData()
	{
		if ( m_renderTarget )
		{
			GpuApi::Release( m_renderTarget );
		}
		if ( m_depthStencil )
		{
			GpuApi::Release( m_depthStencil );
		}
	}

private:
	CRenderScaleformRenderTargetData( SF::Render::RenderBuffer* buffer, GpuApi::TextureRef& renderTarget, SF::Render::DepthStencilBuffer* pdsb, GpuApi::TextureRef& depthStencil) : 
		SF::Render::RenderBuffer::RenderTargetData(buffer, pdsb), m_renderTarget(renderTarget), m_depthStencil(depthStencil)
		{
			AddRefIfValid( m_renderTarget );
			AddRefIfValid( m_depthStencil );
		}
};

/////////////////////////////////////////////////////////////////////////
#endif // USE_SCALEFORM
//////////////////////////////////////////////////////////////////////////