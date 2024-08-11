/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "gpuApi.h"
#include "../gpuApiUtils/gpuApiMemory.h"
#include "../redSystem/hash.h"

#ifdef GPU_API_DEBUG_PATH
#include <wchar.h>
#endif
#include <string>
#include "../redSystem/hash.h"

// HACK HACK

namespace GpuApi
{	
	void DeleteShaderInternal( SDeviceData &dd, SShaderData &data );

	void AddRef( const ShaderRef &shader )
	{
		GPUAPI_ASSERT( GetDeviceData().m_Shaders.IsInUse(shader) );
		GetDeviceData().m_Shaders.IncRefCount( shader );
	}

	void DeleteFetchShader(SDeviceData &dd, Uint16 id)
	{
		if ( id != 0 )
		{
			if ( dd.m_fetchShaders[id] != nullptr ) 
			{
				GPU_API_FREE(GpuMemoryPool_Shaders, MC_ShaderCode, dd.m_fetchShaders[id]->m_fetchShaderMemory);

				delete dd.m_fetchShaders[id]->m_resourceTable;
				delete dd.m_fetchShaders[id];

				dd.m_fetchShaders[id] = nullptr;
			}

			// release the fetch shader id
			extern void PushFetchShaderId(SDeviceData& dd, Uint16 id);
			PushFetchShaderId(dd, id);
		}
	}

	Int32 Release( const ShaderRef &shader )
	{
		GPUAPI_ASSERT( shader );

		SDeviceData &dd = GetDeviceData();
		GPUAPI_ASSERT( dd.m_Shaders.IsInUse(shader) );
		GPUAPI_ASSERT( dd.m_Shaders.GetRefCount(shader) >= 1 );
		Int32 refCount = dd.m_Shaders.DecRefCount( shader );
		if ( 0 == refCount )
		{
			QueueForDestroy( shader );
		}
		return refCount;
	}

	void Destroy(const ShaderRef& shader)
	{
		SDeviceData &dd = GetDeviceData();
		SShaderData &data = dd.m_Shaders.Data( shader );

		// Release resources
		GPUAPI_ASSERT( nullptr != data.m_shaderStruct );
		if ( data.m_shaderStruct )
		{
			DeleteShaderInternal( dd, data );
		}

		delete data.m_streamOutDesc;

		for (Uint32 i = 0; i < ARRAY_COUNT(data.m_lsFetchShaderId); ++i)
		{
			DeleteFetchShader( dd, data.m_vsFetchShaderId[i] );
			DeleteFetchShader( dd, data.m_lsFetchShaderId[i] );
			DeleteFetchShader( dd, data.m_esFetchShaderId[i] );
		}

		if ( data.m_sourceBinaryData )
		{
			GPU_API_FREE( GpuMemoryPool_Shaders, MC_ShaderCode, data.m_sourceBinaryData );
			data.m_sourceBinaryData = nullptr;
			data.m_sourceBinaryDataSize = 0;
		}		

		// Destroy shit
		dd.m_Shaders.Destroy( shader );
	}

	ShaderRef CreateGeometryShaderWithSOFromSource( const char* code, const char* mainFunction, const char* shaderTarget, ShaderDefine* defines, Uint32 numDefines, const char* fileName, const VertexLayoutDesc& outputDesc, GpuApi::VertexLayoutDesc* adjustedDesc /*nullptr*/ )
	{
		GPUAPI_LOG( TXT("Tried to create shader from source file, named: %hs, returning null shader"), fileName );

		return ShaderRef::Null();
	}

	ShaderRef CreateGeometryShaderWithSOFromBinary( const void* shaderBuffer, Uint32 shaderBufferSize, const VertexLayoutDesc& outputDesc, GpuApi::VertexLayoutDesc* adjustedDesc /*nullptr*/ )
	{
		ShaderRef shaderRef = CreateShaderFromBinary (GpuApi::GeometryShader, shaderBuffer, shaderBufferSize);

		VertexLayoutDesc* descCopy = new VertexLayoutDesc(outputDesc);
		GetDeviceData().m_Shaders.Data( shaderRef ).m_streamOutDesc = descCopy;

		return shaderRef;
	}

	ShaderRef CreateShaderFromSource( eShaderType shaderType, const char* code, const char* mainFunction, const char* shaderTarget, ShaderDefine* defines, Uint32 numDefines, const char* fileName )
	{
		//GPUAPI_HALT(TXT("NOT AVAILABLE ON ORBIS")); // Not from source

		GPUAPI_LOG( TXT("Tried to create shader from source file, named: %hs, returning null shader"), fileName );

		return ShaderRef::Null();
	}

	Bool GetShaderTargetAndEntryPoint( eShaderType shaderType, const AnsiChar*& outShaderTarget, const AnsiChar*& outMainFunction )
	{
		switch ( shaderType )
		{
		case GpuApi::PixelShader:
			outMainFunction = "ps_main";
			outShaderTarget = "sce_ps_orbis";
			break;
		case GpuApi::VertexShader:
			outMainFunction = "vs_main";
			outShaderTarget = "sce_vs_vs_orbis";
			break;
		case GpuApi::DomainShader:
			outMainFunction = "ds_main";
			outShaderTarget = "sce_ds_vs_orbis";
			break;
		case GpuApi::ComputeShader:
			outMainFunction = "cs_main";
			outShaderTarget = "sce_cs_orbis";
			break;
		case GpuApi::GeometryShader:
			outMainFunction = "gs_main";
			outShaderTarget = "sce_gs_orbis";
			break;
		case GpuApi::HullShader:
			outMainFunction = "hs_main";
			outShaderTarget = "sce_hs_orbis";
			break;
		default:
			GPUAPI_HALT( "Invalid shader type" );
			return false;
		}
		return true;
	}


	/*
	This function takes a VsShader/PsShader/HsShader/etc., extracts header from it, and returns it.
	The header returned is of the same type, but this isn't the same object.
	NOT FOR USE WITH THE GEOMETRY AND COMPUTE SHADERS.
	*/
	template < typename GNMX_SHADERTYPE >
	GNMX_SHADERTYPE* ExtractShaderHeader( GNMX_SHADERTYPE* inShader, void* shaderBinaryGarlic )
	{
		// Copy shader header to onion memory
		void* shaderHeader = GPU_API_ALLOCATE( GpuMemoryPool_SmallOnion, MC_ShaderHeaders, inShader->computeSize(), sce::Gnm::kAlignmentOfBufferInBytes );
		Red::System::MemoryCopy( shaderHeader, inShader, inShader->computeSize() );

		// Store GPU shader address in the header
		GNMX_SHADERTYPE* typedHeader = static_cast< GNMX_SHADERTYPE* >( shaderHeader );
		typedHeader->patchShaderGpuAddress( shaderBinaryGarlic );

		return typedHeader;
	}

	template < typename GNMX_SHADERTYPE >
	void SetupScratchPad( SShaderData &data, GNMX_SHADERTYPE* inShader )
	{
		if ( inShader->m_common.m_scratchSizeInDWPerThread > 0 )
		{
			// Need to allocate a scratch buffer
			Uint32 maxNumWaves = 32 * 18;
			Uint32 num1KbyteChunksPerWave = (inShader->m_common.m_scratchSizeInDWPerThread + 3) / 4;

			if ( num1KbyteChunksPerWave > SShaderData::s_graphicsNum1KbyteChunksPerWave )
			{
				GPUAPI_LOG( TXT("Enlarging scratch pad.") );

				data.s_graphicsMaxNumWaves = maxNumWaves;
				data.s_graphicsNum1KbyteChunksPerWave = num1KbyteChunksPerWave;
				data.s_graphicsScratchBufferSize = maxNumWaves * num1KbyteChunksPerWave * 1024;

				void* scratchBufferMemory = GPU_API_ALLOCATE( GpuMemoryPool_Shaders, MC_GPURingBuffers, data.s_graphicsScratchBufferSize, sce::Gnm::kAlignmentOfShaderInBytes );
				if ( !scratchBufferMemory )
				{
					GPUAPI_HALT( "Can't allocate scratch buffer memory!" );
					return;
				}
				data.s_graphicsScratchBuffer.initAsScratchRingBuffer( scratchBufferMemory, data.s_graphicsScratchBufferSize );
			}
		}
	}

	ShaderRef CreateShaderFromBinary( eShaderType shaderType, const void* shaderBuffer, Uint32 shaderBufferSize, const char* debugName )
	{
		SDeviceData &dd = GetDeviceData();

		// Create GpuApi shader.
		Uint32 newShaderId = dd.m_Shaders.Create( 1 );
		if ( !newShaderId )
		{
			GPUAPI_HALT( "Can't create another gpuapi shader - reached full capacity." );
			return ShaderRef::Null();
		}
		GPUAPI_ASSERT( dd.m_Shaders.IsInUse( newShaderId ) );
		SShaderData &data = dd.m_Shaders.Data( newShaderId );
		data.m_type = shaderType;

		void* shaderInitData = const_cast< void* >( shaderBuffer );
		if ( shaderType == VertexShader )
		{
			data.m_sourceBinaryDataSize = shaderBufferSize;
			data.m_sourceBinaryData = GPU_API_ALLOCATE( GpuMemoryPool_Shaders, MC_ShaderCode, shaderBufferSize, 16 );
			Red::System::MemoryCopy( data.m_sourceBinaryData, shaderBuffer, shaderBufferSize );

			shaderInitData = data.m_sourceBinaryData;
		}

		// parse the shader program to allow reflection and semantic remapping etc
		sce::Shader::Binary::PsslStatus program_load = data.m_program.loadFromMemory( shaderInitData, shaderBufferSize );
		GPUAPI_ASSERT (program_load == sce::Shader::Binary::kStatusOk);
		RED_UNUSED(program_load);

		// Manually pull shader type info from the binary - because you know - SONY
		sce::Shader::Binary::PsslShaderType psslType = sce::Shader::Binary::kShaderTypeShaderTypeLast;
		sce::Gnmx::ShaderType shaderStageType = sce::Gnmx::kInvalidShader;
		{
			const sce::Shader::Binary::Header *binaryHeader = reinterpret_cast<const sce::Shader::Binary::Header*>( shaderInitData );
			psslType = static_cast< sce::Shader::Binary::PsslShaderType >( binaryHeader->m_shaderType );

			const sce::Gnmx::ShaderFileHeader *header = reinterpret_cast<const sce::Gnmx::ShaderFileHeader*>( binaryHeader + 1 );
			GPUAPI_ASSERT( header->m_fileHeader == sce::Gnmx::kShaderFileHeaderId, TXT("Something is wrong with the shader file header. The binary may be corrupted.") );
			shaderStageType = static_cast< sce::Gnmx::ShaderType >( header->m_type );
		}

		// Parse and setup the shader according to it's type
		switch( shaderStageType )
		{
		case sce::Gnmx::kLocalShader: 
			{
				// It is a vertex shader ( Tess ON ).
				// ==============================================================================
				GPUAPI_ASSERT( shaderType == VertexShader );
				GPUAPI_ASSERT( psslType == sce::Shader::Binary::kShaderTypeVsShader );

				// Parse shader binary.
				sce::Gnmx::ShaderInfo lsShaderInfo;
				sce::Gnmx::parseShader( &lsShaderInfo, shaderInitData );

				// Copy shader code to Garlic memory
				void* shaderBinaryGarlic  = GPU_API_ALLOCATE( GpuMemoryPool_Shaders, MC_ShaderCode, lsShaderInfo.m_gpuShaderCodeSize, sce::Gnm::kAlignmentOfShaderInBytes );
				Red::System::MemoryCopy( shaderBinaryGarlic, lsShaderInfo.m_gpuShaderCode, lsShaderInfo.m_gpuShaderCodeSize );

				// Store the header pointer in the shader data.
				data.m_lsShader = ExtractShaderHeader( lsShaderInfo.m_lsShader, shaderBinaryGarlic );

				SetupScratchPad( data, data.m_lsShader );
			}
			break;
		case sce::Gnmx::kVertexShader: 
			{
				// It is a vertex shader ( Tess OFF Geom OFF ) or a domain shader (Tess ON, Geom OFF)
				// ========================================================================================================
				GPUAPI_ASSERT( shaderType == VertexShader || shaderType == DomainShader );
				GPUAPI_ASSERT( psslType == sce::Shader::Binary::kShaderTypeVsShader || psslType == sce::Shader::Binary::kShaderTypeDsShader );

				// Parse shader binary.
				sce::Gnmx::ShaderInfo vsShaderInfo;
				sce::Gnmx::parseShader( &vsShaderInfo, shaderInitData );

				// Copy shader code to Garlic memory
				void* shaderBinaryGarlic  = GPU_API_ALLOCATE( GpuMemoryPool_Shaders, MC_ShaderCode, vsShaderInfo.m_gpuShaderCodeSize, sce::Gnm::kAlignmentOfShaderInBytes );
				Red::System::MemoryCopy( shaderBinaryGarlic, vsShaderInfo.m_gpuShaderCode, vsShaderInfo.m_gpuShaderCodeSize );

				// Store the header pointer in the shader data.
				data.m_vsShader = ExtractShaderHeader( vsShaderInfo.m_vsShader, shaderBinaryGarlic );

				SetupScratchPad( data, data.m_vsShader );
			}
			break;
		case sce::Gnmx::kHullShader: 
			{
				// It is a hull shader ( Tess ON )
				// =====================================================
				GPUAPI_ASSERT( shaderType == HullShader );
				GPUAPI_ASSERT( psslType == sce::Shader::Binary::kShaderTypeHsShader );

				// Parse shader binary.
				sce::Gnmx::ShaderInfo hsShaderInfo;
				sce::Gnmx::parseShader( &hsShaderInfo, shaderInitData );

				// Copy shader code to Garlic memory
				void* shaderBinaryGarlic  = GPU_API_ALLOCATE( GpuMemoryPool_Shaders, MC_ShaderCode, hsShaderInfo.m_gpuShaderCodeSize, sce::Gnm::kAlignmentOfShaderInBytes );
				Red::System::MemoryCopy( shaderBinaryGarlic, hsShaderInfo.m_gpuShaderCode, hsShaderInfo.m_gpuShaderCodeSize );

				// Store the header pointer in the shader data.
				data.m_hsShader = ExtractShaderHeader( hsShaderInfo.m_hsShader, shaderBinaryGarlic );

				SetupScratchPad( data, data.m_hsShader );
			}
			break;
		case sce::Gnmx::kExportShader: 
			{
				// It is a vertex shader ( Tess OFF, Geom ON ), or a domain shader (Tess ON, Geom ON)
				// ===================================================================================
				GPUAPI_ASSERT( shaderType == VertexShader || shaderType == DomainShader );
				GPUAPI_ASSERT( psslType == sce::Shader::Binary::kShaderTypeVsShader || psslType == sce::Shader::Binary::kShaderTypeDsShader );

				// Parse shader binary.
				sce::Gnmx::ShaderInfo esShaderInfo;
				sce::Gnmx::parseShader( &esShaderInfo, shaderInitData );

				// Copy shader code to Garlic memory
				void* shaderBinaryGarlic  = GPU_API_ALLOCATE( GpuMemoryPool_Shaders, MC_ShaderCode, esShaderInfo.m_gpuShaderCodeSize, sce::Gnm::kAlignmentOfShaderInBytes );
				Red::System::MemoryCopy( shaderBinaryGarlic, esShaderInfo.m_gpuShaderCode, esShaderInfo.m_gpuShaderCodeSize );

				// Store the header pointer in the shader data.
				data.m_esShader = ExtractShaderHeader( esShaderInfo.m_esShader, shaderBinaryGarlic );
				
				SetupScratchPad( data, data.m_esShader );
			}
			break;
		case sce::Gnmx::kGeometryShader: 
			{
				// It is a geometry shader, and it is bound to TWO shader stages at the same time (GS and VS)
				// ==========================================================================================
				GPUAPI_ASSERT( shaderType == GeometryShader );
				GPUAPI_ASSERT( psslType == sce::Shader::Binary::kShaderTypeGsShader );

				// Parse shader binary.
				sce::Gnmx::ShaderInfo gsShaderInfo;
				sce::Gnmx::ShaderInfo vsShaderInfo;
				sce::Gnmx::parseGsShader( &gsShaderInfo, &vsShaderInfo, shaderInitData );

				// Copy shader code to Garlic memory
				void* shaderBinaryGarlicGS  = GPU_API_ALLOCATE( GpuMemoryPool_Shaders, MC_ShaderCode, gsShaderInfo.m_gpuShaderCodeSize, sce::Gnm::kAlignmentOfShaderInBytes );
				void* shaderBinaryGarlicVS  = GPU_API_ALLOCATE( GpuMemoryPool_Shaders, MC_ShaderCode, vsShaderInfo.m_gpuShaderCodeSize, sce::Gnm::kAlignmentOfShaderInBytes );
				Red::System::MemoryCopy( shaderBinaryGarlicGS, gsShaderInfo.m_gpuShaderCode, gsShaderInfo.m_gpuShaderCodeSize );
				Red::System::MemoryCopy( shaderBinaryGarlicVS, vsShaderInfo.m_gpuShaderCode, vsShaderInfo.m_gpuShaderCodeSize );

				// Copy shader header to onion memory
				Uint32 gsHeaderSize = gsShaderInfo.m_gsShader->computeSize();
				void* shaderHeader = GPU_API_ALLOCATE( GpuMemoryPool_SmallOnion, MC_ShaderHeaders, gsHeaderSize, sce::Gnm::kAlignmentOfBufferInBytes );
				Red::System::MemoryCopy( shaderHeader, gsShaderInfo.m_gsShader, gsHeaderSize );

				// Store GPU shader address in the header
				sce::Gnmx::GsShader* typedHeader = static_cast< sce::Gnmx::GsShader* >( shaderHeader );
				typedHeader->patchShaderGpuAddresses( shaderBinaryGarlicGS, shaderBinaryGarlicVS );

				// Store the header pointer in the shader data.
				data.m_gsShader = typedHeader;

				SetupScratchPad( data, data.m_gsShader );
			}
			break;
		case sce::Gnmx::kComputeShader: 
			{
				// It is a compute shader
				// ======================
				GPUAPI_ASSERT( shaderType == ComputeShader );
				GPUAPI_ASSERT( psslType == sce::Shader::Binary::kShaderTypeCsShader );

				// Parse shader binary.
				sce::Gnmx::ShaderInfo csShaderInfo;
				sce::Gnmx::parseShader( &csShaderInfo, shaderInitData );

				// Copy shader code to Garlic memory
				void* shaderBinaryGarlic  = GPU_API_ALLOCATE( GpuMemoryPool_Shaders, MC_ShaderCode, csShaderInfo.m_gpuShaderCodeSize, sce::Gnm::kAlignmentOfShaderInBytes );
				Red::System::MemoryCopy( shaderBinaryGarlic, csShaderInfo.m_gpuShaderCode, csShaderInfo.m_gpuShaderCodeSize );

				// Store the header pointer in the shader data.
				data.m_csShader = ExtractShaderHeader( csShaderInfo.m_csShader, shaderBinaryGarlic );

				SetupScratchPad( data, data.m_csShader );
			}
			break;
		case sce::Gnmx::kPixelShader: 
			{
				// It is a pixel shader
				// ========================================================================================================
				GPUAPI_ASSERT( shaderType == PixelShader );
				GPUAPI_ASSERT( psslType == sce::Shader::Binary::kShaderTypeFsShader );

				// Parse shader binary.
				sce::Gnmx::ShaderInfo psShaderInfo;
				sce::Gnmx::parseShader( &psShaderInfo, shaderInitData );

				// Copy shader code to Garlic memory
				void* shaderBinaryGarlic  = GPU_API_ALLOCATE( GpuMemoryPool_Shaders, MC_ShaderCode, psShaderInfo.m_gpuShaderCodeSize, sce::Gnm::kAlignmentOfShaderInBytes );
				Red::System::MemoryCopy( shaderBinaryGarlic, psShaderInfo.m_gpuShaderCode, psShaderInfo.m_gpuShaderCodeSize );

				// Store the header pointer in the shader data.
				data.m_psShader = ExtractShaderHeader( psShaderInfo.m_psShader, shaderBinaryGarlic );

				SetupScratchPad( data, data.m_psShader );
			}
			break;
		case sce::Gnmx::kComputeVertexShader:
			GPUAPI_HALT( "CSVS NOT IMPLEMENTED" );
			break;

		default: GPUAPI_HALT( "Unknown shader type." );
		}
		
		// Store the hardware stage type
		data.m_stageType = shaderStageType;

		// build InputResourceOffsets table
#if defined(SCE_GNMX_ENABLE_GFX_LCUE)
		sce::Gnmx::generateInputResourceOffsetTable( &data.m_resourceOffsets, MapShaderTypeToShaderStage(shaderStageType), data.m_shaderStruct );
#endif

		// enumerate required samplers
		Uint32 samplerStates = 0;
		for (Uint32 i = 0; i < data.m_program.m_numSamplerStates; ++i)
		{
			Uint32 slot = data.m_program.m_samplerStates[i].m_resourceIndex;
			GPUAPI_ASSERT (slot < MAX_PS_SAMPLER_STATES, TXT("Shader contains a sampler with index(%d) > MAX_PS_SAMPLER_STATES"), slot);
			if (slot < MAX_PS_SAMPLER_STATES)
			{
				samplerStates |= (1 << slot);
			}
		}
		data.m_samplerMask = samplerStates;

		Uint32 constantBufferMask = 0;
		Uint32 resourceMask = 0;
		Uint32 textureMask = 0;
		for (Uint32 i = 0; i < data.m_program.m_numBuffers; ++i)
		{
			const sce::Shader::Binary::Buffer& buffer = data.m_program.m_buffers[i];
			if (buffer.m_internalType == sce::Shader::Binary::kInternalBufferTypeCbuffer)
			{
				Uint32 slot = buffer.m_resourceIndex;
				GPUAPI_ASSERT (slot < MAX_CONSTANT_BUFFERS, TXT("Shader contains a CB with index(%d) > MAX_CONSTANT_BUFFERS"), slot);
				if (slot < MAX_CONSTANT_BUFFERS)
				{
					constantBufferMask |= (1 << slot);
				}
			}
			else if (buffer.m_internalType == sce::Shader::Binary::kInternalBufferTypeSrv)
			{
				Uint32 slot = buffer.m_resourceIndex;
				GPUAPI_ASSERT (slot < MAX_PS_SAMPLERS, TXT("Shader contains a CB with index(%d) > MAX_PS_SAMPLERS"), slot);
				if (slot < MAX_PS_SAMPLERS)
				{
					resourceMask |= (1 << slot);
				}
			}
			else if (buffer.m_internalType == sce::Shader::Binary::kInternalBufferTypeTextureSampler)
			{
				Uint32 slot = buffer.m_resourceIndex;
				GPUAPI_ASSERT (slot < MAX_PS_SAMPLERS, TXT("Shader contains a Texture with index(%d) > MAX_PS_SAMPLERS"), slot);
				if (slot < MAX_PS_SAMPLERS)
				{
					textureMask |= (1 << slot);
				}
			}
		}
		data.m_constantBufferMask = constantBufferMask;
		data.m_resourceMask = resourceMask;
		data.m_textureMask = textureMask;

		ShaderRef ret(newShaderId);

		if (debugName != nullptr)
		{
			SetShaderDebugPath( ret, debugName );
		}

		return ret;
	}

#define DEFINE_SHADER_REMOVER( stage, registers, addrlo, addrhi )							\
	void Delete##stage##Shader( sce::Gnmx::	stage##Shader *shader )							\
	{																						\
		if( shader )																		\
		{																					\
			void *binary = reinterpret_cast<void*>(											\
			(static_cast<uint64_t>(shader->registers.addrhi) << 40) |						\
			(static_cast<uint64_t>(shader->registers.addrlo) << 8) );						\
			GPU_API_FREE( GpuMemoryPool_Shaders, MC_ShaderCode, binary );					\
			GPU_API_FREE( GpuMemoryPool_SmallOnion, MC_ShaderHeaders, shader );			\
		}																					\
	}

	DEFINE_SHADER_REMOVER(Vs, m_vsStageRegisters, m_spiShaderPgmLoVs, m_spiShaderPgmHiVs)
	DEFINE_SHADER_REMOVER(Ps, m_psStageRegisters, m_spiShaderPgmLoPs, m_spiShaderPgmHiPs)
	DEFINE_SHADER_REMOVER(Es, m_esStageRegisters, m_spiShaderPgmLoEs, m_spiShaderPgmHiEs)
	DEFINE_SHADER_REMOVER(Ls, m_lsStageRegisters, m_spiShaderPgmLoLs, m_spiShaderPgmHiLs)
	DEFINE_SHADER_REMOVER(Hs, m_hsStageRegisters, m_spiShaderPgmLoHs, m_spiShaderPgmHiHs)
	DEFINE_SHADER_REMOVER(Gs, m_gsStageRegisters, m_spiShaderPgmLoGs, m_spiShaderPgmHiGs)
	DEFINE_SHADER_REMOVER(Cs, m_csStageRegisters, m_computePgmLo, m_computePgmHi)

	void DeleteShaderInternal( SDeviceData &dd, SShaderData &data )
	{
		switch( data.m_stageType )
		{
		case sce::Gnmx::kVertexShader:					DeleteVsShader( data.m_vsShader ); break;	// TODO: free vs fetch shader memory
		case sce::Gnmx::kPixelShader:					DeletePsShader( data.m_psShader ); break;
		case sce::Gnmx::kHullShader:					DeleteHsShader( data.m_hsShader ); break;
		case sce::Gnmx::kExportShader:					DeleteEsShader( data.m_esShader ); break;	// TODO: free es fetch shader memory
		case sce::Gnmx::kGeometryShader:				DeleteGsShader( data.m_gsShader ); break;
		case sce::Gnmx::kComputeShader:					DeleteCsShader( data.m_csShader ); break;
		case sce::Gnmx::kLocalShader:					DeleteLsShader( data.m_lsShader ); break;	// TODO: free ls fetch shader memory
		case sce::Gnmx::kComputeVertexShader:			GPUAPI_HALT( "NOT IMPLEMENTED" ); break;
		default:  GPUAPI_HALT( "Trying to delete a shader with invalid stage type!!!" );
		}

		data.m_shaderStruct = nullptr;
	}

	eShaderLanguage GetShaderLanguage()
	{
		return SL_PSSL;
	}

	Uint32 GetShaderCodeSize( const ShaderRef& shader )
	{
		if ( !shader.isNull() )
		{
			GPUAPI_HALT("NOT IMPLEMENTED");
		}

		return 0;
	}

	void CopyShaderCode( const ShaderRef& shader, void* targetCode )
	{
		GPUAPI_HALT("NOT IMPLEMENTED");	// Won't be needed anyway I guess
	}

	void SetShader( const ShaderRef& shader, eShaderType shaderType )
	{
		SDeviceData &dd = GetDeviceData();

		if ( dd.m_shadersSet[ shaderType ] != shader )
		{
			GPUAPI_ASSERT( shader.isNull() || GetDeviceData().m_Shaders.Data(shader).m_type == shaderType );
			
			dd.m_shadersSet[shaderType] = shader;
			dd.m_shadersChangedMask |= ( 1 << shaderType );
		}
	}
}
