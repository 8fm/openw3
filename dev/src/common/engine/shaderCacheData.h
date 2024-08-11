#pragma once

#include "materialCompiler.h"
#include "../../common/core/compression/zlib.h"

enum ECookingPlatform : Int32;

#define SHADER_CACHE_MAGIC						'SHDR'
#define SHADER_CACHE_VERSION_BASE				2
#define SHADER_CACHE_VERSION_COMPRESSED_ENTRIES	3
#define SHADER_CACHE_VERSION_CURRENT			SHADER_CACHE_VERSION_COMPRESSED_ENTRIES

#ifdef RED_FINAL_BUILD
	#define RED_OPTIMIZED_MATERIAL_ENTRY
#endif

/// Shader cache entry
struct MaterialEntry
{
#ifdef RED_OPTIMIZED_MATERIAL_ENTRY
	DECLARE_STRUCT_MEMORY_POOL( MemoryPool_SmallObjects, MC_MaterialCacheEntry );
#else
	DECLARE_STRUCT_MEMORY_POOL( MemoryPool_Default, MC_MaterialCacheEntry );
#endif

	typedef TDynArray< SamplerStateInfo, MC_MaterialSamplerStates > TSamplerStateInfoArray;

	Uint64						m_hash;
#ifndef RED_OPTIMIZED_MATERIAL_ENTRY
	Uint64						m_crc;
	Uint64						m_includesCRC;
	Uint64						m_shaderHashes[ GpuApi::ShaderTypeMax ];
#else
	Uint64						m_shaderHashVS;
	Uint64						m_shaderHashPS;
	Uint64						m_shaderHashDS;
	Uint64						m_shaderHashHS;
#endif


	TSamplerStateInfoArray		m_vsSamplerStates; // vertex shader
	TMaterialUsedParameterArray	m_vsUsedParameters; // vertex shader

	TSamplerStateInfoArray		m_psSamplerStates; // pixel shader
	TMaterialUsedParameterArray	m_psUsedParameters; // pixel shader


#ifndef RED_OPTIMIZED_MATERIAL_ENTRY
	String						m_path;
	Uint32						m_contextId;
#endif

	RED_INLINE MaterialEntry()
		: m_hash( 0 )
#ifndef RED_OPTIMIZED_MATERIAL_ENTRY
		, m_crc( 0 )
		, m_includesCRC( 0 )
#endif
	{
	}

	RED_INLINE void Serialize( IFile& file )
	{
		file << m_hash;
#ifndef RED_OPTIMIZED_MATERIAL_ENTRY
		file << m_crc;
		file << m_includesCRC;
#else
		Uint64						crc;
		Uint64						includesCRC;
		file << crc;
		file << includesCRC;
#endif

		String skipString;		// This is just needed to resolve char type. Keep it the same type as m_path
#ifndef RED_OPTIMIZED_MATERIAL_ENTRY
		static_assert( std::is_same< decltype( skipString ), decltype( m_path ) >::value, "Skip string types must match or serialisation will break!" );
		// Note! we keep these strings as a debug measure, but don't actually need them during runtime
		if( !GIsCooker )
		{
			StringHelpers::SkipSerialisation( file, skipString );
		}
		else
		{
			file << m_path;
		}
		file << m_contextId;
#else
		StringHelpers::SkipSerialisation( file, skipString );
		Uint32 contextId;
		file << contextId;
#endif


#ifndef RED_OPTIMIZED_MATERIAL_ENTRY
		for ( Uint32 i = 0; i < GpuApi::ShaderTypeMax; ++i )
		{
			GpuApi::eShaderType shaderType = (GpuApi::eShaderType)i;
			file << m_shaderHashes[ shaderType ];
		}
#else
		Uint64 shaderHashGS;
		Uint64 shaderHashCS;
		file << m_shaderHashVS;
		file << m_shaderHashPS;
		file << shaderHashGS;
		file << m_shaderHashHS;
		file << m_shaderHashDS;
		file << shaderHashCS;
#endif

		if ( file.IsReader() )
		{
			Uint32 vsSamplerStatesSize = 0;
			file << vsSamplerStatesSize;
			m_vsSamplerStates.Resize( vsSamplerStatesSize );
			for ( Uint32 i = 0; i < vsSamplerStatesSize; ++i )
			{
				m_vsSamplerStates[i].Serialize( file );
			}
			Uint32 psSamplerStatesSize = 0;
			file << psSamplerStatesSize;
			m_psSamplerStates.Resize( psSamplerStatesSize );
			for ( Uint32 i = 0; i < psSamplerStatesSize; ++i )
			{
				m_psSamplerStates[i].Serialize( file );
			}

			Uint32 vsParametersSize = 0;
			file << vsParametersSize;
			m_vsUsedParameters.Resize( vsParametersSize );
			for ( Uint32 i = 0; i < vsParametersSize; ++i )
			{
				file << m_vsUsedParameters[i].m_name;
				file << m_vsUsedParameters[i].m_register;
			}
			Uint32 psParametersSize = 0;
			file << psParametersSize;
			m_psUsedParameters.Resize( psParametersSize );
			for ( Uint32 i = 0; i < psParametersSize; ++i )
			{
				file << m_psUsedParameters[i].m_name;
				file << m_psUsedParameters[i].m_register;
			}
		}
		else if ( file.IsWriter() )
		{
			Uint32 vsSize = m_vsSamplerStates.Size();
			file << vsSize;
			for ( Uint32 i = 0; i < vsSize; ++i )
			{
				m_vsSamplerStates[i].Serialize( file );
			}
			Uint32 psSize = m_psSamplerStates.Size();
			file << psSize;
			for ( Uint32 i = 0; i < psSize; ++i )
			{
				m_psSamplerStates[i].Serialize( file );
			}

			Uint32 vsParametersSize = m_vsUsedParameters.Size();
			file << vsParametersSize;
			for ( Uint32 i = 0; i < vsParametersSize; ++i )
			{
				file << m_vsUsedParameters[i].m_name;
				file << m_vsUsedParameters[i].m_register;
			}
			Uint32 psParametersSize = m_psUsedParameters.Size();
			file << psParametersSize;
			for ( Uint32 i = 0; i < psParametersSize; ++i )
			{
				file << m_psUsedParameters[i].m_name;
				file << m_psUsedParameters[i].m_register;
			}
		}
	}
	
	RED_INLINE Bool SetShader( GpuApi::eShaderType shaderType, Uint64 hash )
	{
#ifndef RED_OPTIMIZED_MATERIAL_ENTRY
		ASSERT( shaderType < GpuApi::ShaderTypeMax );
		m_shaderHashes[ shaderType ] = hash;
#endif
		return true;
	}

	RED_INLINE Uint64 GetShaderHash( GpuApi::eShaderType shaderType ) const
	{
#ifndef RED_OPTIMIZED_MATERIAL_ENTRY
		return m_shaderHashes[ shaderType ];
#else
		switch (shaderType)
		{
		case GpuApi::VertexShader:
			return m_shaderHashVS;
		case GpuApi::PixelShader:
			return m_shaderHashPS;
		case GpuApi::DomainShader:
			return m_shaderHashDS;
		case GpuApi::HullShader:
			return m_shaderHashHS;
		default:
			RED_HALT("ShaderType not supported");
			return 0;
		}
#endif
	}
};

//////////////////////////////////////////////////////////////////////////

struct ShaderEntry
{
	DECLARE_STRUCT_MEMORY_POOL( MemoryPool_SmallObjects, MC_ShaderCacheEntry );

	static Double		s_compressionTime;
	static Double		s_decompressionTime;

	Uint64				m_hash;
	Uint32				m_uncompressedSize;
	Bool				m_isCompressed;
	DataBuffer			m_data;

	RED_INLINE ShaderEntry()
		: m_hash( 0 )
		, m_data( TDataBufferAllocator< MC_ShaderCacheEntry >::GetInstance() )
		, m_isCompressed( false )
		, m_uncompressedSize( 0 )
	{
	}

	RED_INLINE void SetData( const DataBuffer& data )
	{
		// provided data buffer contains uncompressed data - try to compress it
		Compress( data );
	}

	RED_INLINE void Serialize( IFile& file )
	{
		file << m_hash;

		if ( file.GetVersion() >= SHADER_CACHE_VERSION_COMPRESSED_ENTRIES )
		{
			file << m_isCompressed;
			file << m_uncompressedSize;
		}

		if ( file.IsReader() )
		{
			if ( file.GetVersion() >= SHADER_CACHE_VERSION_COMPRESSED_ENTRIES && m_isCompressed )
			{
				m_data.Serialize( file );
			}
			else
			{
				DataBuffer tempBuffer;
				tempBuffer.Serialize( file );

				Compress( tempBuffer );
			}
		}
		else
		{
			m_data.Serialize( file );
		}
	}

	RED_INLINE Bool DecompressToBuffer( DataBuffer& data )
	{
		CTimeCounter timer;
		if ( m_isCompressed )
		{
			RED_ASSERT( m_uncompressedSize > 0, TXT("Compressed shader has 0 uncompressed size, will be created from source.") );
			if ( m_uncompressedSize == 0 )
			{
				s_decompressionTime += timer.GetTimePeriodMS();
				return false;
			}

			data.Allocate( m_uncompressedSize );
			Red::Core::Decompressor::CZLib decompressor;
			if ( Red::Core::Decompressor::EStatus::Status_Success != decompressor.Initialize( m_data.GetData(), data.GetData(), m_data.GetSize(), m_uncompressedSize ) )
			{
				s_decompressionTime += timer.GetTimePeriodMS();
				return false;
			}
			if ( Red::Core::Decompressor::EStatus::Status_Success != decompressor.Decompress() )
			{
				s_decompressionTime += timer.GetTimePeriodMS();
				return false;
			}
		}
		else
		{
			data = m_data;
		}

		s_decompressionTime += timer.GetTimePeriodMS();
		return true;
	}

private:
	RED_INLINE void Compress( const DataBuffer& data )
	{
		CTimeCounter timer;
		m_uncompressedSize = data.GetSize();

		Red::Core::Compressor::CZLib compressor;
		RED_VERIFY( compressor.Compress( data.GetData(), data.GetSize() ), TXT( "Could not compress" ) );

		if ( compressor.GetResultSize() >= data.GetSize() )
		{
			m_data = data;
			m_isCompressed = false;
			s_compressionTime += timer.GetTimePeriodMS();
			return;
		}

		m_isCompressed = true;
		m_data.Clear();
		m_data.Allocate( compressor.GetResultSize() );
		Red::System::MemoryCopy( m_data.GetData(), compressor.GetResult(), compressor.GetResultSize() );
		s_compressionTime += timer.GetTimePeriodMS();
	}
};

//////////////////////////////////////////////////////////////////////////

struct StaticShaderEntry
{
	DECLARE_STRUCT_MEMORY_POOL( MemoryPool_SmallObjects, MC_StaticShaderCacheEntry );

	Uint64			m_hash;
	Uint64			m_contentCRC;
	DataBuffer		m_data;

	RED_INLINE StaticShaderEntry()
		: m_hash( 0 )
		, m_contentCRC( 0 )
		, m_data( TDataBufferAllocator< MC_StaticShaderCacheEntry >::GetInstance() )
	{
	}

	RED_INLINE void Serialize( IFile& file )
	{
		file << m_hash;
		file << m_contentCRC;
		m_data.Serialize( file );
	}
};
