/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/
#pragma  once

namespace GpuApi
{

	Bool RenderSettings::Validate()
	{
		Bool error = false;

		if( anisotropy < 1 ) 
		{ 
			anisotropy = 1; 
			error = true; 
		}
		else if( anisotropy > GetDeviceData().m_Caps.maxAnisotropy ) 
		{ 
			anisotropy = GetDeviceData().m_Caps.maxAnisotropy;  
			error = true; 
		}
		else if( false == ISPOW2( anisotropy ) )
		{
			// Drop to previous pow-of-to number
			auto & x = anisotropy;
			{
				x = x | (x >> 1);
				x = x | (x >> 2);
				x = x | (x >> 4);
				x = x | (x >> 8);
				x = x | (x >> 16);
				x = x - (x >> 1);
			}
			error = true;
		}

		return error;
	}

	void AddRef( const SamplerStateRef &stateRef )
	{
		GPUAPI_ASSERT( GetDeviceData().m_SamplerStates.IsInUse(stateRef) );
		GetDeviceData().m_SamplerStates.IncRefCount( stateRef );
	}

	eSamplerStatePreset GetMatchingAnisoSamplerPreset( eSamplerStatePreset samplerState )
	{
		switch( samplerState )
		{
		case SAMPSTATEPRESET_WrapLowAnisoMip :			return SAMPSTATEPRESET_WrapLinearMip;

		case SAMPSTATEPRESET_WrapAnisoNoMip :			return SAMPSTATEPRESET_WrapLinearNoMip;

		case SAMPSTATEPRESET_WrapAnisoMip :				return SAMPSTATEPRESET_WrapLinearMip;

		case SAMPSTATEPRESET_ClampAnisoNoMip :			return SAMPSTATEPRESET_ClampLinearNoMip;

		case SAMPSTATEPRESET_ClampAnisoMip :			return SAMPSTATEPRESET_ClampLinearMip;

		case SAMPSTATEPRESET_MirrorAnisoNoMip :			return SAMPSTATEPRESET_MirrorLinearNoMip;

		case SAMPSTATEPRESET_MirrorAnisoMip :			return SAMPSTATEPRESET_MirrorLinearMip;

		case SAMPSTATEPRESET_MirrorOnceAnisoNoMip :		return SAMPSTATEPRESET_MirrorOnceLinearNoMip;

		default:
			break;

		}

		return samplerState;
	}

	void GetSamplerStateDesc( const SamplerStateRef &ref, SamplerStateDesc &outDesc, eShaderType shaderStage )
	{
		GPUAPI_ASSERT( ref, TXT( "Invalid sampler state" ) );

		const SSamplerStateData &data = GetDeviceData().m_SamplerStates.Data( ref );

		outDesc = data.m_Desc;
	}
	
	SamplerStateRef GetSamplerStatePreset( eSamplerStatePreset preset )
	{
		if ( preset >= SAMPSTATEPRESET_Max )
		{
			GPUAPI_HALT( "Preset index out of range" );
			return SamplerStateRef::Null();
		}

		const SamplerStateRef &ref = GetDeviceData().m_SamplerStatePresets[preset];
		GPUAPI_ASSERT( ref, TXT( "Invalid sampler state" ) );
		return ref;
	}

	void InitSamplerStatePresets( bool assumeRefsPresent )
	{
		if ( assumeRefsPresent )
		{
			// No action needed, just do some internal checks and exit.
			// It's essential to skip any id's changes on device reset to guarantee 
			// that those won't change after device reset.
			for ( Uint32 i=0; i<SAMPSTATEPRESET_Max; ++i )
			{
				GPUAPI_ASSERT( GetDeviceData().m_SamplerStatePresets[i], TXT( "Preset not present!" ) );
			}
			return;
		}

		SDeviceData &dd = GetDeviceData();

		// Pre check

	#if GPUAPI_DEBUG
		for ( Uint32 i=0; i<SAMPSTATEPRESET_Max; ++i )
		{
			GPUAPI_ASSERT( !dd.m_SamplerStatePresets[i], TXT( "Attempted to initialize presets twice?" ) );
		}
	#endif

		// Init presets

	#define INIT_PRESET( preset, addr, minFilter, magFilter, mipFilter )	\
		dd.m_SamplerStatePresets[preset] = RequestSamplerState( Utils::BuildSamplerStateDesc( addr, minFilter, magFilter, mipFilter ) );

	#define INIT_PRESET_MIPBIAS( preset, addr, minFilter, magFilter, mipFilter, mipMapBias )	\
		dd.m_SamplerStatePresets[preset] = RequestSamplerState( Utils::BuildSamplerStateDesc( addr, minFilter, magFilter, mipFilter, COMP_None, false, mipMapBias ) );

	#define INIT_PRESET_SF( preset, addr, minFilter, magFilter, mipFilter )	\
		dd.m_SamplerStatePresets[preset] = RequestSamplerState( Utils::BuildSamplerStateDescScaleform( addr, minFilter, magFilter, mipFilter ) );

		INIT_PRESET( SAMPSTATEPRESET_WrapPointNoMip,		TEXADDR_Wrap,		TEXFILTERMIN_Point,		TEXFILTERMAG_Point,		TEXFILTERMIP_None	);
		INIT_PRESET( SAMPSTATEPRESET_WrapLinearNoMip,		TEXADDR_Wrap,		TEXFILTERMIN_Linear,	TEXFILTERMAG_Linear,	TEXFILTERMIP_None	);
		INIT_PRESET( SAMPSTATEPRESET_WrapAnisoNoMip,		TEXADDR_Wrap,		TEXFILTERMIN_Aniso,		TEXFILTERMAG_Linear,	TEXFILTERMIP_None	);
		INIT_PRESET( SAMPSTATEPRESET_WrapPointMip,			TEXADDR_Wrap,		TEXFILTERMIN_Point,		TEXFILTERMAG_Point,		TEXFILTERMIP_Point	);
		INIT_PRESET( SAMPSTATEPRESET_WrapLinearMip,			TEXADDR_Wrap,		TEXFILTERMIN_Linear,	TEXFILTERMAG_Linear,	TEXFILTERMIP_Linear	);
		INIT_PRESET( SAMPSTATEPRESET_WrapAnisoMip,			TEXADDR_Wrap,		TEXFILTERMIN_Aniso,		TEXFILTERMAG_Linear,	TEXFILTERMIP_Linear	);
		INIT_PRESET( SAMPSTATEPRESET_WrapLowAnisoMip,		TEXADDR_Wrap,		TEXFILTERMIN_AnisoLow,	TEXFILTERMAG_Linear,	TEXFILTERMIP_Linear	);
		INIT_PRESET( SAMPSTATEPRESET_WrapPointMipLinear,	TEXADDR_Wrap,		TEXFILTERMIN_Point,		TEXFILTERMAG_Point,		TEXFILTERMIP_Linear	);
		INIT_PRESET( SAMPSTATEPRESET_ClampPointNoMip,		TEXADDR_Clamp,		TEXFILTERMIN_Point,		TEXFILTERMAG_Point,		TEXFILTERMIP_None	);
		INIT_PRESET( SAMPSTATEPRESET_ClampLinearNoMip,		TEXADDR_Clamp,		TEXFILTERMIN_Linear,	TEXFILTERMAG_Linear,	TEXFILTERMIP_None	);
		INIT_PRESET( SAMPSTATEPRESET_ClampAnisoNoMip,		TEXADDR_Clamp,		TEXFILTERMIN_Aniso,		TEXFILTERMAG_Linear,	TEXFILTERMIP_None	);
		INIT_PRESET( SAMPSTATEPRESET_ClampPointMip,			TEXADDR_Clamp,		TEXFILTERMIN_Point,		TEXFILTERMAG_Point,		TEXFILTERMIP_Point	);
		INIT_PRESET( SAMPSTATEPRESET_ClampLinearMip,		TEXADDR_Clamp,		TEXFILTERMIN_Linear,	TEXFILTERMAG_Linear,	TEXFILTERMIP_Linear	);
		INIT_PRESET( SAMPSTATEPRESET_ClampAnisoMip,			TEXADDR_Clamp,		TEXFILTERMIN_Aniso,		TEXFILTERMAG_Linear,	TEXFILTERMIP_Linear	);
		INIT_PRESET( SAMPSTATEPRESET_MirrorPointNoMip,		TEXADDR_Mirror,		TEXFILTERMIN_Point,		TEXFILTERMAG_Point,		TEXFILTERMIP_None	);
		INIT_PRESET( SAMPSTATEPRESET_MirrorLinearNoMip,		TEXADDR_Mirror,		TEXFILTERMIN_Linear,	TEXFILTERMAG_Linear,	TEXFILTERMIP_None	);
		INIT_PRESET( SAMPSTATEPRESET_MirrorAnisoNoMip,		TEXADDR_Mirror,		TEXFILTERMIN_Aniso,		TEXFILTERMAG_Linear,	TEXFILTERMIP_None	);
		INIT_PRESET( SAMPSTATEPRESET_MirrorPointMip,		TEXADDR_Mirror,		TEXFILTERMIN_Point,		TEXFILTERMAG_Point,		TEXFILTERMIP_Point	);
		INIT_PRESET( SAMPSTATEPRESET_MirrorLinearMip,		TEXADDR_Mirror,		TEXFILTERMIN_Linear,	TEXFILTERMAG_Linear,	TEXFILTERMIP_Linear	);
		INIT_PRESET( SAMPSTATEPRESET_MirrorAnisoMip,		TEXADDR_Mirror,		TEXFILTERMIN_Aniso,		TEXFILTERMAG_Linear,	TEXFILTERMIP_Linear	);
		INIT_PRESET( SAMPSTATEPRESET_MirrorOncePointNoMip,	TEXADDR_MirrorOnce, TEXFILTERMIN_Point,		TEXFILTERMAG_Point,		TEXFILTERMIP_None	);
		INIT_PRESET( SAMPSTATEPRESET_MirrorOnceLinearNoMip,	TEXADDR_MirrorOnce, TEXFILTERMIN_Linear,	TEXFILTERMAG_Linear,	TEXFILTERMIP_None	);
		INIT_PRESET( SAMPSTATEPRESET_MirrorOnceAnisoNoMip,	TEXADDR_MirrorOnce, TEXFILTERMIN_Aniso,		TEXFILTERMAG_Linear,	TEXFILTERMIP_None	);
		INIT_PRESET( SAMPSTATEPRESET_MirrorOncePointMip,	TEXADDR_MirrorOnce, TEXFILTERMIN_Point,		TEXFILTERMAG_Point,		TEXFILTERMIP_Point	);
		INIT_PRESET( SAMPSTATEPRESET_MirrorOnceLinearMip,	TEXADDR_MirrorOnce, TEXFILTERMIN_Linear,	TEXFILTERMAG_Linear,	TEXFILTERMIP_Linear	);
		INIT_PRESET( SAMPSTATEPRESET_BorderPointNoMip,		TEXADDR_Border,		TEXFILTERMIN_Point,		TEXFILTERMAG_Point,		TEXFILTERMIP_None	);
		INIT_PRESET( SAMPSTATEPRESET_BorderLinearNoMip,		TEXADDR_Border,		TEXFILTERMIN_Linear,	TEXFILTERMAG_Linear,	TEXFILTERMIP_None	);

		INIT_PRESET_SF( SAMPSTATEPRESET_Scaleform_WrapPointMip,	TEXADDR_Wrap,	TEXFILTERMIN_Point,		TEXFILTERMAG_Point,		TEXFILTERMIP_Point );
		INIT_PRESET_SF( SAMPSTATEPRESET_Scaleform_WrapLinearMip, TEXADDR_Wrap,	TEXFILTERMIN_Linear,	TEXFILTERMAG_Linear,	TEXFILTERMIP_Linear );
		INIT_PRESET_SF( SAMPSTATEPRESET_Scaleform_ClampPointMip, TEXADDR_Clamp,	TEXFILTERMIN_Point,		TEXFILTERMAG_Point,		TEXFILTERMIP_Point );
		INIT_PRESET_SF( SAMPSTATEPRESET_Scaleform_ClampLinearMip, TEXADDR_Clamp, TEXFILTERMIN_Linear,	TEXFILTERMAG_Linear,	TEXFILTERMIP_Linear );

		INIT_PRESET_MIPBIAS( SAMPSTATEPRESET_SpeedTreeStandardSampler, TEXADDR_Wrap,	TEXFILTERMIN_Linear,	TEXFILTERMAG_Linear,	TEXFILTERMIP_Linear, -0.0f );
		INIT_PRESET( SAMPSTATEPRESET_SpeedTreePointSampler , TEXADDR_Wrap,	TEXFILTERMIN_Point,	TEXFILTERMAG_Point,	TEXFILTERMIP_Point );
		INIT_PRESET( SAMPSTATEPRESET_SpeedTreeLinearClampSampler , TEXADDR_Clamp,	TEXFILTERMIN_Linear,	TEXFILTERMAG_Linear,	TEXFILTERMIP_Linear );
		dd.m_SamplerStatePresets[SAMPSTATEPRESET_SpeedTreeShadowMapCompareSampler] =
			RequestSamplerState( Utils::BuildSamplerStateDesc( TEXADDR_Clamp,	TEXFILTERMIN_Linear,	TEXFILTERMAG_Linear,	TEXFILTERMIP_Point, COMP_Less , false , 0.f , false ) );

		// Overcome biasing in lod-shifted enviro probes. Avoid mipmap changing.
		dd.m_SamplerStatePresets[SAMPSTATEPRESET_ClampLinearMipNoBias] =
			RequestSamplerState( Utils::BuildSamplerStateDesc( TEXADDR_Clamp,	TEXFILTERMIN_Linear,	TEXFILTERMAG_Linear,	TEXFILTERMIP_Linear, COMP_None , false , 0.f , false ) );

		dd.m_SamplerStatePresets[SAMPSTATEPRESET_ClampPointNoMipCompareLess] =
			RequestSamplerState( Utils::BuildSamplerStateDesc( TEXADDR_Clamp, TEXFILTERMIN_Point, TEXFILTERMAG_Point, TEXFILTERMIP_Point, COMP_Less ) );
		dd.m_SamplerStatePresets[SAMPSTATEPRESET_ClampLinearNoMipCompareLess] =
			RequestSamplerState( Utils::BuildSamplerStateDesc( TEXADDR_Clamp, TEXFILTERMIN_Linear, TEXFILTERMAG_Linear, TEXFILTERMIP_Point, COMP_Less ) );
		dd.m_SamplerStatePresets[SAMPSTATEPRESET_AtlasLinearMip] =
			RequestSamplerState( Utils::BuildSamplerStateDesc( TEXADDR_Wrap, TEXFILTERMIN_Linear, TEXFILTERMAG_Linear, TEXFILTERMIP_Linear, COMP_None, true ) );

	#undef  INIT_PRESET

		// Post check

	#if GPUAPI_DEBUG
		for ( Uint32 i=0; i<SAMPSTATEPRESET_Max; ++i )
		{
			GPUAPI_ASSERT( dd.m_SamplerStatePresets[i], TXT( "Not all preset were set!" ) );
	#ifdef GPU_API_DEBUG_PATH
			GpuApi::SetSamplerStateDebugPath( dd.m_SamplerStatePresets[i], "presetSamplerState" );
	#endif
		}
	#endif
	}

	void ShutSamplerStatePresets( bool dropRefs )
	{
		if ( !dropRefs )
		{
			// No action needed.
			// It's essential to skip any id's changes on device reset to guarantee 
			// that those won't change after device reset.
			return;
		}

		SDeviceData &dd = GetDeviceData();
		for ( Uint32 i=0; i<SAMPSTATEPRESET_Max; ++i )
		{
			SafeRelease( dd.m_SamplerStatePresets[i] );
		}
	}

}