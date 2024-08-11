/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#include "gpuApi.h"

extern float GRenderSettingsMipBias;
	
namespace GpuApi
{
	namespace Utils
	{
		SamplerStateDesc BuildSamplerStateDesc( eTextureAddress address, eTextureFilterMin minFilter, eTextureFilterMag magFilter, eTextureFilterMip mipFilter, eComparison comp = COMP_None, bool pointZFilter = false, Float mipMapBiasAdd = 0.f, Bool useSettingsBias = true )
		{
			SamplerStateDesc desc;
			desc.addressU = address;
			desc.addressV = address;
			desc.addressW = address;			
			desc.filterMin = minFilter;			
			desc.filterMag = magFilter;			
			desc.filterMip = mipFilter;
			desc.mipLODBias = ( useSettingsBias ? GpuApi::GetDeviceData().m_RenderSettings.mipMapBias + GRenderSettingsMipBias : 0.f ) + mipMapBiasAdd;
			desc.pointZFilter = pointZFilter;
			desc.comparisonFunction = comp;

			return desc;
		}

		SamplerStateDesc BuildSamplerStateDescScaleform( eTextureAddress address, eTextureFilterMin minFilter, eTextureFilterMag magFilter, eTextureFilterMip mipFilter, eComparison comp = COMP_None, bool pointZFilter = false )
		{
			SamplerStateDesc desc;
			desc.addressU = address;
			desc.addressV = address;
			desc.addressW = TEXADDR_Clamp;			// Always clamp
			desc.filterMin = minFilter;			
			desc.filterMag = magFilter;			
			desc.filterMip = mipFilter;
			desc.mipLODBias = -0.75f;				// SF has its own fixed bias
			desc.pointZFilter = pointZFilter;
			desc.comparisonFunction = comp;
			return desc;
		}
	}

	// ----------------------------------------------------------------------

	Int32 Release( const SamplerStateRef &stateRef )
	{	
		GPUAPI_ASSERT( GetDeviceData().m_SamplerStates.IsInUse(stateRef) );

		// Release
		SDeviceData &dd = GetDeviceData();
		GPUAPI_ASSERT( dd.m_SamplerStates.GetRefCount(stateRef) >= 1 );
		Int32 refCount = dd.m_SamplerStates.DecRefCount( stateRef );

		// Optionally destroy
		if ( 0 == refCount )
		{
			dd.m_SamplerStates.Destroy( stateRef );
		}
		return refCount;
	}

	Bool IsDescSupported( const SamplerStateDesc& )
	{
		// All sampler states are supported
		return true;
	}


	template <typename In, typename Out>
	Out binary_rep(In _in)
	{
		union {
			In in;
			Out out;
		} t;
		t.in = _in;
		return t.out;
	};


	template <Uint32 IntBits, Uint32 FracBits>
	Uint32 FloatToSignedFixedPoint(Float f)
	{
		Uint32 IntBitsWithoutSign = IntBits - 1;

		Uint32 max_frac = (1 << FracBits) - 1;
		Uint32 max_int = (1 << IntBitsWithoutSign) - 1;
		Uint32 max_val = (max_int << FracBits) | max_frac;

		Uint32 FloatSignBit = 1 << ((sizeof(Float) * 8) - 1);
		Bool negative = (binary_rep<Float, Uint32>(f) & FloatSignBit) == FloatSignBit;

		Uint32 i = fabs(f) * (1 << FracBits);
		i = (i < max_val) ? i : max_val;

		if (negative)
			i ^= ((1 << (IntBits + FracBits)) - 1);

		return i;
	}


	template <Uint32 IntBits, Uint32 FracBits>
	Uint32 FloatToUnsignedFixedPoint(Float f)
	{
		if (f < 0.0f)
			return 0;

		Uint32 max_frac = (1 << FracBits) - 1;
		Uint32 max_int = (1 << IntBits) - 1;
		Uint32 max_val = (max_int << FracBits) | max_frac;

		Uint32 i = f * (1 << FracBits);
		i = (i < max_val) ? i : max_val;

		return i;
	}


	static sce::Gnm::AnisotropyRatio MapIntAnisoToGNM( Uint32 aniso )
	{
		RED_ASSERT( aniso > 0, TXT("Anisotropy must be greater than 0") );

		const Uint8 _log = 31 - __builtin_clz( aniso );
		switch( _log )
		{
		case 0:			return sce::Gnm::kAnisotropyRatio1;
		case 1:			return sce::Gnm::kAnisotropyRatio2;
		case 2:			return sce::Gnm::kAnisotropyRatio4;
		case 3:			return sce::Gnm::kAnisotropyRatio8;
		case 4:			return sce::Gnm::kAnisotropyRatio16;
		default:
			RED_ASSERT( 0 , TXT("Unknown aniso value. Only support 1,2,4,8 and 16") );
		}

		return sce::Gnm::kAnisotropyRatio1;
	}

	SamplerStateRef RequestSamplerState( const SamplerStateDesc &desc )
	{
		if ( !IsInit() )
		{
			GPUAPI_HALT( "Not init during attempt to create sampler state" );
			return SamplerStateRef::Null();
		}

		if ( !IsDescSupported( desc ) )
		{
			return SamplerStateRef::Null();
		}

		SDeviceData &dd = GetDeviceData();
		
		SamplerStateRef id = SamplerStateRef( dd.m_SamplerStates.FindDescAddRef( desc ) );
		if ( !id )
		{
			id = SamplerStateRef( dd.m_SamplerStates.Create( 1 ) );
			if ( id )
			{	
				SSamplerStateData &data = dd.m_SamplerStates.Data( id );

				// GNM NOTE: I mapped pretty much everything, but there are much more sampler options then in DX11
				// GNM NOTE: Some of them may come in handy at the stage of per-platform polishing

				data.m_sampler.init();
				data.m_sampler.setDepthCompareFunction( Map( desc.comparisonFunction ) );
				data.m_sampler.setXyFilterMode( Map( desc.filterMag ), Map( desc.filterMin ) );
				data.m_sampler.setZFilterMode( sce::Gnm::kZFilterModeNone ); //<- for volume textures, do we use that?
				data.m_sampler.setMipFilterMode( Map( desc.filterMip ) );
				data.m_sampler.setWrapMode( Map( desc.addressU ), Map( desc.addressV ), Map( desc.addressW ) );

				data.m_sampler.setLodRange( FloatToUnsignedFixedPoint<4,8>(0.0f), FloatToUnsignedFixedPoint<4,8>(13.0f) );
				data.m_sampler.setLodBias( FloatToSignedFixedPoint<6,8>(desc.mipLODBias), FloatToSignedFixedPoint<2,4>(0.0f) );	// <- dunno about the secondary bias, don't know of such in DX
				data.m_sampler.setAnisotropyRatio( MapIntAnisoToGNM( MapAnisotropy( desc.filterMin, dd.m_RenderSettings.anisotropy ) ) );				

				data.m_sampler.setBorderColor( sce::Gnm::kBorderColorTransBlack );

				data.m_Desc = desc;

				//data.m_sampler.setBorderColor( sce::Gnm::kBorderColorFromTable );
				//sce::Gnm::DrawCommandBuffer::setBorderColorTableAddr(desc.borderColor);
				//data.m_sampler.setBorderColorTableIndex(0);

				/* GNM custom things that I don't set:
				setAnisotropyRange
				setAnisotropyBias
				setBorderColorTableIndex
				setDisbleCubeWrap
				setForceDegamma
				setForceUnnormalized
				setPerfMips
				setPerfZ
				setTruncateEnable
				*/
			}
			else
			{
				GPUAPI_ERROR(TXT("Failed to create sampler state - out of SSamplerStateData's"));
			}
		}

		return id;
	}

	sce::Gnm::Sampler* GetSamplerStateObject( const SamplerStateRef &ref )
	{
		GPUAPI_ASSERT( GetDeviceData().m_SamplerStates.IsInUse(ref) );
		return &(GetDeviceData().m_SamplerStates.Data(ref).m_sampler);
	}

	void InvalidateSamplerStates()
	{
		SDeviceData &dd = GetDeviceData();

		for ( Uint32 i = 0; i < MAX_PS_SAMPLER_STATES; ++i )
		{
			dd.m_PSSamplerStatePresetsCache[ i ] = SAMPSTATEPRESET_Max;
		}

		for ( Uint32 i = 0; i < MAX_VS_SAMPLER_STATES; ++i )
		{
			dd.m_VSSamplerStatePresetCache[ i ] = SAMPSTATEPRESET_Max;
		}

		for (Uint32 i = 0; i < ShaderTypeMax; ++i)
		{
			for (Uint32 j = 0; j < MAX_PS_SAMPLER_STATES; ++j)
			{
				dd.m_SamplerStateCache[i][j] = SamplerStateRef::Null();
			}
		}
	}

	// TODO: Merge this function with SetSamplerStatePreset
	void SetSamplerStateCommon( Uint32 startSlot, Uint32 numSamplers, const eSamplerStatePreset &statePreset, eShaderType shaderStage )
	{
		const SamplerStateRef &stateRef = GetSamplerStatePreset(statePreset);
		GPUAPI_ASSERT( startSlot + numSamplers <= 32, TXT( "Some semi-reasonable constant - feel free to increase this value" ) );
		SDeviceData &dd = GetDeviceData();

		if ( startSlot + numSamplers > MAX_PS_SAMPLER_STATES )
		{
			GPUAPI_HALT( "Invalid slot when setting SamplerStateRef (startSlot + numSamplers > MAX_PS_SAMPLER_STATES)" );
			return;
		}

		for ( Uint32 i=0; i<numSamplers; ++i )
		{
			GPUAPI_ASSERT( stateRef );
			Uint32 sampler_i = startSlot + i;

			dd.m_SamplerStateCache[shaderStage][sampler_i] = stateRef;

			switch (shaderStage)
			{
			case PixelShader:
				dd.m_PSSamplerStatePresetsCache[ sampler_i ] = statePreset;
				break;
			case VertexShader:
				dd.m_VSSamplerStatePresetCache[ sampler_i ] = statePreset;
				break;
			default: /*don't have to do anything here*/
				break;
			}
		}
	}

	void SetSamplerState( Uint32 slot, const SamplerStateRef& samplerRef, eShaderType shaderStage )
	{
		SDeviceData &dd = GetDeviceData();
		GPUAPI_ASSERT( samplerRef );

		if ( slot >= MAX_PS_SAMPLER_STATES )
		{
			GPUAPI_HALT( "Invalid slot when setting Pixel Shader SamplerStateRef (slot >= MAX_PS_SAMPLER_STATES)" );
			return;
		}

		// cache for later application
		dd.m_SamplerStateCache[shaderStage][slot] = samplerRef;

		switch (shaderStage)
		{
		case PixelShader:
			if ( dd.m_PSSamplerStatePresetsCache[ slot ] != samplerRef )
			{
				dd.m_PSSamplerStatePresetsCache[ slot ] = SAMPSTATEPRESET_Max;
			}
			break;
		case VertexShader:
			if ( dd.m_VSSamplerStatePresetCache[ slot ] != samplerRef )
			{
				dd.m_VSSamplerStatePresetCache[ slot ] = SAMPSTATEPRESET_Max;
			}
			break;
		default: /*No need to do anything here*/
			break;
		}
	}

	void SetSamplerStateDebugPath( const SamplerStateRef &ref, const char* debugPath )
	{
#ifdef GPU_API_DEBUG_PATH
		GPUAPI_ASSERT( GetDeviceData().m_SamplerStates.IsInUse(ref) );
#endif
	}

	void SetSamplerStatePreset( Uint32 slot, const eSamplerStatePreset &statePreset, eShaderType shaderStage )
	{
		SetSamplerStateCommon( slot, 1, statePreset, shaderStage );
	}	

	void ResetSamplerStates()
	{
		// TODO
		// EMPTY, not needed		
	}
}

#include "..\gpuApiUtils\gpuApiSamplerCommon.h"