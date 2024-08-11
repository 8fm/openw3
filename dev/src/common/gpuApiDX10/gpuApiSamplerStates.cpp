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
	*/

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
			desc.mipLODBias = mipMapBiasAdd;
			desc.pointZFilter = pointZFilter;
			desc.comparisonFunction = comp;
			desc.allowSettingsBias = useSettingsBias;

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
			desc.allowSettingsBias = false;
			return desc;
		}
	}

	// ----------------------------------------------------------------------

	ID3D11SamplerState* CreateSamplerStateByDesc( const SamplerStateDesc &desc )
	{
		SDeviceData &dd = GetDeviceData();
		
		D3D11_SAMPLER_DESC samplerDesc;
		samplerDesc.ComparisonFunc = Map( desc.comparisonFunction );
		samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
		samplerDesc.Filter = Map(desc.filterMin, desc.filterMag, desc.filterMip, desc.comparisonFunction );
		samplerDesc.AddressU = Map(desc.addressU);
		samplerDesc.AddressV = Map(desc.addressV);
		samplerDesc.AddressW = Map(desc.addressW);
		samplerDesc.MipLODBias = desc.mipLODBias;
		samplerDesc.MaxAnisotropy = MapAnisotropy( desc.filterMin, dd.m_RenderSettings.anisotropy );
		samplerDesc.BorderColor[0] = desc.borderColor[0];
		samplerDesc.BorderColor[1] = desc.borderColor[1];
		samplerDesc.BorderColor[2] = desc.borderColor[2];
		samplerDesc.BorderColor[3] = desc.borderColor[3];
		samplerDesc.MinLOD = 0.f;

		if ( desc.allowSettingsBias )
		{
			samplerDesc.MipLODBias += GpuApi::GetDeviceData().m_RenderSettings.mipMapBias + GRenderSettingsMipBias;
		}

		ID3D11SamplerState *samplerState = nullptr;
		GetDevice()->CreateSamplerState(&samplerDesc, &samplerState);
		return samplerState;
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
			SSamplerStateData &data = dd.m_SamplerStates.Data( stateRef );
			ULONG refcount = data.m_samplerState->Release();
			RED_UNUSED(refcount);
			//GPUAPI_ASSERT( refcount == 0, TXT( "D3DSamplerState refcount > 0, object won't be destroyed" ) );
			// it looks like there are multiple references on the same D3D object but we will release all of them
			dd.m_SamplerStates.Destroy( stateRef );
		}
		return refCount;
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
			GPUAPI_ASSERT( !id.isNull(), TXT("Creating SamplerState failed.") );
			if ( id )
			{	
				SSamplerStateData &data = dd.m_SamplerStates.Data( id );
				data.m_samplerState = CreateSamplerStateByDesc( desc );
				data.m_Desc = desc;
			}
		}

		return id;
	}

	ID3D11SamplerState* GetSamplerStateObject( const SamplerStateRef &ref )
	{
		GPUAPI_ASSERT( GetDeviceData().m_SamplerStates.IsInUse(ref) );
		return GetDeviceData().m_SamplerStates.Data(ref).m_samplerState;
	}
	
	void InvalidateSamplerStates()
	{
		SDeviceData &dd = GetDeviceData();

		for ( Uint32 i = 0; i < MAX_PS_SAMPLER_STATES; ++i )
		{
			ID3D11SamplerState* nullSamplerState = NULL;
			GetDeviceContext()->PSSetSamplers( i, 1, &nullSamplerState );
			dd.m_PSSamplerStatePresetsCache[ i ] = SAMPSTATEPRESET_Max;
			dd.m_PSSamplerStateCache[ i ] = SamplerStateRef::Null();
		}

		for ( Uint32 i = 0; i < MAX_VS_SAMPLER_STATES; ++i )
		{
			ID3D11SamplerState* nullSamplerState = NULL;
			GetDeviceContext()->VSSetSamplers( i, 1, &nullSamplerState );
			dd.m_VSSamplerStatePresetCache[ i ] = SAMPSTATEPRESET_Max;
			dd.m_VSSamplerStateCache[ i ] = SamplerStateRef::Null();
		}
	}

	void SetSamplerStateCommon( Uint32 startSlot, Uint32 numSamplers, const eSamplerStatePreset &statePreset, eShaderType shaderStage )
	{
		const SamplerStateRef &stateRef = GetSamplerStatePreset(statePreset);
		GPUAPI_ASSERT( startSlot + numSamplers <= 32, TXT( "Some semi-reasonable constant - feel free to increase this value" ) );
		SDeviceData &dd = GetDeviceData();
		for ( Uint32 i=0; i<numSamplers; ++i )
		{
			GPUAPI_ASSERT( stateRef );
			Uint32 sampler_i = startSlot + i;

			ID3D11SamplerState* stateObject = GetSamplerStateObject(stateRef);
			switch (shaderStage)
			{
			case PixelShader:
				{
					GPUAPI_ASSERT( startSlot + numSamplers <= MAX_PS_SAMPLER_STATES );
					if ( dd.m_PSSamplerStatePresetsCache[ sampler_i ] != statePreset )
					{
						GetDeviceContext()->PSSetSamplers( sampler_i, 1, &stateObject );
						dd.m_PSSamplerStatePresetsCache[ sampler_i ] = statePreset;
						dd.m_PSSamplerStateCache[ sampler_i ] = stateRef;
					}
					break;
				}
			case VertexShader:
				{
					GPUAPI_ASSERT( startSlot + numSamplers <= MAX_VS_SAMPLER_STATES );
					if ( dd.m_VSSamplerStatePresetCache[ sampler_i ] != statePreset )
					{
						GetDeviceContext()->VSSetSamplers( sampler_i, 1, &stateObject );
						dd.m_VSSamplerStatePresetCache[ sampler_i ] = statePreset;
						dd.m_VSSamplerStateCache[ sampler_i ] = stateRef;
					}
					break;
				}
			case GeometryShader:
				{
					GetDeviceContext()->GSSetSamplers( sampler_i, 1, &stateObject );
					break;
				}
			case HullShader:
				{
					GetDeviceContext()->HSSetSamplers( sampler_i, 1, &stateObject );
					break;
				}
			case DomainShader:
				{
					GetDeviceContext()->DSSetSamplers( sampler_i, 1, &stateObject );
					break;
				}
			case ComputeShader:
				{
					GetDeviceContext()->CSSetSamplers( sampler_i, 1, &stateObject );
					break;
				}
			}
		}
	}

	void SetSamplerState( Uint32 slot, const SamplerStateRef& state, eShaderType shaderStage )
	{
		SDeviceData &dd = GetDeviceData();
		GPUAPI_ASSERT( state );

		ID3D11SamplerState* stateObject = GetSamplerStateObject( state );
		switch (shaderStage)
		{
		case PixelShader:
			{
				if ( dd.m_PSSamplerStateCache[ slot ] != state )
				{
					if ( slot >= MAX_PS_SAMPLER_STATES )
					{
						GPUAPI_HALT( "Invalid slot when setting Pixel Shader SamplerStateRef (slot >= MAX_PS_SAMPLER_STATES)" );
						return;
					}
					GetDeviceContext()->PSSetSamplers( slot, 1, &stateObject );
					dd.m_PSSamplerStateCache[ slot ] = state;
					dd.m_PSSamplerStatePresetsCache[ slot ] = SAMPSTATEPRESET_Max;
				}

				break;
			}
		case VertexShader:
			{
				if ( dd.m_VSSamplerStateCache[ slot ] != state )
				{
					if ( slot >= MAX_VS_SAMPLER_STATES )
					{
						GPUAPI_HALT( "Invalid slot when setting Vertex Shader SamplerStateRef (slot >= MAX_VS_SAMPLER_STATES)" );
						return;
					}
					GetDeviceContext()->VSSetSamplers( slot, 1, &stateObject );
					dd.m_VSSamplerStateCache[ slot ] = state;
					dd.m_PSSamplerStatePresetsCache[ slot ] = SAMPSTATEPRESET_Max;
				}
				break;
			}
		case GeometryShader:
			{
				GetDeviceContext()->GSSetSamplers( slot, 1, &stateObject );
				break;
			}
		case HullShader:
			{
				GetDeviceContext()->HSSetSamplers( slot, 1, &stateObject );
				break;
			}
		case DomainShader:
			{
				GetDeviceContext()->DSSetSamplers( slot, 1, &stateObject );
				break;
			}
		case ComputeShader:
			{
				GetDeviceContext()->CSSetSamplers( slot, 1, &stateObject );
				break;
			}
		}
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

		ID3D11SamplerState* stateObject = GetSamplerStateObject(stateRef);
		switch (shaderStage)
		{
		case PixelShader:
			{
				if ( dd.m_PSSamplerStatePresetsCache[ slot ] != statePreset )
				{
					if( slot >= MAX_PS_SAMPLER_STATES )
					{
						GPUAPI_HALT( "Setting sampler state >=MAX_PS_SAMPLER_STATES" );
						return;
					}
					GetDeviceContext()->PSSetSamplers( slot, 1, &stateObject );
					dd.m_PSSamplerStatePresetsCache[ slot ] = statePreset;
					dd.m_PSSamplerStateCache[ slot ] = stateRef;
				}
				
				break;
			}
		case VertexShader:
			{
				if ( dd.m_VSSamplerStatePresetCache[ slot ] != statePreset )
				{
					if( slot >= MAX_VS_SAMPLER_STATES )
					{
						GPUAPI_HALT( "Setting sampler state >=MAX_VS_SAMPLER_STATES" );
						return;
					}
					GetDeviceContext()->VSSetSamplers( slot, 1, &stateObject );
					dd.m_VSSamplerStatePresetCache[ slot ] = statePreset;
					dd.m_VSSamplerStateCache[ slot ] = stateRef;
				}
				break;
			}
		case GeometryShader:
			{
				GetDeviceContext()->GSSetSamplers( slot, 1, &stateObject );
				break;
			}
		case HullShader:
			{
				GetDeviceContext()->HSSetSamplers( slot, 1, &stateObject );
				break;
			}
		case DomainShader:
			{
				GetDeviceContext()->DSSetSamplers( slot, 1, &stateObject );
				break;
			}
		case ComputeShader:
			{
				GetDeviceContext()->CSSetSamplers( slot, 1, &stateObject );
				break;
			}
		}
	}

	void ResetSamplerStates()
	{
		SDeviceData &dd = GetDeviceData();

		// Reset sampler state cache in order to ensure that new sampler state will be rebound in case it's already bound
		InvalidateSamplerStates();

		// Get all sampler states and addref
		Uint32 numAllSamplerStates = 0;
		Uint32 *allSamplerStates = nullptr;
		dd.m_SamplerStates.GetAllAddRef( numAllSamplerStates, allSamplerStates );

		// Reset sampler states and dec ref's
		for ( Uint32 ss_i=0; ss_i<numAllSamplerStates; ++ss_i )
		{
			const Uint32 ssId = allSamplerStates[ss_i];
			
			// Recreate sampler state
			SSamplerStateData &data = dd.m_SamplerStates.Data( ssId );
			GPUAPI_ASSERT( data.m_samplerState );
			if ( data.m_samplerState )
			{
				data.m_samplerState->Release();
				data.m_samplerState = CreateSamplerStateByDesc( data.m_Desc );
				GPUAPI_ASSERT( data.m_samplerState );
			}

			// Release reference
			SamplerStateRef ssRef ( ssId );
			SafeRelease( ssRef );
		}

		// Free temp memory
		delete [] allSamplerStates;
	}

}

#include "..\gpuApiUtils\gpuApiSamplerCommon.h"