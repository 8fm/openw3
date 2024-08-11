/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "renderShader.h"
#include "../core/depot.h"
#include "../core/dependencyMapper.h"
#include "../engine/shaderCacheManager.h"
#include "../engine/materialCompilerDefines.h"
#include "../gpuApiUtils/gpuApiShaderParsing.h"

#define SHADERS_PATH_TYPE	FOF_AbsolutePath

CRenderShader::CRenderShader( ERenderShaderType type, GpuApi::ShaderRef shader, Uint64 hash )
	: m_type( type )
	, m_shader( shader )
	, m_hash( hash )
{
}

CRenderShader::~CRenderShader()
{
	{
		Red::Threads::CScopedLock< Red::Threads::CMutex > lock( CRenderShaderMap::s_mutex );
		RED_VERIFY( GRenderShaderMap->Erase( m_hash ) );
	}

	GpuApi::SafeRelease( m_shader );
}

CName CRenderShader::GetCategory() const
{
	switch (m_type)
	{
	case RST_VertexShader:
		return CNAME( RenderVertexShader );
	case RST_PixelShader:
		return CNAME( RenderPixelShader );
	case RST_GeometryShader:
		return CNAME( RenderGeometryShader );
	case RST_HullShader:
		return CNAME( RenderHullShader );
	case RST_DomainShader:
		return CNAME( RenderDomainShader );
	case RST_ComputeShader:
		return CNAME( RenderComputeShader );
	default:
		return CName::NONE;
	}
}

void CRenderShader::Bind()
{
	GetRenderer()->GetStateManager().SetShader( m_shader, m_type );
}

void CRenderShader::UnBind()
{
	GetRenderer()->GetStateManager().SetShader( GpuApi::ShaderRef::Null(), m_type );
}

CRenderShader* CRenderShader::Create( ERenderShaderType type, const AnsiChar* code, const CMaterialCompilerDefines& definesList, const String& fileName, Uint64& hash, Bool isStatic /*=false*/ )
{
	GpuApi::eShaderType shaderType = Map( type );

	// Setup defines
	TDynArray< GpuApi::ShaderDefine > defines;
	MapDefines( type, definesList, defines );

	const AnsiChar* shaderTarget = nullptr;
	const AnsiChar* entryPoint = nullptr;
	if ( !GpuApi::GetShaderTargetAndEntryPoint( shaderType, shaderTarget, entryPoint ) )
	{
		return nullptr;
	}

	Uint64 filenameHash = 0;
	if ( GShaderCache )
	{
		CRenderShader* cachedShader = nullptr;
		if ( CreateFromCache( fileName, code, defines, type, entryPoint, isStatic, false, cachedShader, filenameHash, hash ) )
		{
			return cachedShader;
		}
	}
	else
	{
		filenameHash = GpuApi::GetFilenameHash( UNICODE_TO_ANSI( fileName.AsChar() ), defines.TypedData(), defines.Size(), shaderType );
		if ( !GpuApi::GetShaderHash( hash, code, entryPoint, defines.TypedData(), defines.Size(), UNICODE_TO_ANSI( fileName.AsChar() ) ) )
		{
			return nullptr;
		}
	}

	GpuApi::ShaderRef shader = GpuApi::CreateShaderFromSource( shaderType, code, entryPoint, shaderTarget, defines.TypedData(), defines.Size(), UNICODE_TO_ANSI(fileName.AsChar()) );
	if ( shader.isNull() )
	{
		return nullptr;
	}

	if ( GShaderCache )
	{
		DataBuffer shaderCode( TDataBufferAllocator< MC_Temporary >::GetInstance() );
		Uint32 shaderSize = 0;

		shaderSize = GpuApi::GetShaderCodeSize( shader );
		shaderCode.Allocate( shaderSize );
		GpuApi::CopyShaderCode( shader, shaderCode.GetData() );

		if ( isStatic )
		{
			GShaderCache->AddStaticShader( filenameHash, hash, shaderCode );
		}
		else
		{
			GShaderCache->AddShader( hash, shaderCode );
		}
		GShaderCache->Flush();
	}

	{
		Red::Threads::CScopedLock< Red::Threads::CMutex > lock( CRenderShaderMap::s_mutex );

		CRenderShader* renderShader = nullptr;
		if ( GRenderShaderMap->Find( isStatic ? filenameHash : hash, renderShader ) )
		{
			if ( renderShader )
			{
				renderShader->AddRef();
			}
			GpuApi::SafeRelease( shader );
			return renderShader;
		}

		RED_LOG_SPAM( RED_LOG_CHANNEL(Shaders), TXT("Shader '%ls' compiled"), fileName.AsChar() );
		renderShader = new CRenderShader( type, shader, isStatic ? filenameHash : hash );

		if ( renderShader )
		{
			GRenderShaderMap->Insert( isStatic ? filenameHash : hash, renderShader );
		}

		return renderShader;
	}
}

CRenderShader* CRenderShader::Create( ERenderShaderType type, const String& fileName, const CMaterialCompilerDefines& defines )
{
	if ( GShaderCache )
	{
		// try cached version first
		if ( GShaderCache->StaticShadersMatch() )
		{
			CRenderShader* cachedShader = nullptr;
			TDynArray< GpuApi::ShaderDefine > shaderDefines;
			MapDefines( type, defines, shaderDefines );
			if ( CreateStaticFromCache( fileName, shaderDefines, type, false, cachedShader ) )
			{
				return cachedShader;
			}
		}
		
		// make sure we're not running cooked game
		RED_ASSERT( !GShaderCache->IsReadonly(), TXT("Shader not found in the shader cache while running cooked game") );
	}

	// Find the file
	String absoluteFilePath = String( GpuApi::GetShaderRootPath() ) + fileName;
	IFile* file = GFileManager->CreateFileReader( absoluteFilePath, SHADERS_PATH_TYPE );
	if ( !file )
	{
		HALT( "Missing shader file: '%ls'", fileName.AsChar() );
		return nullptr;
	}

	// Load code
	size_t fileSize = static_cast< size_t >( file->GetSize() );
	RED_ASSERT( (Uint64)fileSize == file->GetSize(), TXT("Unexpectedly large file '%ls'"), file->GetFileNameForDebug() );
	AnsiChar* code = reinterpret_cast< AnsiChar* >( RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_Temporary , sizeof(AnsiChar) * (fileSize + 1)) );
	file->Serialize( code, fileSize );
	code[ fileSize ] = 0;
	delete file;

	// Assemble shader
	Uint64 hash = 0;
	CRenderShader* shader = CRenderShader::Create( type, code, defines, fileName, hash, true );
	RED_MEMORY_FREE( MemoryPool_Default, MC_Temporary, code );

	return shader;
}

CRenderShader* CRenderShader::Create( ERenderShaderType type, const DataBuffer& shaderData, const Uint64 hash, const String& fileName )
{
	RED_ASSERT( shaderData.GetSize() );

	// Get memory size
	const GpuApi::Uint32 memorySize = static_cast< GpuApi::Uint32 >( shaderData.GetSize() );

	// Create shader

	GpuApi::ShaderRef shader = GpuApi::CreateShaderFromBinary( Map(type), shaderData.GetData(), memorySize, UNICODE_TO_ANSI(fileName.AsChar()) );
	if ( shader.isNull() )
	{
		return nullptr;
	}

	// Create wrapper
	return new CRenderShader( type, shader, hash );
}

CRenderShader* CRenderShader::CreateGSwithSO( const AnsiChar* code, const CMaterialCompilerDefines& definesList, const String& fileName, const GpuApi::VertexLayoutDesc& outputDesc, GpuApi::VertexLayoutDesc* adjustedDesc /*nullptr*/, Bool isStatic /*=false*/ )
{
	GpuApi::eShaderType shaderType = GpuApi::GeometryShader;

	// Setup defines
	TDynArray< GpuApi::ShaderDefine > defines;
	MapDefines( RST_GeometryShader, definesList, defines );

	const AnsiChar* entryPoint = nullptr;
	const AnsiChar* shaderTarget = nullptr;
	if ( !GetShaderTargetAndEntryPoint( shaderType, shaderTarget, entryPoint ) )
	{
		return nullptr;
	}

	Uint64 filenameHash = 0;
	Uint64 hash = 0;
	if ( GShaderCache )
	{
		CRenderShader* cachedShader = nullptr;
		if ( CreateFromCache( fileName, code, defines, RST_GeometryShader, entryPoint, isStatic, true, cachedShader, filenameHash, hash, &outputDesc, adjustedDesc ) )
		{
			return cachedShader;
		}
	}
	else
	{
		// calculate hashes
		filenameHash = GpuApi::GetFilenameHash( UNICODE_TO_ANSI( fileName.AsChar() ), defines.TypedData(), defines.Size(), shaderType );
		if ( !GpuApi::GetShaderHash( hash, code, entryPoint, defines.TypedData(), defines.Size(), UNICODE_TO_ANSI( fileName.AsChar() ) ) )
		{
			return nullptr;
		}
	}

	GpuApi::ShaderRef shader = GpuApi::CreateGeometryShaderWithSOFromSource( code, entryPoint, shaderTarget, defines.TypedData(), defines.Size(), UNICODE_TO_ANSI(fileName.AsChar()), outputDesc, adjustedDesc );
	if ( shader.isNull() )
	{
		// error
		return nullptr;
	}

	if ( GShaderCache )
	{
		DataBuffer shaderCode( TDataBufferAllocator< MC_Temporary >::GetInstance() );

		Uint32 shaderSize = GpuApi::GetShaderCodeSize( shader );
		shaderCode.Allocate( shaderSize );
		GpuApi::CopyShaderCode( shader, shaderCode.GetData() );

		if ( isStatic )
		{
			GShaderCache->AddStaticShader( filenameHash, hash, shaderCode );
		}
		else
		{
			GShaderCache->AddShader( hash, shaderCode );
		}
		GShaderCache->Flush();
	}

	{
		Red::Threads::CScopedLock< Red::Threads::CMutex > lock( CRenderShaderMap::s_mutex );

		CRenderShader* renderShader = nullptr;		
		if ( GRenderShaderMap->Find( isStatic ? filenameHash : hash, renderShader ) )
		{
			if ( renderShader )
			{
				renderShader->AddRef();
			}
			GpuApi::SafeRelease( shader );
			return renderShader;
		}

		RED_LOG_SPAM( RED_LOG_CHANNEL(Shaders), TXT("Shader '%ls' compiled"), fileName.AsChar() );
		renderShader = new CRenderShader( RST_GeometryShader, shader, isStatic ? filenameHash : hash );

		if ( renderShader )
		{
			GRenderShaderMap->Insert( isStatic ? filenameHash : hash, renderShader );
		}

		return renderShader;
	}
}

CRenderShader* CRenderShader::CreateGSwithSO( const DataBuffer& shaderData, const Uint64 hash, const GpuApi::VertexLayoutDesc& outputDesc, GpuApi::VertexLayoutDesc* adjustedDesc /*nullptr*/ )
{
	RED_ASSERT( shaderData.GetSize() );

	const GpuApi::Uint32 memorySize = static_cast< GpuApi::Uint32 >( shaderData.GetSize() );
	GpuApi::ShaderRef shader = GpuApi::CreateGeometryShaderWithSOFromBinary( shaderData.GetData(), memorySize, outputDesc, adjustedDesc );

	// Create wrapper
	return new CRenderShader( RST_GeometryShader, shader, hash );
}

CRenderShader* CRenderShader::CreateGSwithSO( const String& fileName, const CMaterialCompilerDefines& definesList, const GpuApi::VertexLayoutDesc& outputDesc, GpuApi::VertexLayoutDesc* adjustedDesc )
{
	// try cached first
	if ( GShaderCache )
	{
		if ( GShaderCache->StaticShadersMatch() )
		{
			CRenderShader* cachedShader = nullptr;
			TDynArray< GpuApi::ShaderDefine > shaderDefines;
			MapDefines( RST_GeometryShader, definesList, shaderDefines );
			if ( CreateStaticFromCache( fileName, shaderDefines, RST_GeometryShader, true, cachedShader, &outputDesc, adjustedDesc ) )
			{
				return cachedShader;
			}
		}
		
		RED_ASSERT( !GShaderCache->IsReadonly() );
	}	

	// Find the file
	String absoluteFilePath = String( GpuApi::GetShaderRootPath() ) + fileName;
	IFile* file = GFileManager->CreateFileReader( absoluteFilePath, SHADERS_PATH_TYPE );
	if ( !file )
	{
		HALT( "Missing shader file: '%ls'", fileName.AsChar() );
		return nullptr;
	}

	// Load code
	size_t fileSize = static_cast< size_t >( file->GetSize() );
	RED_ASSERT( (Uint64)fileSize == file->GetSize(), TXT("Unexpectedly large file '%ls'"), file->GetFileNameForDebug() );
	AnsiChar* code = reinterpret_cast< AnsiChar* >( RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_Temporary , sizeof(AnsiChar) * (fileSize + 1)) );
	file->Serialize( code, fileSize );
	code[ fileSize ] = 0;
	delete file;

	// Assemble shader
	CRenderShader* shader = CreateGSwithSO( code, definesList, fileName, outputDesc, adjustedDesc, true );
	RED_MEMORY_FREE( MemoryPool_Default, MC_Temporary, code );

	return shader;
}

Bool CRenderShader::CreateFromCache( const String& fileName, const AnsiChar* code, const TDynArray< GpuApi::ShaderDefine >& defines, ERenderShaderType type, const AnsiChar* entryPoint, Bool isStatic, Bool hasStreamOut, CRenderShader*& outShader, Uint64& filenameHash, Uint64& contentHash, const GpuApi::VertexLayoutDesc* outputDesc /*=nullptr*/, GpuApi::VertexLayoutDesc* adjustedDesc /*nullptr*/ )
{
	GpuApi::eShaderType shaderType = Map( type );

	filenameHash = GpuApi::GetFilenameHash( UNICODE_TO_ANSI( fileName.AsChar() ), defines.TypedData(), defines.Size(), shaderType );
	DataBuffer data;
	if ( isStatic )
	{
		StaticShaderEntry* entry = nullptr;
		if ( GShaderCache->GetStaticShader( filenameHash, entry ) && entry )
		{
			data = entry->m_data;
			if ( !GShaderCache->StaticShadersMatch() )
			{
				RED_VERIFY( GpuApi::GetShaderHash( contentHash, code, entryPoint, defines.TypedData(), defines.Size(), UNICODE_TO_ANSI( fileName.AsChar() ) ) );
				if ( entry->m_contentCRC != contentHash )
				{
					data.Clear();
				}
			}
		}
		else
		{
			RED_VERIFY( GpuApi::GetShaderHash( contentHash, code, entryPoint, defines.TypedData(), defines.Size(), UNICODE_TO_ANSI( fileName.AsChar() ) ) );
		}
	}
	else
	{
		RED_VERIFY( GpuApi::GetShaderHash( contentHash, code, entryPoint, defines.TypedData(), defines.Size(), UNICODE_TO_ANSI( fileName.AsChar() ) ) );
		IShaderCache::EResult getShaderResult = GShaderCache->GetShaderData( contentHash, data );
		if ( getShaderResult != IShaderCache::eResult_Valid )
		{
			data.Clear();
		}
	}

	{
		Red::Threads::CScopedLock< Red::Threads::CMutex > lock( CRenderShaderMap::s_mutex );
		if ( GRenderShaderMap->Find( isStatic ? filenameHash : contentHash, outShader ) )
		{
			if ( outShader )
			{
				outShader->AddRef();
			}
			return outShader != nullptr;
		}

		if ( data.GetSize() > 0 )
		{
			if ( hasStreamOut )
			{
				RED_ASSERT( outputDesc );
				outShader = CreateGSwithSO( data, isStatic ? filenameHash : contentHash, *outputDesc, adjustedDesc );
			}
			else
			{
				outShader = Create( type, data, isStatic ? filenameHash : contentHash, fileName );
			}
		}
		else
		{
			String defString = String::EMPTY;
			for ( Uint32 i = 0; i < defines.Size(); ++i )
			{
				defString += String::Printf(TXT(" '%ls'='%ls';"), ANSI_TO_UNICODE( defines[ i ].Name ), ANSI_TO_UNICODE( defines[ i ].Definition ) );
			}
			RED_LOG( RED_LOG_CHANNEL( Shaders ), TXT("File '%ls' (static: %d) defines: '%ls', hash: %") RED_PRIWu64 TXT( " filenameHash: %" ) RED_PRIWu64, fileName.AsChar(), isStatic, defString.AsChar(), contentHash, filenameHash );
		}

		if ( outShader )
		{
			GRenderShaderMap->Insert( isStatic ? filenameHash : contentHash, outShader );
		}
	}

	return outShader != nullptr;
}

Bool CRenderShader::CreateStaticFromCache( const String& fileName, const TDynArray< GpuApi::ShaderDefine >& defines, ERenderShaderType type, Bool hasStreamOut, CRenderShader*& outShader, const GpuApi::VertexLayoutDesc* outputDesc /*=nullptr*/, GpuApi::VertexLayoutDesc* adjustedDesc /*nullptr*/ )
{
	GpuApi::eShaderType shaderType = Map( type );

	Uint64 filenameHash = GpuApi::GetFilenameHash( UNICODE_TO_ANSI( fileName.AsChar() ), defines.TypedData(), defines.Size(), shaderType );

	{
		Red::Threads::CScopedLock< Red::Threads::CMutex > lock( CRenderShaderMap::s_mutex );
		if ( GRenderShaderMap->Find( filenameHash, outShader ) )
		{
			if ( outShader )
			{
				outShader->AddRef();
			}
			return outShader != nullptr;
		}
		
		StaticShaderEntry* entry = nullptr;
		if ( !GShaderCache->GetStaticShader( filenameHash, entry ) )
		{
			return false;
		}

		if ( entry )
		{
			if ( hasStreamOut )
			{
				RED_ASSERT( outputDesc );
				outShader = CreateGSwithSO( entry->m_data, entry->m_hash, *outputDesc, adjustedDesc );
			}
			else
			{
				outShader = Create( type, entry->m_data, entry->m_hash, fileName );
			}
		}

		if ( outShader )
		{
			GRenderShaderMap->Insert( filenameHash, outShader );
		}
	}

	return outShader != nullptr;
}

void CRenderShader::MapDefines( ERenderShaderType type, const CMaterialCompilerDefines& defines, TDynArray< GpuApi::ShaderDefine >& outDefines )
{
	GpuApi::eShaderType shaderType = Map( type );

	if ( shaderType == GpuApi::VertexShader || shaderType == GpuApi::GeometryShader || shaderType == GpuApi::HullShader || shaderType == GpuApi::DomainShader )
	{
		outDefines.PushBack( GpuApi::ShaderDefine() );
		outDefines.Back().Name = "VERTEXSHADER";
		outDefines.Back().Definition = "1";
	}
	else if ( shaderType == GpuApi::PixelShader )
	{
		outDefines.PushBack( GpuApi::ShaderDefine() );
		outDefines.Back().Name = "PIXELSHADER";
		outDefines.Back().Definition = "1";
	}

	// Additional defines for shaders to generate different hash then the vertex shader
	if ( shaderType == GpuApi::GeometryShader )
	{
		outDefines.PushBack( GpuApi::ShaderDefine() );
		outDefines.Back().Name = "GEOMETRYSHADER";
		outDefines.Back().Definition = "1";
	}
	if ( shaderType == GpuApi::HullShader )
	{
		outDefines.PushBack( GpuApi::ShaderDefine() );
		outDefines.Back().Name = "HULLSHADER";
		outDefines.Back().Definition = "1";
	}
	if ( shaderType == GpuApi::DomainShader )
	{
		outDefines.PushBack( GpuApi::ShaderDefine() );
		outDefines.Back().Name = "DOMAINSHADER";
		outDefines.Back().Definition = "1";
	}
	if ( shaderType == GpuApi::ComputeShader )
	{
		outDefines.PushBack( GpuApi::ShaderDefine() );
		outDefines.Back().Name = "COMPUTESHADER";
		outDefines.Back().Definition = "1";
	}

#ifdef RED_PLATFORM_ORBIS
	outDefines.PushBack( GpuApi::ShaderDefine() );
	outDefines.Back().Name = "__PSSL__";
	outDefines.Back().Definition = "1";
#endif

	// Custom defines
	const TMaterialCompilerDefinesList& customDefines = defines.GetDefines();
	for ( Uint32 i = 0; i < customDefines.Size(); ++i )
	{
		outDefines.PushBack( GpuApi::ShaderDefine() );
		outDefines.Back().Name = customDefines[ i ].m_first.AsChar();
		outDefines.Back().Definition = customDefines[ i ].m_second.AsChar();
	}
}

//////////////////////////////////////////////////////////////////////////

Red::Threads::CMutex CRenderShaderMap::s_mutex;

CRenderShaderMap::CRenderShaderMap() { }
CRenderShaderMap::~CRenderShaderMap() { }

Bool CRenderShaderMap::Exists( Uint64 hash )
{
	return m_map.KeyExist( hash );
}

Bool CRenderShaderMap::Insert( Uint64 hash, CRenderShader* shader )
{
	RED_FATAL_ASSERT( shader, "Trying to insert null renderShader into map - DEBUG THIS!" );
	RED_FATAL_ASSERT( !m_map.KeyExist( hash ), "RenderShader already exists - DEBUG THIS! Hash: %" RED_PRIWu64, hash );

	RED_LOG_SPAM( Shaders, TXT( "RenderShaderMap: Inserting shader %" ) RED_PRIWu64, hash );

	return m_map.Insert( hash, shader );
}

Bool CRenderShaderMap::Erase( Uint64 hash )
{
	RED_LOG_SPAM( Shaders, TXT( "RenderShaderMap: Erasing shader: %" ) RED_PRIWu64, hash );
	RED_FATAL_ASSERT( m_map.KeyExist( hash ), "Trying to erase non-exising RenderShader %" RED_PRIWu64, hash );
	return m_map.Erase( hash );
}

Bool CRenderShaderMap::Find( Uint64 hash, CRenderShader*& outShader )
{
	return m_map.Find( hash, outShader );
}

static CRenderShaderMap s_renderShaderMap;
CRenderShaderMap* GRenderShaderMap = &s_renderShaderMap;