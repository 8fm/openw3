/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#include "gpuApi.h"

extern float GRenderSettingsMipBias;
	
namespace GpuApi
{
	/*
	template< typename TFirst, typename TSecond >
	struct Pair
	{
		TFirst	m_first;
		TSecond	m_second;

		Pair()
			: m_first( (TFirst)0 )
			, m_second( (TSecond)0 )
		{
		}

		Pair( TFirst first, TSecond second )
			: m_first( first )
			, m_second( second )
		{}
	};

	static Int32 SortFunc( const void* a, const void* b )
	{
		const Pair< Uint32, SamplerStateRef >* dataA = (const Pair< Uint32, SamplerStateRef >*)a;
		const Pair< Uint32, SamplerStateRef >* dataB = (const Pair< Uint32, SamplerStateRef >*)b;
		return dataA->m_first > dataB->m_first;
	}

	struct SamplerStateMap
	{
		#define MASK_ADDRESS_U				((Uint32)( 0x3 << (14+16) ))	// 4 possibilities, 2 bits
		#define MASK_ADDRESS_V				((Uint32)( 0x3 << (12+16) ))	// 4 possibilities, 2 bits
		#define MASK_ADDRESS_W				((Uint32)( 0x3 << (10+16) ))	// 4 possibilities, 2 bits
		#define MASK_FILTERING_MIN			((Uint32)( 0x7 << ( 7+16) ))	// 4 possibilities, 3 bits (because of Aniso = 4)
		#define MASK_FILTERING_MAG			((Uint32)( 0x1 << ( 6+16) ))	// 2 possibilities, 1 bit
		#define MASK_FILTERING_MIP			((Uint32)( 0x3 << ( 4+16) ))	// 3 possibilities, 2 bits
		#define MASK_COMPARISON				((Uint32)( 0x1 << ( 3+16) ))	// 2 possibilities, 1 bit
		#define MASK_POINT_Z_FILTERING		((Uint32)( 0x1 << ( 2+16) ))	// 2 possibilities, 1 bit
		#define MASK_MIP_LOD_BIAS			0xFFFF							// Float

		typedef Pair< Uint32, SamplerStateRef > SamplerStatePair;

#define MAP_SIZE 1024
		SamplerStatePair	m_map[ MAP_SIZE ];
		Uint32				m_count;

		SamplerStateMap()
			: m_count( 0 )
		{ }

		void Store( const SamplerStateDesc& desc, const SamplerStateRef& ref )
		{
			SamplerStatePair pair( GetHash( desc ), ref );
			m_map[ m_count ] = pair;
			++m_count;

			qsort( m_map, m_count, sizeof( SamplerStatePair ), SortFunc );
		}

		SamplerStateRef& Get( Uint32 hash )
		{
			// do a binary search
			Uint32 count = m_count;
			Uint32 begin = 0;
			while( count > 0 )
			{
				Uint32 halfCount = count / 2;
				Uint32 middle = begin + halfCount;
				if ( m_map[ middle ].m_first < hash )
				{
					begin = middle + 1;
					count = count - halfCount - 1;
				}
				else
				{
					count = halfCount;
				}
			}

			return m_map[0].m_second;
		}

#if 0
		const char* ToBinary(Uint32 x)
		{
			static char b[32];
			for ( Uint32 i = 0; i < 32; ++i )
			{
				Uint32 z = 1 << i;
				b[ 31 - i ] = ((x & z) == z) ? '1' : '0';
			}

			return b;
		}
#endif

		static GpuApi::Uint32 GetHash( const SamplerStateDesc& desc )
		{
			Uint32 addU			= (Uint32)( desc.addressU << (14+16) );				// 4 possibilities, 2 bits
			Uint32 addV			= (Uint32)( desc.addressV << (12+16) );				// 4 possibilities, 2 bits
			Uint32 addW			= (Uint32)( desc.addressW << (10+16) );				// 4 possibilities, 2 bits
			Uint32 fMin			= (Uint32)( desc.filterMin << (7+16) );				// 4 possibilities, 3 bits (because of Aniso = 4)
			Uint32 fMag			= (Uint32)( desc.filterMag << (6+16) );				// 2 possibilities, 1 bit
			Uint32 fMip			= (Uint32)( desc.filterMip << (4+16) );				// 3 possibilities, 2 bits
			Uint32 comp			= (Uint32)( desc.comparisonFunction << (3+16) );				// 2 possibilities, 1 bit
			Uint32 pointZFilt	= (Uint32)( desc.pointZFilter << (2+16) );				// 2 possibilities, 1 bit
			Uint32 mipLODBias	= (*( (Uint32*)&desc.mipLODBias )) >> 16;				// Float

			Uint32 hash	=  addU;
			hash		|= addV;
			hash		|= addW;
			hash		|= fMin;
			hash		|= fMag;
			hash		|= fMip;
			hash		|= comp;
			hash		|= pointZFilt;
			hash		|= mipLODBias;

			return hash;
		}

		static SamplerStateDesc GetDesc( Uint32 hash )
		{
			Uint32 addressU		= (hash & MASK_ADDRESS_U)			>> (14+16);		// 4 possibilities, 2 bits
			Uint32 addressV		= (hash & MASK_ADDRESS_V)			>> (12+16);		// 4 possibilities, 2 bits
			Uint32 addressW		= (hash & MASK_ADDRESS_W)			>> (10+16);		// 4 possibilities, 2 bits
			Uint32 filterMin	= (hash & MASK_FILTERING_MIN)		>> (7+16);		// 4 possibilities, 2 bits
			Uint32 filterMag	= (hash & MASK_FILTERING_MAG)		>> (6+16);		// 2 possibilities, 1 bit
			Uint32 filterMip	= (hash & MASK_FILTERING_MIP)		>> (4+16);		// 3 possibilities, 2 bits
			Uint32 comparison	= (hash & MASK_COMPARISON)			>> (3+16);		// 2 possibilities, 1 bit
			Uint32 pointZFilter	= (hash & MASK_POINT_Z_FILTERING)	>> (2+16);		// 2 possibilities, 1 bit
			Uint32 mipLODBias	= (hash & MASK_MIP_LOD_BIAS)		<< 16;			// Float
			Float fMipLODBias	= *(Float*)&mipLODBias;

			SamplerStateDesc desc;
			desc.addressU = (eTextureAddress)addressU;
			desc.addressV = (eTextureAddress)addressV;
			desc.addressW = (eTextureAddress)addressW;
			desc.filterMin = (eTextureFilterMin)filterMin;
			desc.filterMag = (eTextureFilterMag)filterMag;
			desc.filterMip = (eTextureFilterMip)filterMip;
			desc.comparisonFunction = (eComparison)comparison;
			desc.pointZFilter = pointZFilter > 0;
			desc.mipLODBias = fMipLODBias;
			return desc;
		}
	};	

	// ----------------------------------------------------------------------
	SamplerStateMap gSamplerStateMap;
	*/

	namespace Utils
	{
		SamplerStateDesc BuildSamplerStateDesc( eTextureAddress address, eTextureFilterMin minFilter, eTextureFilterMag magFilter, eTextureFilterMip mipFilter, eComparison comp = COMP_None, bool pointZFilter = false, Float mipMapBiasAdd = 0.f )
		{
			SamplerStateDesc desc;
			desc.addressU = address;
			desc.addressV = address;
			desc.addressW = address;
			desc.filterMin = minFilter;
			desc.filterMag = magFilter;
			desc.filterMip = mipFilter;
			desc.mipLODBias = GpuApi::GetDeviceData().m_RenderSettings.mipMapBias + GRenderSettingsMipBias + mipMapBiasAdd;
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

	void AddRef( const SamplerStateRef &stateRef )
	{
		GPUAPI_ASSERT( GetDeviceData().m_SamplerStates.IsInUse(stateRef) );
		GetDeviceData().m_SamplerStates.IncRefCount( stateRef );
	}

	Int32 Release( const SamplerStateRef &stateRef )
	{	
		GPUAPI_ASSERT( GetDeviceData().m_SamplerStates.IsInUse(stateRef) );

		// Release
		SDeviceData &dd = GetDeviceData();
		GPUAPI_ASSERT( dd.m_SamplerStates.GetRefCount(stateRef) >= 1 );
		Int32 newRefCount = dd.m_SamplerStates.DecRefCount( stateRef );

		// Optionally destroy
		if ( 0 == newRefCount )
		{	
			SSamplerStateData &data = dd.m_SamplerStates.Data( stateRef );
			
			glDeleteSamplers( 1, &data.m_samplerID );
			dd.m_SamplerStates.Destroy( stateRef );
		}

		return 0;
	}

	Bool IsDescSupported( const SamplerStateDesc& )
	{
		// All sampler states are supported
		return true;
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

				GLuint samplerState = 0;
				glGenSamplers(1, &samplerState);

				glSamplerParameteri(samplerState, GL_TEXTURE_COMPARE_MODE, MapCompareMode(desc.comparisonFunction));
				glSamplerParameteri(samplerState, GL_TEXTURE_COMPARE_FUNC, MapCompareFunc(desc.comparisonFunction));

				glSamplerParameterf(samplerState, GL_TEXTURE_MAX_LOD, 1000.f);
				glSamplerParameterf(samplerState, GL_TEXTURE_MIN_LOD, 0.f);
				glSamplerParameterf(samplerState, GL_TEXTURE_LOD_BIAS, desc.mipLODBias);

				glSamplerParameteri(samplerState, GL_TEXTURE_MAG_FILTER, Map( desc.filterMag ) );
				glSamplerParameteri(samplerState, GL_TEXTURE_MIN_FILTER, Map( desc.filterMin, desc.filterMip ));
				glSamplerParameterf(samplerState, GL_TEXTURE_MAX_ANISOTROPY_EXT, (Float)MapAnisotropy( desc.filterMin, dd.m_RenderSettings.anisotropy ));

				glSamplerParameteri(samplerState, GL_TEXTURE_WRAP_S, Map( desc.addressU ) );
				glSamplerParameteri(samplerState, GL_TEXTURE_WRAP_T, Map( desc.addressV ) );
				glSamplerParameteri(samplerState, GL_TEXTURE_WRAP_R, Map( desc.addressW ) );

				GLfloat borderColor[4] = { 0.f, 0.f, 0.f, 0.f };
				glSamplerParameterfv(samplerState, GL_TEXTURE_BORDER_COLOR, borderColor );

#ifdef GPU_API_DEBUG_PATH
				//data.m_samplerState->SetPrivateData( WKPDID_D3DDebugObjectName, 4, "samp" );
#endif
				data.m_Desc = desc;
				data.m_samplerID = samplerState;
			}
		}

		return id;
	}

	GLuint GetSamplerStateID( const SamplerStateRef &ref )
	{
		GPUAPI_ASSERT( GetDeviceData().m_SamplerStates.IsInUse(ref) );
		return GetDeviceData().m_SamplerStates.Data(ref).m_samplerID;
	}
	
	void InvalidateSamplerStates()
	{
		SDeviceData &dd = GetDeviceData();

		for ( Uint32 i = 0; i < MAX_PS_SAMPLER_STATES; ++i )
		{
			glBindSampler( i, 0 );
			dd.m_PSSamplerStatePresetsCache[ i ] = SAMPSTATEPRESET_Max;
			dd.m_PSSamplerStateCache[ i ] = SamplerStateRef::Null();
		}
	}

	void SetSamplerStateCommon( Uint32 startSlot, Uint32 numSamplers, const eSamplerStatePreset &statePreset, eShaderType shaderStage )
	{
		const SamplerStateRef &stateRef = GetSamplerStatePreset(statePreset);
		GPUAPI_ASSERT( stateRef );
		GPUAPI_ASSERT( startSlot + numSamplers <= 32, TXT( "Some semi-reasonable constant - feel free to increase this value" ) );
		SDeviceData &dd = GetDeviceData();
		GLuint stateID = GetSamplerStateID(stateRef);

		//for ( Uint32 i=0; i<numSamplers; ++i )
		//{
		//	Uint32 sampler_i = startSlot + i;

		//	// TODO moradin deal with shader stages somehow
		//	glBindSamplers( sampler_i, 1, &stateID );
		//	
		//	//switch (shaderStage)
		//	//{
		//	//case PixelShader:
		//	//	{
		//	//		GPUAPI_ASSERT( startSlot + numSamplers <= MAX_PS_SAMPLER_STATES );
		//	//		if ( dd.m_PSSamplerStatePresetsCache[ sampler_i ] != statePreset )
		//	//		{
		//	//			GetDeviceContext()->PSSetSamplers( sampler_i, 1, &stateObject );
		//	//			dd.m_PSSamplerStatePresetsCache[ sampler_i ] = statePreset;
		//	//			dd.m_PSSamplerStateCache[ sampler_i ] = stateRef;
		//	//		}
		//	//		break;
		//	//	}
		//	//case VertexShader:
		//	//	{
		//	//		GPUAPI_ASSERT( startSlot + numSamplers <= MAX_VS_SAMPLER_STATES );
		//	//		if ( dd.m_VSSamplerStatePresetCache[ sampler_i ] != statePreset )
		//	//		{
		//	//			GetDeviceContext()->VSSetSamplers( sampler_i, 1, &stateObject );
		//	//			dd.m_VSSamplerStatePresetCache[ sampler_i ] = statePreset;
		//	//			dd.m_VSSamplerStateCache[ sampler_i ] = stateRef;
		//	//		}
		//	//		break;
		//	//	}
		//	//case GeometryShader:
		//	//	{
		//	//		GetDeviceContext()->GSSetSamplers( sampler_i, 1, &stateObject );
		//	//		break;
		//	//	}
		//	//case HullShader:
		//	//	{
		//	//		GetDeviceContext()->HSSetSamplers( sampler_i, 1, &stateObject );
		//	//		break;
		//	//	}
		//	//case DomainShader:
		//	//	{
		//	//		GetDeviceContext()->DSSetSamplers( sampler_i, 1, &stateObject );
		//	//		break;
		//	//	}
		//	//case ComputeShader:
		//	//	{
		//	//		GetDeviceContext()->CSSetSamplers( sampler_i, 1, &stateObject );
		//	//		break;
		//	//	}
		//	//}
		//}
	}

	void SetSamplerState( Uint32 slot, const SamplerStateRef& state, eShaderType shaderStage )
	{
		SDeviceData &dd = GetDeviceData();
		GPUAPI_ASSERT( state );

		GLuint stateID = GetSamplerStateID(state);
		// GL TODO: deal with shader stages somehow
		glBindSamplers(slot, 1, &stateID);

		//ID3D11SamplerState* stateObject = GetSamplerStateObject( state );
		//switch (shaderStage)
		//{
		//case PixelShader:
		//	{
		//		if ( dd.m_PSSamplerStateCache[ slot ] != state )
		//		{
		//			if ( slot >= MAX_PS_SAMPLER_STATES )
		//			{
		//				GPUAPI_HALT( TXT( "Invalid slot when setting Pixel Shader SamplerStateRef (slot >= MAX_PS_SAMPLER_STATES)" ) );
		//				return;
		//			}
		//			GetDeviceContext()->PSSetSamplers( slot, 1, &stateObject );
		//			dd.m_PSSamplerStateCache[ slot ] = state;
		//			dd.m_PSSamplerStatePresetsCache[ slot ] = SAMPSTATEPRESET_Max;
		//		}

		//		break;
		//	}
		//case VertexShader:
		//	{
		//		if ( dd.m_VSSamplerStateCache[ slot ] != state )
		//		{
		//			if ( slot >= MAX_VS_SAMPLER_STATES )
		//			{
		//				GPUAPI_HALT( TXT( "Invalid slot when setting Vertex Shader SamplerStateRef (slot >= MAX_VS_SAMPLER_STATES)" ) );
		//				return;
		//			}
		//			GetDeviceContext()->VSSetSamplers( slot, 1, &stateObject );
		//			dd.m_VSSamplerStateCache[ slot ] = state;
		//			dd.m_PSSamplerStatePresetsCache[ slot ] = SAMPSTATEPRESET_Max;
		//		}
		//		break;
		//	}
		//case GeometryShader:
		//	{
		//		GetDeviceContext()->GSSetSamplers( slot, 1, &stateObject );
		//		break;
		//	}
		//case HullShader:
		//	{
		//		GetDeviceContext()->HSSetSamplers( slot, 1, &stateObject );
		//		break;
		//	}
		//case DomainShader:
		//	{
		//		GetDeviceContext()->DSSetSamplers( slot, 1, &stateObject );
		//		break;
		//	}
		//case ComputeShader:
		//	{
		//		GetDeviceContext()->CSSetSamplers( slot, 1, &stateObject );
		//		break;
		//	}
		//}
	}

	void SetSamplerStateDebugPath( const SamplerStateRef &ref, const char* debugPath )
	{
#ifdef GPU_API_DEBUG_PATH
		GPUAPI_ASSERT( GetDeviceData().m_SamplerStates.IsInUse(ref) );
		SSamplerStateData &data = GetDeviceData().m_SamplerStates.Data(ref);

		Uint32 pathLen = ( Uint32 )Red::System::StringLength( debugPath );

		// Destroy previous data
		data.m_samplerState->SetPrivateData( WKPDID_D3DDebugObjectName, 0, NULL );

		if (pathLen > 0)
		{
			data.m_samplerState->SetPrivateData( WKPDID_D3DDebugObjectName, pathLen, debugPath );
		}
#endif
	}

	void SetSamplerStatePreset( Uint32 slot, const eSamplerStatePreset &statePreset, eShaderType shaderStage )
	{
		SDeviceData &dd = GetDeviceData();
		const SamplerStateRef &stateRef = GetSamplerStatePreset(statePreset);
		GPUAPI_ASSERT( stateRef );

		GLuint stateID = GetSamplerStateID(stateRef);
		// GL TODO: deal with shader stages somehow
		glBindSampler( slot, stateID );

		//ID3D11SamplerState* stateObject = GetSamplerStateObject(stateRef);
		//switch (shaderStage)
		//{
		//case PixelShader:
		//	{
		//		if ( dd.m_PSSamplerStatePresetsCache[ slot ] != statePreset )
		//		{
		//			if( slot >= MAX_PS_SAMPLER_STATES )
		//			{
		//				GPUAPI_HALT( TXT( "Setting sampler state >=MAX_PS_SAMPLER_STATES" ) );
		//				return;
		//			}
		//			GetDeviceContext()->PSSetSamplers( slot, 1, &stateObject );
		//			dd.m_PSSamplerStatePresetsCache[ slot ] = statePreset;
		//			dd.m_PSSamplerStateCache[ slot ] = stateRef;
		//		}
		//		
		//		break;
		//	}
		//case VertexShader:
		//	{
		//		if ( dd.m_VSSamplerStatePresetCache[ slot ] != statePreset )
		//		{
		//			if( slot >= MAX_VS_SAMPLER_STATES )
		//			{
		//				GPUAPI_HALT( TXT( "Setting sampler state >=MAX_VS_SAMPLER_STATES" ) );
		//				return;
		//			}
		//			GetDeviceContext()->VSSetSamplers( slot, 1, &stateObject );
		//			dd.m_VSSamplerStatePresetCache[ slot ] = statePreset;
		//			dd.m_VSSamplerStateCache[ slot ] = stateRef;
		//		}
		//		break;
		//	}
		//case GeometryShader:
		//	{
		//		GetDeviceContext()->GSSetSamplers( slot, 1, &stateObject );
		//		break;
		//	}
		//case HullShader:
		//	{
		//		GetDeviceContext()->HSSetSamplers( slot, 1, &stateObject );
		//		break;
		//	}
		//case DomainShader:
		//	{
		//		GetDeviceContext()->DSSetSamplers( slot, 1, &stateObject );
		//		break;
		//	}
		//case ComputeShader:
		//	{
		//		GetDeviceContext()->CSSetSamplers( slot, 1, &stateObject );
		//		break;
		//	}
		//}
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
		INIT_PRESET_SF( SAMPSTATEPRESET_Scaleform_WrapPointMip,	TEXADDR_Wrap,	TEXFILTERMIN_Point,		TEXFILTERMAG_Point,		TEXFILTERMIP_Point );
		INIT_PRESET_SF( SAMPSTATEPRESET_Scaleform_WrapLinearMip, TEXADDR_Wrap,	TEXFILTERMIN_Linear,	TEXFILTERMAG_Linear,	TEXFILTERMIP_Linear );
		INIT_PRESET_SF( SAMPSTATEPRESET_Scaleform_ClampPointMip, TEXADDR_Clamp,	TEXFILTERMIN_Point,		TEXFILTERMAG_Point,		TEXFILTERMIP_Point );
		INIT_PRESET_SF( SAMPSTATEPRESET_Scaleform_ClampLinearMip, TEXADDR_Clamp, TEXFILTERMIN_Linear,	TEXFILTERMAG_Linear,	TEXFILTERMIP_Linear );

		dd.m_SamplerStatePresets[SAMPSTATEPRESET_ClampPointNoMipCompareLess] =
			RequestSamplerState( Utils::BuildSamplerStateDesc( TEXADDR_Clamp, TEXFILTERMIN_Point, TEXFILTERMAG_Point, TEXFILTERMIP_Point, COMP_Less ) );
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
