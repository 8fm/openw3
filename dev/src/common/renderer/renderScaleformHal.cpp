/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

/////////////////////////////////////////////////////////////////////////
#ifdef USE_SCALEFORM
//////////////////////////////////////////////////////////////////////////

//#include <Kernel/SF_Debug.h>
#include <Kernel/SF_Random.h>

#include "renderScaleformHal.h"

#include <Render/Render_BufferGeneric.h>
#include <Kernel/SF_HeapNew.h>
#include <Render/Render_TextureCacheGeneric.h>

//FIXME: Why for hacks namespace? Can't include gpuApiBase.h
#if defined( RED_PLATFORM_DURANGO )
namespace GpuApi
{
	namespace Hacks
	{
		ID3D11Device*				GetDevice();
		ID3D11DeviceContext*		GetDeviceContext();
		ID3D11RenderTargetView*		GetTextureRTV( const TextureRef& ref );
		ID3D11ShaderResourceView*	GetTextureSRV( const TextureRef& ref );
		ID3D11Texture2D*			GetTexture( const TextureRef& ref );
		ID3D11PixelShader*			GetPixelShader( const ShaderRef& ref );
	}
}
#elif defined( RED_PLATFORM_ORBIS )
namespace GpuApi
{
	namespace Hacks
	{
		sce::Gnmx::GfxContext& GetGfxContext();
		sce::Gnm::RenderTarget& GetRenderTarget();
		sce::Gnm::DepthRenderTarget& GetDepthStencil();
	}
}
#endif

// For SF_AMP_SCOPE_TIMER
using SF::AmpFunctionTimer;
using SF::AmpServer;


//////////////////////////////////////////////////////////////////////////

#define DRAWCONTEXT_REFVALUE( blend, stencil, refVal ) ((Uint8)blend | (((Uint8)stencil) << 8 ) | (((Uint8)refVal) << 16 ) )

 static const GpuApi::eGUIBlendStateType GpuApiBlendModeStates[] =
 {
	 GpuApi::GUI_BlendDesc_None,
	 GpuApi::GUI_BlendDesc_Normal,
	 GpuApi::GUI_BlendDesc_Layer,
	 GpuApi::GUI_BlendDesc_Multiply,
	 GpuApi::GUI_BlendDesc_Screen,
	 GpuApi::GUI_BlendDesc_Lighten,
	 GpuApi::GUI_BlendDesc_Darken,
	 GpuApi::GUI_BlendDesc_Difference,
	 GpuApi::GUI_BlendDesc_Add,
	 GpuApi::GUI_BlendDesc_Subtract,
	 GpuApi::GUI_BlendDesc_Invert,
	 GpuApi::GUI_BlendDesc_Alpha,
	 GpuApi::GUI_BlendDesc_Erase,
	 GpuApi::GUI_BlendDesc_Overlay,
	 GpuApi::GUI_BlendDesc_HardLight,
	 GpuApi::GUI_BlendDesc_Overwrite,
	 GpuApi::GUI_BlendDesc_OverwriteAll,
	 GpuApi::GUI_BlendDesc_FullAdditive,
	 GpuApi::GUI_BlendDesc_FilterBlend,
	 GpuApi::GUI_BlendDesc_Ignore,

	 GpuApi::GUI_BlendDesc_None_SourceAc,
	 GpuApi::GUI_BlendDesc_Normal_SourceAc,
	 GpuApi::GUI_BlendDesc_Layer_SourceAc,
	 GpuApi::GUI_BlendDesc_Multiply_SourceAc,
	 GpuApi::GUI_BlendDesc_Screen_SourceAc,
	 GpuApi::GUI_BlendDesc_Lighten_SourceAc,
	 GpuApi::GUI_BlendDesc_Darken_SourceAc,
	 GpuApi::GUI_BlendDesc_Difference_SourceAc,
	 GpuApi::GUI_BlendDesc_Add_SourceAc,
	 GpuApi::GUI_BlendDesc_Subtract_SourceAc,
	 GpuApi::GUI_BlendDesc_Invert_SourceAc,
	 GpuApi::GUI_BlendDesc_Alpha_SourceAc,
	 GpuApi::GUI_BlendDesc_Erase_SourceAc,
	 GpuApi::GUI_BlendDesc_Overlay_SourceAc,
	 GpuApi::GUI_BlendDesc_HardLight_SourceAc,
	 GpuApi::GUI_BlendDesc_Overwrite_SourceAc,
	 GpuApi::GUI_BlendDesc_OverwriteAll_SourceAc,
	 GpuApi::GUI_BlendDesc_FullAdditive_SourceAc,
	 GpuApi::GUI_BlendDesc_FilterBlend_SourceAc,
	 GpuApi::GUI_BlendDesc_Ignore_SourceAc,

	 GpuApi::GUI_BlendDesc_NoColorWrite,
 };

 static const GpuApi::eGUIStencilModeType GpuApiDepthStencilStates[] =
 {
	 GpuApi::GUI_Stencil_Invalid,
	 GpuApi::GUI_Stencil_Disabled,
	 GpuApi::GUI_Stencil_StencilClear,
	 GpuApi::GUI_Stencil_StencilClearHigher,
	 GpuApi::GUI_Stencil_StencilIncrementEqual,
	 GpuApi::GUI_Stencil_StencilTestLessEqual,
	 GpuApi::GUI_Stencil_DepthWrite,
	 GpuApi::GUI_Stencil_DepthTestEqual,
 };

//////////////////////////////////////////////////////////////////////////
// ***** sync

void CRenderScaleformRenderSync::KickOffFences( SF::Render::FenceType waitType )
{
	SF_UNUSED(waitType);
	m_hal->SubmitAndResetStates();
}

bool CRenderScaleformRenderSync::IsPending( SF::Render::FenceType waitType, SF::UInt64 handle, const SF::Render::FenceFrame& parent )
{
	return GpuApi::IsFencePending( handle );
}

void CRenderScaleformRenderSync::WaitFence( SF::Render::FenceType waitType, SF::UInt64 handle, const SF::Render::FenceFrame& parent )
{
	GpuApi::InsertWaitOnFence( handle );
}

SF::UInt64 CRenderScaleformRenderSync::SetFence(  )
{
	return GpuApi::InsertFence();
}

void CRenderScaleformRenderSync::ReleaseFence( SF::UInt64 apiHandle )
{
	GpuApi::ReleaseFence( apiHandle );
}

//////////////////////////////////////////////////////////////////////////
// ***** HAL_D3D1x

CRenderScaleformHAL::CRenderScaleformHAL(SF::Render::ThreadCommandQueue* commandQueue)
	: SF::Render::ShaderHAL<CRenderScaleformShaderManager, CRenderScaleformShaderInterface>(commandQueue)
	, m_gpuBlendMode( GpuApi::GUI_BlendDesc_None )
	, m_gpuStencilMode( GpuApi::GUI_Stencil_Disabled )
	, m_gpuStencilSet( 0 )
	, m_hackWillSetViewportAfterwardsDuringRenderTargetSetup( false )
	, Cache(SF::Memory::GetGlobalHeap(), SF::Render::MeshCacheParams::PC_Defaults)
	, RSync( this )
	, PrevBatchType(SF::Render::PrimitiveBatch::DP_None)
	, CurrentConstantBuffer(0)
	, m_textureManager( nullptr )
{
	// Set to supported viewport size for when GpuApi
	// sets the render targets before a valid viewport is later set.
	m_viewportDesc.Set( 1, 1, 0, 0, 0.f, 0.f );

	RED_ASSERT( CurrentStencilRef == 0 );
	RED_ASSERT( CurrentDepthStencilState == DepthStencil_Disabled );
	RED_ASSERT( CurrentBlendState.Mode == SF::Render::Blend_None );
}

CRenderScaleformHAL::~CRenderScaleformHAL()
{
	ShutdownHAL();
}

// *** RenderHAL_D3D1x Implementation
	
SFBool CRenderScaleformHAL::InitHAL(const CRenderScaleformHALInitParams& params)
{
	if ( !initHAL(params))
		return false;

	if ( ! GpuApi::IsInit() )
	{
		return false;
	}

//    ScaleformGpuApi::RenderEvent::InitializeEvents( GpuApi::Hacks::GetDeviceContext() );

	if ( createConstantBuffers() && 
		 SManager.Initialize(this) &&
		 Cache.Initialize(&SManager))
	{
		// Create Texture manager if needed.
		if (params.pTextureManager)
		{
			m_textureManager = params.GetTextureManager();
		}
		else
		{
			m_textureManager = *SF_HEAP_AUTO_NEW(this) CRenderScaleformTextureManager(params.RenderThreadId, pRTCommandQueue);
			if (!m_textureManager)
			{
				Cache.Reset();
				SManager.Reset();
			}
		}

		// Create RenderBufferManager if needed.
		if (params.pRenderBufferManager)
			pRenderBufferManager = params.pRenderBufferManager;
		else
		{
			// Create the default render target, and manager.
			pRenderBufferManager = *SF_HEAP_AUTO_NEW(this) SF::Render::RenderBufferManagerGeneric(SF::Render::RBGenericImpl::DSSM_Exact);
			if ( !pRenderBufferManager || !createDefaultRenderBuffer() )
			{
				ShutdownHAL();
				return false;
			}
		}

		// Success.
		if ( m_textureManager && pRenderBufferManager )
		{
			HALState|= HS_ModeSet;
			notifyHandlers(SF::Render::HAL_Initialize);
			return true;
		}
	}

	// Failure.  
	return false;
}

// Returns back to original mode (cleanup)
bool CRenderScaleformHAL::ShutdownHAL()
{
	if (!(HALState & HS_ModeSet))
		return true;

	if (!shutdownHAL())
		return false;

 //   ScaleformGpuApi::RenderEvent::ShutdownEvents();

	// Destroy device states.
	destroyConstantBuffers();
	destroyRenderBuffers();
	pRenderBufferManager.Clear();

	// Do TextureManager::Reset to ensure shutdown on the current thread.

	m_textureManager.Clear();

	SManager.Reset();
	Cache.Reset();

	m_gpuBlendMode = GpuApi::GUI_BlendDesc_None;
	m_gpuStencilMode = GpuApi::GUI_Stencil_Disabled;
	m_gpuStencilSet = 0;
	m_viewportDesc.Set( 1, 1, 0, 0, 0.f, 0.f );

	RED_WARNING( CurrentStencilRef == 0, "Stencil ref %u, expected 0", CurrentStencilRef );
	RED_WARNING( CurrentDepthStencilState == DepthStencil_Disabled, "Depth stencil value %u, expected DepthStencil_Disabled", (Uint32)CurrentDepthStencilState );
	RED_WARNING( CurrentBlendState.Mode == SF::Render::Blend_None, "Blend state value %u, expected Blend_None", (Uint32)CurrentBlendState.Mode );

	return true;
}

void CRenderScaleformHAL::PrepareForReset()
{
	ASSERT(HALState & HS_ModeSet);
	if (HALState & HS_ReadyForReset)
		return;

	// Release the default rendertarget, and depth stencil references.
	if ( RenderTargetStack.GetSize() > 0 )
	{
		RenderTargetEntry& entry = RenderTargetStack[0];
		if ( entry.pRenderTarget )
		{
			ASSERT( entry.pRenderTarget->GetType() == SF::Render::RBuffer_Default);
			CRenderScaleformRenderTargetData* phd = (CRenderScaleformRenderTargetData*)entry.pRenderTarget->GetRenderTargetData();
			if ( phd )
			{
				GpuApi::SafeRelease( phd->m_renderTarget );
				GpuApi::SafeRelease( phd->m_depthStencil );
			}
		}
	}
	
	notifyHandlers(SF::Render::HAL_PrepareForReset);
	HALState |= HS_ReadyForReset;
}

bool CRenderScaleformHAL::RestoreAfterReset()
{
	if (!IsInitialized())
		return false;
	if (!(HALState & HS_ReadyForReset))
		return true;

	// Reobtain the default rendertarget and depthstencil references.
	if ( RenderTargetStack.GetSize() > 0 )
	{
		RenderTargetEntry& entry = RenderTargetStack[0];

		// We'll AddRef to these buffers since we're not implicitly keeping an
		// addref by using the D3D "get" function, and we're not addref'ing by
		// calling the update render target data function.
		GpuApi::RenderTargetSetup rtSetup = GpuApi::GetRenderTargetSetup();
		
		CRenderScaleformRenderTargetData* phd = 0;
		if ( entry.pRenderTarget && rtSetup.numColorTargets > 0 && rtSetup.colorTargets[0])
		{
			ASSERT( entry.pRenderTarget->GetType() == SF::Render::RBuffer_Default);
			phd = (CRenderScaleformRenderTargetData*)entry.pRenderTarget->GetRenderTargetData();
			ASSERT( phd->m_renderTarget == 0 );
			phd->m_renderTarget = rtSetup.colorTargets[0];
			GpuApi::AddRef( rtSetup.colorTargets[0] );
			if ( rtSetup.depthTarget )
			{
				GpuApi::AddRef( rtSetup.depthTarget );
				phd->m_depthStencil = rtSetup.depthTarget;
			}
		}
	}

	m_gpuBlendMode = GpuApi::GUI_BlendDesc_None;
	m_gpuStencilMode = GpuApi::GUI_Stencil_Disabled;
	m_gpuStencilSet = 0;

// 	RED_ASSERT( CurrentStencilRef == 0 );
// 	RED_ASSERT( CurrentDepthStencilState == DepthStencil_Disabled );
// 	RED_ASSERT( CurrentBlendState.Mode == SF::Render::Blend_None );
	
	m_viewportDesc.Set( 1, 1, 0, 0, 0.f, 0.f );

	notifyHandlers(SF::Render::HAL_RestoreAfterReset);
	HALState &= ~HS_ReadyForReset;
	return true;
}

// ***** Rendering

SFBool CRenderScaleformHAL::BeginScene()
{
	// Render event for anything that is done immediately within BeginScene (but not the entire scene).
	SF::Render::ScopedRenderEvent GPUEvent(GetEvent(SF::Render::Event_BeginScene), "CRenderScaleformHAL::BeginScene-SetState");

	if ( !BaseHAL::BeginScene() )
		return false;

	m_gpuStencilMode = GpuApi::GUI_Stencil_Disabled;
	m_gpuStencilSet = 0;
	GpuApi::SetDrawContext( GpuApi::DRAWCONTEXT_Scaleform2D, DRAWCONTEXT_REFVALUE( m_gpuBlendMode, m_gpuStencilMode, m_gpuStencilSet ) );

	CurrentConstantBuffer = 0;

	// Start the scene
	SManager.BeginScene();
	ShaderData.BeginScene();
	return true;
}

SFBool CRenderScaleformHAL::EndScene()
{
	SF::Render::ScopedRenderEvent GPUEvent(GetEvent(SF::Render::Event_Scene), 0, false);

	if ( !BaseHAL::EndScene())
		return false;

	return true;
}

// This call submits the current command buffers, and resets all render states, such that rendering can continue. This method
// is required for the Orbis::RenderSync, as it must submit the current context for the GPU to finish using resources.
void CRenderScaleformHAL::SubmitAndResetStates()
{
	GpuApi::Flush();

	// Set the render target again.
	CRenderScaleformRenderTargetData* prtdata = (CRenderScaleformRenderTargetData*)(RenderTargetStack.Back().pRenderTarget->GetRenderTargetData());
	GpuApi::TextureRef& renderTarget = prtdata->m_renderTarget;
	GpuApi::TextureRef& depthStencil = prtdata->m_depthStencil;
	GpuApi::RenderTargetSetup rtSetup;
	rtSetup.SetColorTarget( 0, renderTarget );
	rtSetup.SetDepthStencilTarget( depthStencil );
	rtSetup.SetViewport( m_viewportDesc );
	GpuApi::SetupRenderTargets( rtSetup );

	// Set depth/stencil states to the correct ones, based on the current mask state.
	DepthStencilMode currentMode = CurrentDepthStencilState;
	unsigned currentRef = CurrentStencilRef;
	CurrentDepthStencilState = DepthStencil_Invalid;
	CurrentStencilRef = (unsigned)-1;
	applyDepthStencilMode(currentMode, currentRef);

	// this is set for every drawcall

	//// We always use 16-bit indices, and trilists.
	//GnmxCtx->setIndexSize(sce::Gnm::IndexSize::kIndexSize16);
	//GnmxCtx->setPrimitiveType(sce::Gnm::PrimitiveType::kPrimitiveTypeTriList);
	//GnmxCtx->setIndexOffset(0);

	//// Uncache previous batch type (in case instancing was used externally).
	//PrevBatchType = PrimitiveBatch::DP_None;

	// Reset any cached shaders.
	SManager.BeginScene();
	ShaderData.BeginScene();

	// Update viewport/blend and raster modes.
	updateViewport();
	applyBlendModeImpl(CurrentBlendState.Mode, CurrentBlendState.SourceAc, CurrentBlendState.ForceAc);
	applyRasterModeImpl(CurrentSceneRasterMode);
}

// Updates D3D HW Viewport and ViewportMatrix based on provided viewport
// and view rectangle.
void CRenderScaleformHAL::updateViewport()
{
	SF::Render::Rect<int>      vpRect;

	if (HALState & HS_ViewValid)
	{
		int dx = ViewRect.x1 - VP.Left,
			dy = ViewRect.y1 - VP.Top;
		
		// Modify HW matrix and viewport to clip.
		CalcHWViewMatrix(0, &Matrices->View2D, ViewRect, dx, dy);
		Matrices->SetUserMatrix(Matrices->User);
		Matrices->ViewRect    = ViewRect;
		Matrices->UVPOChanged = 1;

		/*
		// TBD: Prepend UserMatrix here is incorrect for nested viewport-based
		// mask clipping; what's needed is a complex combination of viewport and
		// coordinate adjustment. Until this is done, mask viewport clipping will be
		// in the wrong place for UserMatrix.
		if (UserMatrix != Matrix2F::Identity)
		{
			Rect<int> viewportRect;
			Rect<int> userViewRect(
				ViewRect.x1 + (int)UserMatrix.Tx(),
				ViewRect.y1 + (int)UserMatrix.Ty(),
				Size<int>((int)(UserMatrix.Sx() * (float)ViewRect.Width()),
						  (int)(UserMatrix.Sy() * (float)ViewRect.Height())));

			VP.GetClippedRect(&viewportRect);
			viewportRect.IntersectRect(&vpRect, userViewRect);
		}
		*/

		if ( !(HALState & HS_InRenderTarget))
			vpRect = ViewRect;
		else
			vpRect.SetRect(VP.Left, VP.Top, VP.Left + VP.Width, VP.Top + VP.Height);
	}

	m_viewportDesc.x = static_cast< Uint32 >( vpRect.x1 );
	m_viewportDesc.y = static_cast< Uint32 >( vpRect.y1 );

	// DX can't handle a vp with zero area.
	m_viewportDesc.width = ::Max< Uint32 >( vpRect.Width(), 1 );
	m_viewportDesc.height = ::Max< Uint32 >( vpRect.Height(), 1 );
	m_viewportDesc.minZ = 0.0f;
	m_viewportDesc.maxZ = 0.0f;

	// FIXME:/TODO: NO GUARANTEE THIS WILL BE CALLED AFTER SETTING RENDER TARGETS (MAYBE CALLED BEFORE....)
	// COULD THEN CACHE THE VIEWPORT VALUE AND SET IT THOUGH INSTEAD OF THE HACK FLAG

	// Virtual function override so can't just pass this as a parameter
	
	// Or could just set it anyway, but going to be set in GpuApi::SetupRenderTargets which has to come after
	// setting the viewport here (or else it'll be called with a potentially invalid viewport size)
	// All part of how SetupRenderTargets wants to do too much.
	if ( ! m_hackWillSetViewportAfterwardsDuringRenderTargetSetup )
	{
		GpuApi::SetViewportRaw( m_viewportDesc );
	}
}

SF::UPInt CRenderScaleformHAL::setVertexArray(SF::Render::PrimitiveBatch* pbatch, SF::Render::MeshCacheItem* pmeshBase)
{
	CRenderScaleformMeshCacheItem* pmesh = reinterpret_cast<CRenderScaleformMeshCacheItem*>(pmeshBase);
	const GpuApi::BufferRef& pb			 = pmesh->pVertexBuffer->GetHWBuffer();
	Uint32 offset						 = (Uint32)pmesh->VBAllocOffset;

	GpuApi::BindIndexBuffer( pmesh->pIndexBuffer->GetHWBuffer() );
	GpuApi::BindVertexBuffers( 0, 1, &pb, &(pbatch->pFormat->Size), &offset );

	return pmesh->IBAllocOffset / sizeof (SF::Render::IndexType);
}

SF::UPInt CRenderScaleformHAL::setVertexArray(const SF::Render::ComplexMesh::FillRecord& fr, SFUInt formatIndex, SF::Render::MeshCacheItem* pmeshBase)
{
	CRenderScaleformMeshCacheItem* pmesh	= reinterpret_cast<CRenderScaleformMeshCacheItem*>(pmeshBase);
	const GpuApi::BufferRef& pb				= (pmesh->pVertexBuffer->GetHWBuffer());
	Uint32 byteOffset						= (Uint32)(fr.VertexByteOffset + pmesh->VBAllocOffset);
	const SF::Render::VertexFormat* pformat = fr.pFormats[formatIndex];
	Uint32 vertexSize						= pformat->Size;

	GpuApi::BindVertexBuffers( 0, 1, &pb, &vertexSize, &byteOffset );
	GpuApi::BindIndexBuffer( pmesh->pIndexBuffer->GetHWBuffer() );

	return (SFUInt)(pmesh->IBAllocOffset / sizeof(SF::Render::IndexType));
}

//--------------------------------------------------------------------
// *** Mask / Stencil support
//--------------------------------------------------------------------
void CRenderScaleformHAL::applyDepthStencilMode(DepthStencilMode mode, SFUInt stencilRef)
{
	SF::Render::ScopedRenderEvent GPUEvent(GetEvent(SF::Render::Event_ApplyDepthStencil), __FUNCTION__);

	// GpuApi will do redundancy checking, but just to keep some of the logic more obvious here
	if (mode != CurrentDepthStencilState || CurrentStencilRef != stencilRef)
	{
		//pDeviceContext->OMSetDepthStencilState(DepthStencilStates[mode], (UINT)stencilRef);
		//*** DBlock: Note how it doesn't update always update CurrentDepthStencilState below. ***
		m_gpuStencilMode = GpuApiDepthStencilStates[ mode ];
		m_gpuStencilSet = stencilRef;
		GpuApi::SetDrawContext( GpuApi::DRAWCONTEXT_Scaleform2D, DRAWCONTEXT_REFVALUE( m_gpuBlendMode, m_gpuStencilMode, stencilRef ) );
		CurrentStencilRef = stencilRef;

		if (DepthStencilModeTable[mode].ColorWriteEnable != DepthStencilModeTable[CurrentDepthStencilState].ColorWriteEnable)
		{
			// applyBlendModeImpl will check the CurrentDepthStencilState, and set CWE appropriately, so set it before calling.
			// Note that we are calling the Impl directly, as applyBlendMode may detect this as a duplicate setting of the same state.
			CurrentDepthStencilState = mode;
			applyBlendModeImpl(CurrentBlendState.Mode, CurrentBlendState.SourceAc, CurrentBlendState.ForceAc);
		}
		CurrentDepthStencilState = mode;
	}
}

SFBool CRenderScaleformHAL::checkDepthStencilBufferCaps()
{
	if (!StencilChecked)
	{
		const GpuApi::RenderTargetSetup& rtSetup = GpuApi::GetRenderTargetSetup();

		// Test for depth-stencil presence.
		// NOTE: Cases not handled in the integration since they're not in the GpuApi mapping (add here if ever really used).
		// DXGI_FORMAT_D32_FLOAT_S8X24_UINT, DXGI_FORMAT_D16_UNORM, DXGI_FORMAT_D32_FLOAT. The DSV might be bound to the right
		// type, but can only check the resource type.
		if ( rtSetup.depthTarget )
		{
			GpuApi::TextureDesc desc;
			GpuApi::GetTextureDesc( rtSetup.depthTarget, desc );
			switch ( desc.format )
			{
				case GpuApi::TEXFMT_D24S8:
					StencilAvailable = 1;
					MultiBitStencil  = 1;
					DepthBufferAvailable = 1;
					break;
				default:
					ASSERT( false, TXT( "Unexpected depth surface format %u" ), desc.format );
					break;
			}
		}

		StencilChecked = 1;
	}

	if (!StencilAvailable && !DepthBufferAvailable)
	{
		WARN_RENDERER( TXT("RendererHAL::PushMask_BeginSubmit used, but stencil is not available") );
		return false;
	}
	return true;
}

SFBool CRenderScaleformHAL::IsRasterModeSupported(RasterModeType mode) const
{
	//FIXME>>>>>>>>>>>>>>> Make agnostic!
	// D3D1x doesn't support point.
	return mode != HAL::RasterMode_Point;
}

void CRenderScaleformHAL::applyRasterModeImpl(RasterModeType mode)
{
	//FIXME>>>>>>>>>>>>>>> Make agnostic!
	//pDeviceContext->RSSetState(RasterStates[mode]);
}


//--------------------------------------------------------------------
// *** BlendMode Stack support
//--------------------------------------------------------------------

bool CRenderScaleformHAL::createConstantBuffers()
{
	// NOTE: Resetting here to keep the same behavior as the reference implementation.
	// Make sure we never have old buffers at least.
	for ( Uint32 i = 0; i < ARRAY_COUNT( ConstantBuffers ); ++i )
	{
		ConstantBuffers[ i ] = GpuApi::BufferRef::Null();
	}

	// LAVA: on PS4 settings this size to something more sane then 1036 ... 
	// TODO: understand what the fuck happens here
#ifdef RED_PLATFORM_ORBIS
	const Uint32 bufferSize = 128 * 4 * sizeof(Float);
#else
	const Uint32 bufferSize = SF::Alg::Max( ScaleformGpuApi::Uniform::SU_VertexSize, ScaleformGpuApi::Uniform::SU_FragSize) * 4 * sizeof(Float);
#endif

	for ( Uint32 b = 0; b < ConstantBufferCount; ++b )
	{
		ConstantBuffers[ b ] = GpuApi::CreateBuffer( bufferSize, GpuApi::BCC_Constant, GpuApi::BUT_Dynamic, GpuApi::BAF_CPUWrite );
		const GpuApi::BufferRef& cbuffer = ConstantBuffers[ b ];

		if ( ! cbuffer )
		{
			return false;
		}

		GpuApi::SetBufferDebugPath( cbuffer, "SF cbuffer" );
	}
	return true;
}

void CRenderScaleformHAL::destroyConstantBuffers()
{
	for ( Uint32 b = 0; b < ConstantBufferCount; ++b )
	{
		GpuApi::BufferRef& cbuffer = ConstantBuffers[b];
		GpuApi::SafeRelease( cbuffer );
	}
}

const GpuApi::BufferRef& CRenderScaleformHAL::getNextConstantBuffer()
{
	CurrentConstantBuffer++; 
	CurrentConstantBuffer %= ConstantBufferCount; 
	return ConstantBuffers[CurrentConstantBuffer];
}

void CRenderScaleformHAL::applyBlendModeImpl(SF::Render::BlendMode mode, SFBool sourceAc, SFBool forceAc )
{    
	SF_UNUSED(forceAc);
	if ( ! GpuApi::IsInit() )
	{
		return;
	}

	Uint32 blendType = GetBlendType(mode, DepthStencilModeTable[CurrentDepthStencilState].ColorWriteEnable ? Write_All : Write_None, sourceAc);
	
	m_gpuBlendMode = GpuApiBlendModeStates[ blendType ];
	GpuApi::SetDrawContext( GpuApi::DRAWCONTEXT_Scaleform2D, DRAWCONTEXT_REFVALUE( m_gpuBlendMode, m_gpuStencilMode, m_gpuStencilSet ) );
}

SF::Render::RenderTarget*   CRenderScaleformHAL::CreateRenderTarget( GpuApi::TextureRef& renderTarget, GpuApi::TextureRef& depthStencil )
{
	SF::Render::ImageSize dsSize(0), rtSize(0);

	GpuApi::TextureDesc rtDesc;
	GpuApi::GetTextureDesc( renderTarget, rtDesc );

	rtSize.SetSize(rtDesc.width, rtDesc.height);

	SF::Ptr<SF::Render::RenderTarget> ptarget = pRenderBufferManager->CreateRenderTarget(rtSize, SF::Render::RBuffer_User, SF::Render::Image_R8G8B8A8, 0);

	SF::Ptr<SF::Render::DepthStencilBuffer> pdsb = 0;
	if ( depthStencil )
	{
		GpuApi::TextureDesc dsDesc;
		GpuApi::GetTextureDesc( depthStencil, dsDesc );
		SF::Render::ImageSize dsSize(dsDesc.width, dsDesc.height);
		pdsb = *SF_HEAP_AUTO_NEW(this) SF::Render::DepthStencilBuffer(0, dsSize);
	}

	CRenderScaleformRenderTargetData::UpdateData(ptarget, renderTarget, pdsb, depthStencil);
	return ptarget;
}

SF::Render::RenderTarget* CRenderScaleformHAL::CreateRenderTarget(SF::Render::Texture* texture, SFBool needsStencil)
{
	CRenderScaleformTexture* pt = (CRenderScaleformTexture*)texture;

	// Cannot render to textures which have multiple HW representations.
	if ( !pt || pt->TextureCount != 1 )
		return 0;
	SF::Render::RenderTarget* prt = pRenderBufferManager->CreateRenderTarget(
		texture->GetSize(), SF::Render::RBuffer_Texture, texture->GetFormat(), texture);   
	if ( !prt )
		return 0;

  
	GpuApi::TextureRef depthStencil = GpuApi::TextureRef::Null();

	// FIXME:/NOTE: GpuApi should have already created the render target if the right flags were set...
	//     if ( !prt || FAILED(pDevice->CreateRenderTargetView(pt->pTextures[0].pTexture, 0, &prtView.GetRawRef() )) )
	//         return 0;
	GpuApi::TextureRef& renderTarget = pt->m_textureArray[0].m_texture; // SF addref'd with a scoped smart pointer, but not really necessary...  // Ptr<ID3D1x(RenderTargetView)> prtView;

	GpuApi::TextureDesc desc;
	GpuApi::GetTextureDesc( renderTarget, desc );
	ASSERT( ( desc.usage & GpuApi::TEXUSAGE_RenderTarget ) );
	if ( ! ( desc.usage & GpuApi::TEXUSAGE_RenderTarget ) )
	{
		return 0;
	}

	SF::Ptr<SF::Render::DepthStencilBuffer> pdsb =0;
	if ( needsStencil )
	{
		pdsb = *pRenderBufferManager->CreateDepthStencilBuffer(texture->GetSize());
		if ( pdsb )
		{
			CRenderScaleformDepthStencilSurface* surf = (CRenderScaleformDepthStencilSurface*)pdsb->GetSurface();
			if ( surf )
				depthStencil = surf->m_depthStencilTexture;
		}
	}
	CRenderScaleformRenderTargetData::UpdateData(prt, renderTarget, pdsb, depthStencil);
	return prt;
}

SF::Render::RenderTarget* CRenderScaleformHAL::CreateTempRenderTarget(const SF::Render::ImageSize& size, SFBool needsStencil)
{
	SF::Render::RenderTarget* prt = pRenderBufferManager->CreateTempRenderTarget(size);
	if ( !prt )
		return 0;

	CRenderScaleformRenderTargetData* phd = (CRenderScaleformRenderTargetData*)prt->GetRenderTargetData();
	if ( phd && (!needsStencil || phd->pDepthStencilBuffer))
		return prt;

	CRenderScaleformTexture* pt = (CRenderScaleformTexture*)prt->GetTexture();

	// Cannot render to textures which have multiple HW representations.
	ASSERT(pt->TextureCount == 1); 

	// NOTE: GpuApi should have already created the render target view if the right flags were set
	GpuApi::TextureRef& renderTarget = pt->m_textureArray[0].m_texture;
	GpuApi::TextureRef depthStencil = GpuApi::TextureRef::Null();

	GpuApi::TextureDesc desc;
	GpuApi::GetTextureDesc( renderTarget, desc );
	ASSERT( ( desc.usage & GpuApi::TEXUSAGE_RenderTarget ) );
	if ( ! ( desc.usage & GpuApi::TEXUSAGE_RenderTarget ) )
	{
		return 0;
	}

	SF::Ptr<SF::Render::DepthStencilBuffer> pdsb = 0;
	if ( needsStencil )
	{   
		pdsb = *pRenderBufferManager->CreateDepthStencilBuffer(pt->GetSize());
		if ( pdsb )
		{
			CRenderScaleformDepthStencilSurface* surf = (CRenderScaleformDepthStencilSurface*)pdsb->GetSurface();
			if ( surf )
			{
				depthStencil = surf->m_depthStencilTexture;
			}
		}
	}

	CRenderScaleformRenderTargetData::UpdateData(prt, renderTarget, pdsb, depthStencil);
	return prt;
}

SFBool CRenderScaleformHAL::SetRenderTarget(SF::Render::RenderTarget* ptarget, SFBool setState)
{
	// When changing the render target while in a scene, we must flush all drawing.
	if ( HALState & HS_InScene)
		Flush();

	// Cannot set the bottom level render target if already in display.
	if ( HALState & HS_InDisplay )
		return false;

	RenderTargetEntry entry;
	CRenderScaleformRenderTargetData* prtdata = (CRenderScaleformRenderTargetData*)ptarget->GetRenderTargetData();

	GpuApi::TextureRef& renderTarget = prtdata->m_renderTarget;
	GpuApi::TextureRef& depthStencil = prtdata->m_depthStencil;

	if ( setState )
	{
		{
			GpuApi::RenderTargetSetup rtSetup;
			rtSetup.SetColorTarget( 0, renderTarget );
			rtSetup.SetDepthStencilTarget( depthStencil );
			rtSetup.SetViewport( m_viewportDesc );
			GpuApi::SetupRenderTargets( rtSetup );
		}
	}

	entry.pRenderTarget = ptarget;

	// Replace the stack entry at the bottom, or if the stack is empty, add one.
	if ( RenderTargetStack.GetSize() > 0 )
		RenderTargetStack[0] = entry;
	else
		RenderTargetStack.PushBack(entry);
	return true;
}

void CRenderScaleformHAL::PushRenderTarget(const SF::Render::RectF& frameRect, SF::Render::RenderTarget* prt, SFUInt flags, SF::Render::Color clearColor)
{
	GetEvent(SF::Render::Event_RenderTarget).Begin(__FUNCTION__);

	HALState |= HS_InRenderTarget;
	RenderTargetEntry entry;
	entry.pRenderTarget = prt;
	entry.OldViewport = VP;
	entry.OldViewRect = ViewRect;
	entry.OldMatrixState.CopyFrom(Matrices);
	Matrices->Orient2D.SetIdentity();
	Matrices->Orient3D.SetIdentity();
	Matrices->SetUserMatrix(SF::Render::Matrix2F::Identity);

	// Setup the render target/depth stencil on the device.
	if ( !prt )
	{
		WARN_RENDERER( TXT("PushRenderTarget - invalid render target.") );
		RenderTargetStack.PushBack(entry);
		return;
	}
	CRenderScaleformRenderTargetData* phd = (CRenderScaleformRenderTargetData*)prt->GetRenderTargetData();

	// Unbind the texture if it is bound (can't have a texture bound as an input and target).
	GpuApi::TextureRef clearTextures[2];
	m_textureManager->SetSamplerState(0, 2, clearTextures);

	GpuApi::TextureRef& renderTarget = phd->m_renderTarget;
	GpuApi::TextureRef& depthStencil = phd->m_depthStencil;
	
	StencilChecked = false;
	++AccumulatedStats.RTChanges;

	// Setup viewport.
	SF::Render::Rect<SFInt> viewRect = prt->GetRect(); // On the render texture, might not be the entire surface.
	const SF::Render::ImageSize& bs = prt->GetBufferSize();
	VP = SF::Render::Viewport(bs.Width, bs.Height, viewRect.x1, viewRect.y1, viewRect.Width(), viewRect.Height());    
	ViewRect.x1 = (SFInt)frameRect.x1;
	ViewRect.y1 = (SFInt)frameRect.y1;
	ViewRect.x2 = (SFInt)frameRect.x2;
	ViewRect.y2 = (SFInt)frameRect.y2;

	// Must offset the 'original' viewrect, otherwise the 3D compensation matrix will be offset.
	Matrices->ViewRectOriginal.Offset(-entry.OldViewport.Left, -entry.OldViewport.Top);
	Matrices->UVPOChanged = true;
	HALState |= HS_ViewValid;

	{
		m_hackWillSetViewportAfterwardsDuringRenderTargetSetup = true;
		updateViewport();
		m_hackWillSetViewportAfterwardsDuringRenderTargetSetup = false;
	}

	{
		GpuApi::RenderTargetSetup rtSetup;
		rtSetup.SetColorTarget( 0, renderTarget );
		rtSetup.SetDepthStencilTarget( depthStencil );
		rtSetup.SetViewport( m_viewportDesc );
		GpuApi::SetupRenderTargets( rtSetup );
	}

	// Clear, if not specifically excluded
	if ( (flags & PRT_NoClear) == 0 )
	{
		Vector clear;
		clearColor.GetRGBAFloat( clear.A );
		clear.W = 0.0f;
		GetRenderer()->ClearColorTarget( clear );
	}

	RenderTargetStack.PushBack(entry);
}

void CRenderScaleformHAL::PopRenderTarget(SFUInt flags)
{
	SF::Render::ScopedRenderEvent GPUEvent(GetEvent(SF::Render::Event_RenderTarget), 0, false);

	RenderTargetEntry& entry = RenderTargetStack.Back();
	SF::Render::RenderTarget* prt = entry.pRenderTarget;
	if ( prt && prt->GetType() == SF::Render::RBuffer_Temporary )
	{
		// Strip off the depth stencil surface/buffer from temporary targets.
		CRenderScaleformRenderTargetData* plasthd = (CRenderScaleformRenderTargetData*)prt->GetRenderTargetData();
		GpuApi::SafeRelease( plasthd->m_depthStencil );
		plasthd->pDepthStencilBuffer = 0;
	}
	Matrices->CopyFrom(&entry.OldMatrixState);
	ViewRect = entry.OldViewRect;
	VP = entry.OldViewport;

	RenderTargetStack.PopBack();
	GpuApi::TextureRef depthStencil = GpuApi::TextureRef::Null();
	//ID3D1x(DepthStencilView)* pds = 0;
	CRenderScaleformRenderTargetData* phd = 0;
	if ( RenderTargetStack.GetSize() > 0 )
	{
		RenderTargetEntry& back = RenderTargetStack.Back();
		phd = (CRenderScaleformRenderTargetData*)back.pRenderTarget->GetRenderTargetData();
		depthStencil = phd->m_depthStencil; //pds = (ID3D1x(DepthStencilView)*)phd->pDSSurface;
	}

	if ( RenderTargetStack.GetSize() == 1 )
		HALState &= ~HS_InRenderTarget;

	// Unbind the texture if it is bound (can't have a texture bound as an input and target).
	GpuApi::TextureRef clearTextures[2];
	m_textureManager->SetSamplerState(0, 2, clearTextures);

	// Restore the old render target on the device.
	if ((flags & PRT_NoSet) == 0)
	{
		++AccumulatedStats.RTChanges;

		// Reset the viewport to the last render target on the stack.
		HALState |= HS_ViewValid;
		{
			m_hackWillSetViewportAfterwardsDuringRenderTargetSetup = true;
			updateViewport();
			m_hackWillSetViewportAfterwardsDuringRenderTargetSetup = false;
		}

		const GpuApi::TextureRef& renderTarget = phd->m_renderTarget;
		GpuApi::RenderTargetSetup rtSetup;
		rtSetup.SetColorTarget( 0, renderTarget );
		rtSetup.SetDepthStencilTarget( depthStencil );
		rtSetup.SetViewport( m_viewportDesc );
		GpuApi::SetupRenderTargets( rtSetup );
	}
}

bool CRenderScaleformHAL::createDefaultRenderBuffer()
{
	SF::Render::ImageSize rtSize;

	if ( GetDefaultRenderTarget() )
	{
		SF::Render::RenderTarget* prt = GetDefaultRenderTarget();
		rtSize = prt->GetSize();
	}
	else
	{

		// FIXME:/TODO: See if this can be moved to a one time init (per device reset) in begin frame
		// Hack in guiRenderSystem.cpp: Set the viewport back. Should set the viewport back after rendering scaleform
		// in end frame or in renderrenderframe.
		// Note: SF uses SF::Ptr with GetRawRef() so the OMGetRenderTargets implicit addref
		// will be released, ref management dont in rendertargetdata/elsewhere.

		GpuApi::RenderTargetSetup rtSetup = GpuApi::GetRenderTargetSetup();
		GpuApi::TextureDesc rtDesc;
		ASSERT( rtSetup.numColorTargets > 0 );
		GpuApi::GetTextureDesc( rtSetup.colorTargets[0], rtDesc );

		rtSize = SF::Render::ImageSize(rtDesc.width, rtDesc.height);
		SF::Ptr<SF::Render::RenderTarget> ptarget = *SF_HEAP_AUTO_NEW(this) SF::Render::RenderTarget(0, SF::Render::RBuffer_Default, rtSize );
		SF::Ptr<SF::Render::DepthStencilBuffer> pdsb = 0;
		if ( rtSetup.depthTarget )
		{
			GpuApi::TextureDesc dsDesc;
			GpuApi::GetTextureDesc( rtSetup.depthTarget, dsDesc );
			SF::Render::ImageSize dsSize(dsDesc.width, dsDesc.height);
			pdsb = *SF_HEAP_AUTO_NEW(this) SF::Render::DepthStencilBuffer(0, dsSize);
		}

		CRenderScaleformRenderTargetData::UpdateData(ptarget, rtSetup.colorTargets[0], pdsb, rtSetup.depthTarget );

		if ( !SetRenderTarget(ptarget))
			return false;
	}

	return pRenderBufferManager->Initialize(m_textureManager, SF::Render::Image_R8G8B8A8, rtSize);
}

void CRenderScaleformHAL::setBatchUnitSquareVertexStream()
{
	static const Uint32 stride = sizeof(SF::Render::VertexXY16fAlpha);
	const Uint32 offset = 0;
	const GpuApi::BufferRef& vb = Cache.m_maskEraseBatchVertexBuffer;
	GpuApi::BindVertexBuffers( 0, 1, &vb, &stride, &offset );
}

void CRenderScaleformHAL::drawPrimitive(SFUInt indexCount, SFUInt meshCount)
{
	ASSERT( indexCount % 3 == 0 );
	GpuApi::DrawPrimitiveRaw( GpuApi::PRIMTYPE_TriangleList, 0, indexCount / 3 );

	SF_UNUSED(meshCount);
#ifndef RED_FINAL_BUILD
#if !defined(SF_BUILD_SHIPPING)
	AccumulatedStats.Meshes += meshCount;
	AccumulatedStats.Triangles += indexCount / 3;
	AccumulatedStats.Primitives++;
#endif
#endif
}

void CRenderScaleformHAL::drawIndexedPrimitive(SFUInt indexCount, SFUInt vertexCount, SFUInt meshCount, SF::UPInt indexPtr, SF::UPInt vertexOffset)
{
	ASSERT( indexCount % 3 == 0 );
	ASSERT( vertexOffset <= 0x7FFFFFFF ); // TBD: DrawIndexedPrimitiveRaw why take signed?
	GpuApi::DrawIndexedPrimitiveRaw( GpuApi::PRIMTYPE_TriangleList, static_cast< GpuApi::Int32 >( vertexOffset ),
									 0 /*unused*/, 0 /*unused*/, static_cast< GpuApi::Uint32 >( indexPtr ), static_cast< GpuApi::Uint32 >( indexCount / 3 ) );

	SF_UNUSED2(meshCount, vertexCount);
#ifndef RED_FINAL_BUILD
#if !defined(SF_BUILD_SHIPPING)
	AccumulatedStats.Meshes += meshCount;
	AccumulatedStats.Triangles += indexCount / 3;
	AccumulatedStats.Primitives++;
#endif
#endif
}

void CRenderScaleformHAL::drawIndexedInstanced(SFUInt indexCount, SFUInt vertexCount, SFUInt meshCount, SF::UPInt indexPtr, SF::UPInt vertexOffset )
{
	ASSERT( indexCount % 3 == 0 );
	ASSERT( vertexOffset <= 0x7FFFFFFF ); // TBD: DrawIndexedPrimitiveRaw why take signed?
	GpuApi::DrawInstancedIndexedPrimitiveRaw( GpuApi::PRIMTYPE_TriangleList, static_cast< GpuApi::Int32 >( vertexOffset ), 0 /*unused*/, 0 /*unused*/,
											  static_cast< GpuApi::Uint32 >( indexPtr ), static_cast< GpuApi::Uint32 >( indexCount / 3), meshCount );

	SF_UNUSED2(meshCount, vertexCount);
#ifndef RED_FINAL_BUILD
#if !defined(SF_BUILD_SHIPPING)
	AccumulatedStats.Meshes += meshCount;
	AccumulatedStats.Triangles += (indexCount / 3) * meshCount;
	AccumulatedStats.Primitives++;
#endif
#endif
}

SFBool CRenderScaleformHAL::shouldRenderFilters(const SF::Render::FilterPrimitive* prim) const
{
#if defined( RED_PLATFORM_DURANGO )
	// Durango can always render all filters.
	return true;
#elif defined(USE_DX10) || defined(USE_DX11)
	ASSERT( SManager.GetShaderVersion() == ScaleformGpuApi::ShaderDesc::ShaderVersion_D3D1xFL11X );
#endif
	return true;

#if 0
	// FeatureLevel 10.0+ can always render all filters.
	if (SManager.GetShaderVersion() == ScaleformGpuApi::ShaderDesc::ShaderVersion_D3D1xFL11X)
		return true;

	// If the profile doesn't support dynamic loops, check to see if there are any ColorMatrix
	// filters, which can still be rendered. If there are, allow filtering still.
	const SF::Render::FilterSet* filters = prim->GetFilters();
	Uint32 filterCount = filters->GetFilterCount();
	for ( Uint32 f = 0; f < filterCount; ++f )
	{
		const SF::Render::Filter* filter = filters->GetFilter(f);
		if ( filter->GetFilterType() == SF::Render::Filter_ColorMatrix )
			return true;
	}
	return false;
#endif
}

SF::Render::RenderEvent& CRenderScaleformHAL::GetEvent(SF::Render::EventType eventType)
{
#if 0// !defined(RED_FINAL_BUILD) && !defined(SF_BUILD_SHIPPING) && !defined(SF_OS_WINMETRO)
	static ScaleformGpuApi::RenderEvent D3D1xEvents[SF::Render::Event_Count];
	return D3D1xEvents[eventType];
#else
	// Shipping/Metro builds just return a default event, which does nothing.
	return SF::Render::HAL::GetEvent(eventType);
#endif
}

void CRenderScaleformHAL::drawScreenQuad()
{
	const GpuApi::BufferRef& vb = Cache.m_maskEraseBatchVertexBuffer;
	static const unsigned stride = sizeof(SF::Render::VertexXY16fAlpha);
	const Uint32 offset = 0;
	GpuApi::BindVertexBuffers( 0, 1, &vb, &stride, &offset );
	drawPrimitive(6,1);
}

/////////////////////////////////////////////////////////////////////////
#endif // USE_SCALEFORM
//////////////////////////////////////////////////////////////////////////
