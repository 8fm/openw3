/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "renderRenderSurfaces.h"
#include "renderHelpers.h"
#include "../engine/renderSettings.h"

#ifdef RED_PLATFORM_DURANGO
# include "../engine/globalWaterUpdateParams.h"
#endif

namespace Helper
{
	const char* GetRenderTargetReadableName( ERenderTargetName name )
	{
		switch (name)
		{
		case RTN_Color:
			return "RTN_Color";
			break;
		case RTN_Color2:
			return "RTN_Color2";
			break;
		case RTN_Color3:
			return "RTN_Color3";
			break;
		case RTN_MSAAColor:
			return "RTN_MSAAColor";
			break;
		case RTN_UbersampleAccum:
			return "RTN_UbersampleAccum";
			break;
		case RTN_TemporalAAColor:
			return "RTN_TemporalAAColor";
			break;
		case RTN_TemporalAALum0:
			return "RTN_TemporalAALum0";
			break;
		case RTN_TemporalAALum1:
			return "RTN_TemporalAALum1";
			break;
		case RTN_GBuffer0:
			return "RTN_GBuffer0";
			break;
		case RTN_GBuffer0MSAA:
			return "RTN_GBuffer0MSAA";
			break;
		case RTN_GBuffer1:
			return "RTN_GBuffer1";
			break;
		case RTN_GBuffer1MSAA:
			return "RTN_GBuffer1MSAA";
			break;
		case RTN_GBuffer2:
			return "RTN_GBuffer2";
			break;
		case RTN_GBuffer2MSAA:
			return "RTN_GBuffer2MSAA";
			break;
		case RTN_GBuffer3Depth:
			return "RTN_GBuffer3Depth";
			break;
		case RTN_RLRSky:
			return "RTN_RLRSky";
			break;
		case RTN_RLRColor:
			return "RTN_RLRColor";
			break;
		case RTN_RLRDepth:
			return "RTN_RLRDepth";
			break;
		case RTN_RLRResultHistory:
			return "RTN_RLRResultHistory";
			break;
		case RTN_FinalColor:
			return "RTN_FinalColor";
			break;
		case RTN_GlobalShadowAndSSAO:
			return "RTN_GlobalShadowAndSSAO";
			break;
		case RTN_MSAACoverageMask:
			return "RTN_MSAACoverageMask";
			break;
		case RTN_LuminanceSimpleAPing:
			return "RTN_LuminanceSimpleAPing";
			break;
		case RTN_LuminanceSimpleAPong:
			return "RTN_LuminanceSimpleAPong";
			break;
		case RTN_LuminanceSimpleBPing:
			return "RTN_LuminanceSimpleBPing";
			break;
		case RTN_LuminanceSimpleBPong:
			return "RTN_LuminanceSimpleBPong";
			break;
		case RTN_LuminanceSimpleFinal:
			return "RTN_LuminanceSimpleFinal";
			break;
		case RTN_InteriorVolume:
			return "RTN_InteriorVolume";
			break;
		case RTN_PostProcessTarget1:
			return "RTN_PostProcessTarget1";
			break;
		case RTN_PostProcessTarget2:
			return "RTN_PostProcessTarget2";
			break;
		case RTN_PostProcessTempFull:
			return "RTN_PostProcessTempFull";
			break;
		case RTN_PostProcessTempHalf1:
			return "RTN_PostProcessTempHalf1";
			break;
		case RTN_PostProcessTempHalf2:
			return "RTN_PostProcessTempHalf2";
			break;
		case RTN_PostProcessTempHalf3:
			return "RTN_PostProcessTempHalf3";
			break;
		case RTN_PostProcessTempQuater1:
			return "RTN_PostProcessTempQuater1";
			break;
		case RTN_PostProcessTempQuater2:
			return "RTN_PostProcessTempQuater2";
			break;
		case RTN_PostProcessTempQuater3:
			return "RTN_PostProcessTempQuater3";
			break;
		case RTN_PostProcessTempQuater4:
			return "RTN_PostProcessTempQuater4";
			break;
		case RTN_PostProcessTempQuater5:
			return "RTN_PostProcessTempQuater5";
			break;
		case RTN_CameraInteriorFactor:
			return "RTN_CameraInteriorFactor";
			break;
		case RTN_Max:
			return "RTN_Max";
			break;
		case RTN_None:
			return "RTN_None";
			break;

#ifdef RED_PLATFORM_DURANGO
		case RTN_DURANGO_PostProcessTarget1_R10G10B10_6E4_A2_FLOAT:
			return "RTN_DURANGO_PostProcessTarget1_R10G10B10_6E4_A2_FLOAT";
			break;
		case RTN_DURANGO_PostProcessTarget2_R10G10B10_6E4_A2_FLOAT:
			return "RTN_DURANGO_PostProcessTarget2_R10G10B10_6E4_A2_FLOAT";
			break;
		case RTN_DURANGO_InteriorVolume_TempSurface:
			return "RTN_DURANGO_InteriorVolume_TempSurface";
			break;
		case RTN_DURANGO_FFT0:
			return "RTN_DURANGO_FFT0";
			break;
		case RTN_DURANGO_FFT1:
			return "RTN_DURANGO_FFT1";
			break;
		case RTN_DURANGO_RLR_Result0:
			return "RTN_DURANGO_RLR_Result0";
			break;
#endif

		default:
			RED_HALT("unknown render target name");
			return "unknown render target name";
			break;
		}
	}
}

/************************************************************************/
/* Rendering surface                                                    */
/************************************************************************/
CRenderTarget::CRenderTarget( Uint32 width, Uint32 height, ERenderTargetFormat format, Bool samplable, Uint32 msaaLevel, Int32 esramOffset, Uint32 esramSize, Uint32 numMipLevels, Uint32 extraUsageFlags )
	: m_width( Max<Uint32>( width, 1 ) )
	, m_height( Max<Uint32>( height, 1 ) )
	, m_format( format )
	, m_msaaLevel( msaaLevel )
	, m_samplable( samplable )
	, m_numMipLevels( Max<Uint32>( 1, numMipLevels ) )
	, m_extraUsageFlags( extraUsageFlags )
#ifdef RED_PLATFORM_DURANGO
	, m_esramOffset( esramOffset )
	, m_esramSize( esramSize )
#else
	// set esramData on non durango platform to zero, so that we would be able 
	// to swap rendertarget pointers on non durango platform without getting any errors.
	, m_esramOffset( 0 )
	, m_esramSize( 0 )
#endif
{
	ASSERT( RTFMT_Max != format );
}

CRenderTarget::~CRenderTarget()
{
	ASSERT( !m_texture );
}

void CRenderTarget::CreateResources( Bool dynamicScaling, Uint32 alignment )
{
	// Create texture
	ASSERT( !m_texture );
	{
		// Build desc
		GpuApi::TextureDesc desc;
		desc.type			= GpuApi::TEXTYPE_2D;
		desc.width			= m_width;
		desc.height			= m_height;
		desc.initLevels		= m_numMipLevels;
		desc.msaaLevel		= m_msaaLevel;
		if (m_format == RTFMT_DEPTH)
		{
			desc.usage		= GpuApi::TEXUSAGE_DepthStencil;
		}
		else
		{
			desc.usage		= GpuApi::TEXUSAGE_RenderTarget;
		}

		if (m_samplable)
		{
			desc.usage		|= GpuApi::TEXUSAGE_Samplable;
		}

		if ( m_esramOffset >= 0 )
		{
			desc.usage		|= GpuApi::TEXUSAGE_ESRAMResident;
		}

		// XB1 specific (right now)
		if( dynamicScaling )
		{
			// TODO: make use of our helper function here

			switch(alignment)
			{
			case 17:	desc.usage		|= GpuApi::TEXUSAGE_DynamicScalingAlignSSAO64;			break;
			case 16:	desc.usage		|= GpuApi::TEXUSAGE_DynamicScalingAlignSSAO32;			break;
			case 15:	desc.usage		|= GpuApi::TEXUSAGE_DynamicScalingAlignSSAO16;			break;
			case 14:	desc.usage		|= GpuApi::TEXUSAGE_DynamicScalingAlignSSAO8;			break;
			case 13:	desc.usage		|= GpuApi::TEXUSAGE_DynamicScalingAlignSSAO4;			break;
			case 12:	desc.usage		|= GpuApi::TEXUSAGE_DynamicScalingAlignSSAO2;			break;
			case 11:	desc.usage		|= GpuApi::TEXUSAGE_DynamicScalingAlignSSAO1;			break;
			case 4:		desc.usage		|= GpuApi::TEXUSAGE_DynamicScalingAlign4;			break;
			case 2:		desc.usage		|= GpuApi::TEXUSAGE_DynamicScalingAlign2;			break;
			case 1:		desc.usage		|= GpuApi::TEXUSAGE_DynamicScalingAlign1;			break;

			default:
				RED_ASSERT( 0 && "Unhandled Case!" );
				break;
			}
		}

		// Add extra usage flags
		RED_ASSERT( !(m_extraUsageFlags & ~(GpuApi::TEXUSAGE_GenMip | GpuApi::TEXUSAGE_Tex2DSamplablePerMipLevel)) && "Given extra usage flags not recognised as safe" );
		RED_ASSERT( !(m_numMipLevels <= 1 && (m_extraUsageFlags & GpuApi::TEXUSAGE_Tex2DSamplablePerMipLevel)) );
		RED_ASSERT( !(m_numMipLevels <= 1 && (m_extraUsageFlags & GpuApi::TEXUSAGE_GenMip)) );
		desc.usage |= m_extraUsageFlags;
		
		desc.format			= GetRenderTargetFormat( m_format );
		desc.esramOffset	= m_esramOffset;
		desc.esramSize		= m_esramSize;

		// Create resource
		m_texture = GpuApi::CreateTexture( desc, GpuApi::TEXG_System );
	}
	ASSERT( !m_texture.isNull() );

#ifdef RED_PLATFORM_WINPC
	if (m_format!=RTFMT_DEPTH)
	{
		// Might not have shaders and such loaded at this point, so use GpuApi directly. But, this is just for WINPC so we
		// know we have a proper ClearColorTarget().
		GpuApi::ClearColorTarget( m_texture, Vector::ZEROS.A );
	}
#endif
}

void CRenderTarget::ReleaseResources()
{
	// Release texture
	GpuApi::SafeRelease( m_texture );
}

void CRenderTarget::OnDeviceReset()
{
	CreateResources();
}

void CRenderTarget::OnDeviceLost()
{
	ReleaseResources();
}

/************************************************************************/
/* Group of rendering surfaces                                          */
/************************************************************************/
CRenderSurfaces::CRenderSurfaces( Uint32 width, Uint32 height, Uint32 msaaLevelX, Uint32 msaaLevelY )
	: IDynamicRenderResource()
	, m_width( width )
	, m_height( height )
	, m_msaaLevel( msaaLevelX * msaaLevelY )
	, m_msaaLevelX( msaaLevelX )
	, m_msaaLevelY( msaaLevelY )
	, m_temporalAASupported( false )
	, m_depthBuffer( NULL )
	, m_depthBufferMSAA( NULL )
	, m_highPrecisionEnabled( false )
	, m_dirtyPersistentSurfacesFlags( 0xffffffff )
{
	ASSERT ( msaaLevelX >= 1 && msaaLevelY >= 1 );

	Red::System::MemoryZero( m_renderTargets, sizeof( m_renderTargets ) );

	// Create surfaces
	CreateSurfaces();

	// Recreate surfaces
	CreateResources();
}

CRenderSurfaces::~CRenderSurfaces()
{
	// Release surfaces
	ReleaseResources();

	// Delete render target objects
	for ( Uint32 iRenderTarget = 0; iRenderTarget < RTN_Max; ++ iRenderTarget )
	{
		delete m_renderTargets[ iRenderTarget ];
	}
}

void CRenderSurfaces::CreateResources()
{
	const Uint32 msaaLevel = 0;

	// Create depth buffer
	ASSERT( !m_depthBuffer, TXT("depth buffer already exists") );
	ASSERT( !m_depthBufferMSAA, TXT("MSAA depth buffer already exists") );
	{
		// Init desc
		GpuApi::TextureDesc desc;
		desc.type		= GpuApi::TEXTYPE_2D;
		desc.width		= m_width;
		desc.height		= m_height;
		desc.initLevels = 1;
		desc.msaaLevel = 0;
		desc.format		= GpuApi::TEXFMT_D24S8;

		// the esram related things will not change anything on other platforms than Xbox One
		desc.usage		= GpuApi::TEXUSAGE_DepthStencil | GpuApi::TEXUSAGE_Samplable | GpuApi::TEXUSAGE_ESRAMResident | GpuApi::TEXUSAGE_DynamicScalingAlign4;
		desc.esramOffset = 0;
		desc.esramSize = 0;

		// Create depth buffer
		m_depthBuffer = GpuApi::CreateTexture( desc, GpuApi::TEXG_System );
		GpuApi::SetTextureDebugPath(m_depthBuffer, "depthBuffer");

		if ( IsMSAASupported() )
		{
			desc.msaaLevel = m_msaaLevel;
			m_depthBufferMSAA = GpuApi::CreateTexture( desc, GpuApi::TEXG_System );
			GpuApi::SetTextureDebugPath(m_depthBufferMSAA, "depthBufferMSAA");
		}
	}
	ASSERT( m_depthBuffer, TXT("depth buffer wasn't created") );
	ASSERT( !IsMSAASupported() || m_depthBufferMSAA, TXT("MSAA depth buffer wasn't created") );

	// Reset all render targets
	for ( Uint32 iRenderTarget = 0; iRenderTarget < RTN_Max; ++ iRenderTarget )
	{
		Bool dynamicScaling = true;
		Uint32 alignment = 4;

#ifdef RED_PLATFORM_DURANGO
		switch(iRenderTarget)
		{
		case RTN_DURANGO_FFT0:
		case RTN_DURANGO_FFT1:
		case RTN_LuminanceSimpleAPing:
		case RTN_LuminanceSimpleAPong:
		case RTN_LuminanceSimpleBPing:
		case RTN_LuminanceSimpleBPong:
		case RTN_LuminanceSimpleFinal:
		case RTN_CameraInteriorFactor:
		case RTN_RLRSky:
			dynamicScaling = false;
			break;

		case RTN_InteriorVolume:
		case RTN_PostProcessTempHalf1:
		case RTN_PostProcessTempHalf2:
		case RTN_PostProcessTempHalf3:
			alignment = 2;
			break;

		case RTN_PostProcessTempQuater1:
		case RTN_PostProcessTempQuater2:
		case RTN_PostProcessTempQuater3:
		case RTN_PostProcessTempQuater4:
		case RTN_PostProcessTempQuater5:
			alignment = 1;
			break;

		case RTN_RLRColor:
		case RTN_RLRDepth:
		case RTN_RLRResultHistory:
			#ifdef REALTIME_REFLECTIONS_FIXED_SIZE
			# if REALTIME_REFLECTIONS_FIXED_SIZE
				dynamicScaling = false;
			# else
				alignment = 1;
			# endif
			#else
			# error Expected definition
			#endif
			break;
		}
#endif
		if ( m_renderTargets[ iRenderTarget ] )
		{
			m_renderTargets[ iRenderTarget ]->CreateResources( dynamicScaling, alignment );
			GpuApi::SetTextureDebugPath( m_renderTargets[ iRenderTarget ]->GetTexture(), Helper::GetRenderTargetReadableName((ERenderTargetName)iRenderTarget) );
		}
	}
}

void CRenderSurfaces::ReleaseResources()
{
	// Release depth buffer
	GpuApi::SafeRelease( m_depthBuffer );
	GpuApi::SafeRelease( m_depthBufferMSAA );

	// Release all render targets
	for ( Uint32 iRenderTarget = 0; iRenderTarget < RTN_Max; ++ iRenderTarget )
	{
		if ( m_renderTargets[ iRenderTarget ] )
		{
			m_renderTargets[ iRenderTarget ]->ReleaseResources();
		}
	}
}

void CRenderSurfaces::SetPersistentSurfaceDirty( ePersistentSurface surface, Bool newValue )
{
	m_dirtyPersistentSurfacesFlags = newValue ? (m_dirtyPersistentSurfacesFlags | FLAG(surface)) : (m_dirtyPersistentSurfacesFlags & ~FLAG(surface));
	RED_ASSERT( IsPersistentSurfaceDirty( surface ) == newValue );
}

void CRenderSurfaces::SetAllPersistentSurfacesDirty( Bool newValue )
{
	m_dirtyPersistentSurfacesFlags = newValue ? 0xffffffff : 0;
}

Bool CRenderSurfaces::IsPersistentSurfaceDirty( ePersistentSurface surface ) const
{
	return 0 != (m_dirtyPersistentSurfacesFlags & FLAG(surface));
}

CRenderTarget* CRenderSurfaces::GetRenderTarget( ERenderTargetName renderTargetName ) const
{
	ASSERT( renderTargetName < RTN_Max );
	ASSERT ( NULL != m_renderTargets[ renderTargetName ] );
	return m_renderTargets[ renderTargetName ];
}

GpuApi::TextureRef CRenderSurfaces::GetRenderTargetTex( ERenderTargetName renderTargetName ) const
{
	const CRenderTarget* renderTarget = GetRenderTarget( renderTargetName );
	return renderTarget ? renderTarget->GetTexture() : GpuApi::TextureRef::Null();
}

GpuApi::TextureRef CRenderSurfaces::GetDepthBufferTex() const
{
	return m_depthBuffer;
}

GpuApi::TextureRef CRenderSurfaces::GetDepthBufferTexMSAA() const
{
	ASSERT ( IsMSAASupported() );
	ASSERT ( !m_depthBufferMSAA.isNull() );
	return m_depthBufferMSAA;
}

Int32 AlignUp( Int32 size, Int32 alignment )
{
	Int32 newSize = (size + (alignment - 1)) & ~( alignment - 1 );
	ASSERT( newSize >= size, TXT("Wrong calculation on alignment") );
	return newSize;
}

void CRenderSurfaces::CreateSurfaces()
{
#ifdef RED_PLATFORM_DURANGO
	// On xbox, need to drop color precision a bit so things fit into ESRAM. 16F colorFormat can still fit at 720p, but when we bump up to
	// 900p we run out of ESRAM space.
	ERenderTargetFormat colorFormat = RTFMT_R11G11B10F;
	ERenderTargetFormat colorFormatWithAlpha = RTFMT_R10G10B10_6E4_A2_FLOAT;
	ERenderTargetFormat scaledColorFormat = RTFMT_R11G11B10F;
	ERenderTargetFormat postColorFormat = RTFMT_R11G11B10F;
	ERenderTargetFormat interiorVolumeFormat = RTFMT_G16R16F;
#else
	// TODO : We could use the same formats as above even on other platforms. We don't have the space issues as with Xbox's ESRAM, but using
	// the smaller color format should give a performance boost anyways. For now, keeping it like this, might be useful for doing side-by-side
	// comparisons to check for any artifacts from the lower precision.
	//
	// But, we'll keep the postfx format with 11-11-10, so that we don't have the alpha channel on any platform -- so people don't develop on
	// PC, and then someone notices later that something doesn't work on xbox.
	ERenderTargetFormat colorFormat				= m_highPrecisionEnabled ? RTFMT_A32B32G32R32F : RTFMT_A16B16G16R16F;
	ERenderTargetFormat colorFormatWithAlpha	= colorFormat;
	ERenderTargetFormat scaledColorFormat =	m_highPrecisionEnabled ? RTFMT_A32B32G32R32F : RTFMT_A16B16G16R16F;
	ERenderTargetFormat postColorFormat =	m_highPrecisionEnabled ? RTFMT_A32B32G32R32F : RTFMT_R11G11B10F;
	ERenderTargetFormat interiorVolumeFormat = RTFMT_G16R16F;
#endif

	const Uint32 alignment = 64 * 1024;

	// Set all persistent surfaces dirty
	SetAllPersistentSurfacesDirty( true );

	// The depth buffer is at the beginning of the buffer so we have to start everything else from after that
	GpuApi::TextureDesc depthDesc;
	depthDesc.type		= GpuApi::TEXTYPE_2D;
	depthDesc.width		= m_width;
	depthDesc.height		= m_height;
	depthDesc.initLevels = 1;
	depthDesc.msaaLevel = 0;
	depthDesc.format		= GpuApi::TEXFMT_D24S8;

	// the esram related things will not change anything on other platforms than Xbox One
	depthDesc.usage		= GpuApi::TEXUSAGE_DepthStencil | GpuApi::TEXUSAGE_Samplable | GpuApi::TEXUSAGE_ESRAMResident | GpuApi::TEXUSAGE_DynamicScalingAlign4;
	depthDesc.esramOffset = 0;

	Uint32 depthSize = CalcTextureSize( depthDesc );
	depthDesc.esramSize = depthSize;

	Int32 esramOffset = AlignUp( depthSize, alignment );

	GpuApi::TextureDesc desc;
	desc.type		= GpuApi::TEXTYPE_2D;
	desc.width		= m_width;
	desc.height		= m_height;
	desc.initLevels = 1;
	desc.msaaLevel = 0;
	desc.format		= GpuApi::TEXFMT_R8G8B8A8;
	desc.usage		= GpuApi::TEXUSAGE_RenderTarget | GpuApi::TEXUSAGE_Samplable | GpuApi::TEXUSAGE_ESRAMResident | GpuApi::TEXUSAGE_DynamicScalingAlign4;

	Uint32 gbSize = AlignUp( CalcTextureSize( desc ), alignment );
	Uint32 esramSize = gbSize;

	// GB1 is first for convenience, so it can stay in ESRAM when we do post-process.
	m_renderTargets[ RTN_GBuffer1 ]					= new CRenderTarget( m_width,       m_height,			RTFMT_A8R8G8B8,				true, 0,	esramOffset, esramSize );
	esramOffset = AlignUp( esramOffset + esramSize, alignment );

	const Uint32 esramOffsetGBuffer0 = esramOffset;
	m_renderTargets[ RTN_GBuffer0 ]					= new CRenderTarget( m_width,       m_height,			RTFMT_A8R8G8B8,				true, 0,	esramOffset, esramSize );
	esramOffset = AlignUp( esramOffset + esramSize, alignment );

	m_renderTargets[ RTN_GBuffer2 ]					= new CRenderTarget( m_width,       m_height,			RTFMT_A8R8G8B8,				true, 0,	esramOffset, esramSize );
	esramOffset = AlignUp( esramOffset + esramSize, alignment );

#ifdef RED_PLATFORM_DURANGO
	{
		// Interior Volume Temp Surface
		{
			GpuApi::TextureDesc ivDesc;
			ivDesc.type			= GpuApi::TEXTYPE_2D;
			ivDesc.initLevels	= 1;
			ivDesc.msaaLevel	= 0;
			ivDesc.usage		= GpuApi::TEXUSAGE_RenderTarget | GpuApi::TEXUSAGE_Samplable | GpuApi::TEXUSAGE_ESRAMResident | GpuApi::TEXUSAGE_DynamicScalingAlign4;
			ivDesc.width		= m_width;		/// Full width/height instead of divided by WEATHER_VOLUMES_SIZE_DIV to shut up durango warnings
			ivDesc.height		= m_height;		//
			ivDesc.format		= GetRenderTargetFormat( interiorVolumeFormat );
			const Uint32 interiorVolumeEsramOffset = esramOffsetGBuffer0;
			const Uint32 interiorVolumeEsramSize = AlignUp( CalcTextureSize( ivDesc ), alignment );
			RED_ASSERT( interiorVolumeEsramSize <= esramSize );

			m_renderTargets[ RTN_DURANGO_InteriorVolume_TempSurface ]	= new CRenderTarget( ivDesc.width,   ivDesc.height,		interiorVolumeFormat,			true, 0, interiorVolumeEsramOffset, interiorVolumeEsramSize );
		}

		// RLR Result 0
		{
			GpuApi::TextureDesc ivDesc;
			ivDesc.type			= GpuApi::TEXTYPE_2D;
			ivDesc.initLevels	= 1;
			ivDesc.msaaLevel	= 0;
			ivDesc.usage		= GpuApi::TEXUSAGE_RenderTarget | GpuApi::TEXUSAGE_Samplable | GpuApi::TEXUSAGE_ESRAMResident | GpuApi::TEXUSAGE_DynamicScalingAlign4;
			ivDesc.width		= m_width;		/// Full width/height instead of divided by WEATHER_VOLUMES_SIZE_DIV to shut up durango warnings
			ivDesc.height		= m_height;		//
			ivDesc.format		= GetRenderTargetFormat( colorFormatWithAlpha );
			const Uint32 rlrResult0EsramOffset = esramOffsetGBuffer0;
			const Uint32 rlrResult0EsramSize = AlignUp( CalcTextureSize( ivDesc ), alignment );
			RED_ASSERT( rlrResult0EsramSize <= esramSize );

			m_renderTargets[ RTN_DURANGO_RLR_Result0 ]	= new CRenderTarget( ivDesc.width,   ivDesc.height,		colorFormatWithAlpha,	true, 0, rlrResult0EsramOffset, rlrResult0EsramSize );
		}
	}
#endif

#if 1
	m_renderTargets[ RTN_GBuffer3Depth ]			= new CRenderTarget( m_width,       m_height,			RTFMT_R32F,					true, 0 );
#else
	desc.format = GpuApi::TEXFMT_Float_R32;
	esramSize = AlignUp( CalcTextureSize( desc ), alignment );
	m_renderTargets[ RTN_GBuffer3Depth ]			= new CRenderTarget( m_width,       m_height,			RTFMT_R32F,					true, 0,	esramOffset, esramSize  );
	esramOffset = AlignUp( esramOffset + esramSize, alignment );
#endif

	if ( IsMSAASupported() )
	{
		m_renderTargets[ RTN_GBuffer0MSAA ]				= new CRenderTarget( m_width,       m_height,			RTFMT_A8R8G8B8,			true, m_msaaLevel );
		m_renderTargets[ RTN_GBuffer1MSAA ]				= new CRenderTarget( m_width,       m_height,			RTFMT_A8R8G8B8,			true, m_msaaLevel );
		m_renderTargets[ RTN_GBuffer2MSAA ]				= new CRenderTarget( m_width,       m_height,			RTFMT_A8R8G8B8,			true, m_msaaLevel );
		m_renderTargets[ RTN_MSAACoverageMask ]			= new CRenderTarget( m_width,       m_height,			RTFMT_A8R8G8B8,			true, 0           );
		m_renderTargets[ RTN_MSAAColor ]				= new CRenderTarget( m_width,       m_height,			colorFormat,			true, m_msaaLevel );
		// ace_msaa_todo : what about checking max texture size limit? (for RTN_MSAAColor)
	}

	// Can't put this in ESRAM, it is used in the final present
	m_renderTargets[ RTN_FinalColor ]				= new CRenderTarget( m_width,       m_height,			RTFMT_A8R8G8B8,				true, 0 );
	m_renderTargets[ RTN_GlobalShadowAndSSAO ]		= new CRenderTarget( m_width,       m_height,			RTFMT_A8R8G8B8,				true, 0 );

	{
		Uint32 rlrWidth = m_width/REALTIME_REFLECTIONS_SIZE_DIV;
		Uint32 rlrHeight = m_height/REALTIME_REFLECTIONS_SIZE_DIV;
		#ifdef REALTIME_REFLECTIONS_FIXED_SIZE
		{
		# if REALTIME_REFLECTIONS_FIXED_SIZE
			rlrWidth = REALTIME_REFLECTIONS_SIZE_WIDTH;
			rlrHeight = REALTIME_REFLECTIONS_SIZE_HEIGHT;
		# endif
		}
		#else
		# error Expected definition
		#endif
		
		// 'Min' is sufficient in here
		const Uint32 rlrHistoryMips = Min<Uint32>( MLog2( rlrWidth ), MLog2( rlrHeight ) ) + 1;
		const Uint32 rlrHistoryExtraUsage = GpuApi::TEXUSAGE_Tex2DSamplablePerMipLevel | GpuApi::TEXUSAGE_GenMip;

		m_renderTargets[ RTN_RLRColor ]					= new CRenderTarget( rlrWidth,								rlrHeight,					colorFormat,				true, 0 );
		m_renderTargets[ RTN_RLRDepth ]					= new CRenderTarget( rlrWidth,								rlrHeight,					RTFMT_R32F,					true, 0 );
		m_renderTargets[ RTN_RLRResultHistory ]			= new CRenderTarget( rlrWidth,								rlrHeight,					colorFormatWithAlpha,		true, 0, -1, 0, rlrHistoryMips, rlrHistoryExtraUsage );
		m_renderTargets[ RTN_RLRSky ]					= new CRenderTarget( 64,									64,							colorFormat,				true, 0 );
	}

#if 1
	desc.format = GetRenderTargetFormat( colorFormat );
	esramSize = AlignUp( CalcTextureSize( desc ), alignment );
	m_renderTargets[ RTN_Color ]					= new CRenderTarget( m_width,       m_height,			colorFormat,				true, 0,	esramOffset, esramSize );
	esramOffset = AlignUp( esramOffset + esramSize, alignment );
#else
	//esramSize = AlignUp(m_width * m_height * 8, 64 * 1024 ); // float16 texture
	m_renderTargets[ RTN_Color ]					= new CRenderTarget( m_width,       m_height,			colorFormat,				true, 0 );
	//esramOffset = AlignUp( esramOffset + esramSize, 64 * 1024 );
#endif

	m_renderTargets[ RTN_Color2 ]					= new CRenderTarget( m_width,       m_height,			colorFormat,				true, 0 );
	m_renderTargets[ RTN_Color3 ]					= new CRenderTarget( m_width,       m_height,			colorFormat,				true, 0 );
#ifndef NO_EDITOR
	m_renderTargets[ RTN_UbersampleAccum ]			= new CRenderTarget( m_width,       m_height,			colorFormat,				true, 0 );
#endif

	m_temporalAASupported = Config::cvEnableTemporalAA.Get();
	if ( m_temporalAASupported )
	{
		const ERenderTargetFormat tempAAColorFormat	= RTFMT_R11G11B10F; // colorFormat
		const ERenderTargetFormat tempAALumFormat	= RTFMT_R16F;

		m_renderTargets[ RTN_TemporalAAColor ]			= new CRenderTarget( m_width,       m_height,			tempAAColorFormat,			true, 0 );
		m_renderTargets[ RTN_TemporalAALum0 ]			= new CRenderTarget( m_width,       m_height,			tempAALumFormat,			true, 0 );
		m_renderTargets[ RTN_TemporalAALum1 ]			= new CRenderTarget( m_width,       m_height,			tempAALumFormat,			true, 0 );
	}

	// Luminance* are tiny, and expected to be preserved across frames. So no ESRAM there.
	m_renderTargets[ RTN_LuminanceSimpleAPing ]		= new CRenderTarget( 1,				1,					RTFMT_R32F,					true, 0 );
	m_renderTargets[ RTN_LuminanceSimpleAPong ]		= new CRenderTarget( 1,				1,					RTFMT_R32F,					true, 0 );
	m_renderTargets[ RTN_LuminanceSimpleBPing ]		= new CRenderTarget( 1,				1,					RTFMT_R32F,					true, 0 );
	m_renderTargets[ RTN_LuminanceSimpleBPong ]		= new CRenderTarget( 1,				1,					RTFMT_R32F,					true, 0 );
	m_renderTargets[ RTN_LuminanceSimpleFinal ]		= new CRenderTarget( 1,				1,					RTFMT_R32F,					true, 0 );

	m_renderTargets[ RTN_InteriorVolume ]			= new CRenderTarget( m_width / WEATHER_VOLUMES_SIZE_DIV,   m_height / WEATHER_VOLUMES_SIZE_DIV,		interiorVolumeFormat,			true, 0 );

	// Post Process targets
	esramOffset = 0;
	// During postprocess, depth and GBuffer1 are first in ESRAM.
	// We need to keep GBuffer1 for now, because we don't have a way yet to move it out of ESRAM, and a couple of the post processes use it.
	esramOffset = AlignUp( esramOffset + depthSize, alignment );
	esramOffset = AlignUp( esramOffset + gbSize, alignment );

	Uint32 ppSize, ppHalfSize, ppQuaterSize;
	{
		GpuApi::TextureDesc desc;
		desc.type		= GpuApi::TEXTYPE_2D;
		desc.initLevels = 1;
		desc.msaaLevel = 0;
		desc.usage		= GpuApi::TEXUSAGE_RenderTarget | GpuApi::TEXUSAGE_Samplable | GpuApi::TEXUSAGE_ESRAMResident | GpuApi::TEXUSAGE_DynamicScalingAlign4;

		desc.width		= m_width;
		desc.height		= m_height;
		desc.format		= GetRenderTargetFormat( postColorFormat );
		ppSize = GpuApi::CalcTextureSize( desc );

		desc.width		= m_width / 2;
		desc.height		= m_height / 2;
		desc.format		= GetRenderTargetFormat( postColorFormat );
		desc.usage		= GpuApi::TEXUSAGE_RenderTarget | GpuApi::TEXUSAGE_Samplable | GpuApi::TEXUSAGE_ESRAMResident | GpuApi::TEXUSAGE_DynamicScalingAlign2;
		ppHalfSize = GpuApi::CalcTextureSize( desc );

		desc.width		= m_width / 4;
		desc.height		= m_height / 4;
		desc.format		= GetRenderTargetFormat( postColorFormat );
		desc.usage		= GpuApi::TEXUSAGE_RenderTarget | GpuApi::TEXUSAGE_Samplable | GpuApi::TEXUSAGE_ESRAMResident | GpuApi::TEXUSAGE_DynamicScalingAlign1;
		ppQuaterSize = GpuApi::CalcTextureSize( desc );
	}


	Int32 esramPP1		= esramOffset;
	Int32 esramPP2		= AlignUp( esramPP1 + ppSize, alignment );
	Int32 esramPPTemp	= AlignUp( esramPP2 + ppSize, alignment );
	// Half* share same space as Full, so don't change offset here.
	Int32 esramPPHalf1	= esramPPTemp;
	Int32 esramPPHalf2	= AlignUp( esramPPHalf1 + ppHalfSize, alignment );
	Int32 esramPPHalf3	= AlignUp( esramPPTemp + ppSize, alignment ); // We need one half that doesn't share memory with other buffer

	Int32 esramPPQuater1 =  AlignUp( esramPPHalf3 + ppHalfSize, alignment );
	Int32 esramPPQuater2 =  AlignUp( esramPPQuater1 + ppQuaterSize, alignment );
	Int32 esramPPQuater3 =  AlignUp( esramPPQuater2 + ppQuaterSize, alignment );
	Int32 esramPPQuater4 =  AlignUp( esramPPQuater3 + ppQuaterSize, alignment ); // We need one half that doesn't share memory with other buffer
	Int32 esramPPQuater5 =  AlignUp( esramPPQuater4 + ppQuaterSize, alignment ); // We need one half that doesn't share memory with other buffer

	m_renderTargets[ RTN_PostProcessTarget1 ]		= new CRenderTarget( m_width,		m_height,			postColorFormat,			true, 0, esramPP1,		ppSize );
	m_renderTargets[ RTN_PostProcessTarget2 ]		= new CRenderTarget( m_width,		m_height,			postColorFormat,			true, 0, esramPP2,		ppSize );
	m_renderTargets[ RTN_PostProcessTempFull ]		= new CRenderTarget( m_width,		m_height,			postColorFormat,			true, 0, esramPPTemp,	ppSize );
	
#ifdef RED_PLATFORM_DURANGO
	RED_ASSERT( RTFMT_R11G11B10F == postColorFormat );
	m_renderTargets[ RTN_DURANGO_PostProcessTarget1_R10G10B10_6E4_A2_FLOAT ]		= new CRenderTarget( m_width,		m_height,			RTFMT_R10G10B10_6E4_A2_FLOAT,			true, 0, esramPP1,		ppSize );
	m_renderTargets[ RTN_DURANGO_PostProcessTarget2_R10G10B10_6E4_A2_FLOAT ]		= new CRenderTarget( m_width,		m_height,			RTFMT_R10G10B10_6E4_A2_FLOAT,			true, 0, esramPP2,		ppSize );
	{
		GpuApi::TextureDesc texDescTex;
		texDescTex.type = GpuApi::TEXTYPE_2D;
		texDescTex.format = GpuApi::TEXFMT_Float_R16G16;
		texDescTex.initLevels = 1;
		texDescTex.usage = GpuApi::TEXUSAGE_Samplable | GpuApi::TEXUSAGE_RenderTarget | GpuApi::TEXUSAGE_ESRAMResident;
		texDescTex.width = WATER_RESOLUTION;
		texDescTex.height = WATER_RESOLUTION;

		const Uint32 fftSize = GpuApi::CalcTextureSize( texDescTex );
		RED_ASSERT( fftSize <= ppSize );

		m_renderTargets[ RTN_DURANGO_FFT0 ]		= new CRenderTarget( WATER_RESOLUTION,		WATER_RESOLUTION,			RTFMT_G16R16F,			true, 0, esramPP1,		fftSize );
		m_renderTargets[ RTN_DURANGO_FFT1 ]		= new CRenderTarget( WATER_RESOLUTION,		WATER_RESOLUTION,			RTFMT_G16R16F,			true, 0, esramPP2,		fftSize );
	}
#endif

	m_renderTargets[ RTN_PostProcessTempHalf1 ]		= new CRenderTarget( m_width / 2,	m_height / 2,		postColorFormat,			true, 0, esramPPHalf1,	ppHalfSize );
	m_renderTargets[ RTN_PostProcessTempHalf2 ]		= new CRenderTarget( m_width / 2,	m_height / 2,		postColorFormat,			true, 0, esramPPHalf2,	ppHalfSize );
	m_renderTargets[ RTN_PostProcessTempHalf3 ]		= new CRenderTarget( m_width / 2,	m_height / 2,		postColorFormat,			true, 0, esramPPHalf3,	ppHalfSize );

	m_renderTargets[ RTN_PostProcessTempQuater1 ]	= new CRenderTarget( m_width / 4,	m_height / 4,		postColorFormat,			true, 0 ); // , esramPPQuater1,	ppQuaterSize );
	m_renderTargets[ RTN_PostProcessTempQuater2 ]	= new CRenderTarget( m_width / 4,	m_height / 4,		postColorFormat,			true, 0 ); // , esramPPQuater2,	ppQuaterSize );
	m_renderTargets[ RTN_PostProcessTempQuater3 ]	= new CRenderTarget( m_width / 4,	m_height / 4,		postColorFormat,			true, 0 ); // , esramPPQuater3,	ppQuaterSize );
	m_renderTargets[ RTN_PostProcessTempQuater4 ]	= new CRenderTarget( m_width / 4,	m_height / 4,		postColorFormat,			true, 0 ); // , esramPPQuater4,	ppQuaterSize );
	m_renderTargets[ RTN_PostProcessTempQuater5 ]	= new CRenderTarget( m_width / 4,	m_height / 4,		postColorFormat,			true, 0 ); // , esramPPQuater5,	ppQuaterSize );

	m_renderTargets[ RTN_CameraInteriorFactor ]	= new CRenderTarget( 1,	1,		RTFMT_A8R8G8B8,			true, 0 );
}

GpuApi::TextureRef CRenderSurfaces::GetLocalReflectionsMaskTex() const
{
	return GetRenderTargetTex( RTN_FinalColor );
}

Vector CRenderSurfaces::GetGBufferDefaultClearColor( Uint32 gbufferIndex )
{
	switch ( gbufferIndex )
	{
	case 0:  return Vector( 0.0f, 0.f, 0.0f, 1.0f );
	case 1:  return Vector::ZEROS;
	case 2:  return Vector( 0.0f, 0.f, 0.0f, 0.f/*GBUFF_MATERIAL_MASK_ENCODED_DEFAULT*/ );
	default: ASSERT( !"invalid" ); return Vector::ZEROS;
	}
}

CName CRenderSurfaces::GetCategory() const
{
	return CNAME( RenderSurfaces );
}

Uint32 CRenderSurfaces::GetUsedVideoMemory() const
{
	Uint32 size = 0;
	// Get depth buffer size
	size += GpuApi::CalcTextureSize( m_depthBuffer );
	size += GpuApi::CalcTextureSize( m_depthBufferMSAA );

	// Add render targets
	for ( Uint32 i=0; i<RTN_Max; i++ )
	{
		if ( m_renderTargets[i] )
		{
			size += GpuApi::CalcTextureSize( m_renderTargets[i]->GetTexture() );
		}
	}

	// Return size
	return size;
}

Bool CRenderSurfaces::IsHighPrecisionEnabled() const
{
	return m_highPrecisionEnabled;
}

Bool CRenderSurfaces::SetHighPrecision( Bool enableHighPrecision )
{
	if ( enableHighPrecision == m_highPrecisionEnabled )
	{
		return true;
	}

#if defined(RED_PLATFORM_WINPC) && !defined(NO_EDITOR)
	{
		// Change high precision setting
		m_highPrecisionEnabled = enableHighPrecision;

		for ( Uint32 iRenderTarget = 0; iRenderTarget < RTN_Max; ++ iRenderTarget )
		{
			CRenderTarget *&rt = m_renderTargets[ iRenderTarget ];
			if ( nullptr != rt )
			{
				rt->ReleaseResources();
				delete rt;
				rt = nullptr;
			}
		}

		// Create rendertargets
		CreateSurfaces();

		// Create rendertarget textures
		for ( Uint32 iRenderTarget = 0; iRenderTarget < RTN_Max; ++ iRenderTarget )
		{
			CRenderTarget *&rt = m_renderTargets[ iRenderTarget ];
			if ( rt )
			{
				RED_ASSERT( !rt->GetTexture() );
				rt->CreateResources();
				RED_ASSERT( rt->GetTexture() );
			}
		}

		//
		return true;
	}
#else
	return false;
#endif
}

Bool CRenderSurfaces::IsRenderTargetsSwappable( ERenderTargetName RT1, ERenderTargetName RT2 ) const
{
	return GpuApi::GetTextureDesc( GetRenderTargetTex( RT1 ) ) == GpuApi::GetTextureDesc( GetRenderTargetTex( RT2 ) );
}

void CRenderSurfaces::SwapRenderTargetPointers( ERenderTargetName RT1, ERenderTargetName RT2 )
{
	ASSERT( IsRenderTargetsSwappable( RT1, RT2 ) );

	CRenderTarget* tempptr = m_renderTargets[ RT1 ];
	m_renderTargets[ RT1 ] = m_renderTargets[ RT2 ];
	m_renderTargets[ RT2 ] = tempptr;
}

void CRenderSurfaces::OnDeviceLost()
{
	ReleaseResources();
}

void CRenderSurfaces::OnDeviceReset()
{
	CreateResources();
}

Uint8* GrabRenderSurfacesThumbnail( const GpuApi::Rect* srcRect, Uint32& size )
{
	// Save to PNG :)
	Uint8* buffer = nullptr;
	GpuApi::TextureRef colorBuffer = GetRenderer()->GetSurfaces()->GetRenderTargetTex( RTN_FinalColor );
	if ( !GpuApi::SaveTextureToMemory( colorBuffer, GpuApi::SAVE_FORMAT_PNG, srcRect, (void**)(&(buffer)), size ) )
	{
		size = 0;
		return nullptr;
	}

	// Done
	return buffer;
}
