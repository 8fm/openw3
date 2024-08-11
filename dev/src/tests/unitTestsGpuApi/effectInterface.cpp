/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "effectInterface.h"

#if defined(RED_PLATFORM_WINPC) || defined(RED_PLATFORM_DURANGO)

GpuApi::ShaderRef IEffect::CreateShaderFromFile( const Char* path, GpuApi::eShaderType shaderType ) const
{
	const AnsiChar* shaderTarget = nullptr;
	const AnsiChar* entryPoint = nullptr;
	if ( !GpuApi::GetShaderTargetAndEntryPoint( shaderType, shaderTarget, entryPoint ) )
	{
		ERR_GAT( TXT("Unknown shader target or entry point!") );
		return GpuApi::ShaderRef::Null();
	}

	Red::IO::CNativeFileHandle file;

	if ( !file.Open( path, Red::IO::eOpenFlag_Read ) )
	{
		ERR_GAT( TXT("Missing shader file: '%ls'"), path );
		return GpuApi::ShaderRef::Null();
	}

	Uint64 fileSize = file.GetFileSize();
	char* code = (char*)GPU_API_ALLOCATE( GpuApi::GpuMemoryPool_ShaderInclude, GpuApi::MC_Temporary, fileSize, 16 );

	if( !code )
	{
		ERR_GAT( TXT( "Failed to allocate shader memory!" ) );
		return GpuApi::ShaderRef::Null();
	}

	Uint32 readBytes;
	file.Read( code, static_cast<Uint32>( fileSize ), readBytes );
	if( readBytes == 0 )
	{
		ERR_GAT( TXT( "Failed to read shader from file!" ) );
		GPU_API_FREE( GpuApi::GpuMemoryPool_ShaderInclude, GpuApi::MC_Temporary, code );
		return GpuApi::ShaderRef::Null();
	}

	code[readBytes] = 0;

	GpuApi::ShaderRef ret = GpuApi::CreateShaderFromSource( shaderType, code, entryPoint, shaderTarget, nullptr, 0, UNICODE_TO_ANSI( path ) );
	GPU_API_FREE( GpuApi::GpuMemoryPool_ShaderInclude, GpuApi::MC_Temporary, code );
	return ret;
}

#elif defined(RED_PLATFORM_ORBIS)

void* LoadShaderFromFile( const Char* path, Uint32& codeSize )
{
	Red::IO::CNativeFileHandle file;
	if ( !file.Open( path, Red::IO::eOpenFlag_Read ) )
	{
		RED_HALT( "Missing shader file: '%hs'", path );
		return nullptr;
	}

	GpuApi::Uint64 fileSize = file.GetFileSize();
	void* code = GPU_API_ALLOCATE( GpuApi::GpuMemoryPool_Shaders, GpuApi::MC_Temporary, fileSize, 16 );

	Uint32 readBytes;
	file.Read( code, static_cast< GpuApi::Uint32 >( fileSize ), readBytes );

	codeSize = readBytes;
	file.Close();
	return code;
}

GpuApi::ShaderRef IEffect::CreateShaderFromFile( const Char* path, GpuApi::eShaderType shaderType ) const
{
	Uint32 codeSize = 0;
	char* shaderCode = reinterpret_cast< char* >( LoadShaderFromFile( path, codeSize) );

	if( !shaderCode )
	{
		ERR_GAT( TXT( "Failed to load VS shader from file!" ) );
		return GpuApi::ShaderRef::Null();
	}

	shaderCode[codeSize] = 0;
	
	GpuApi::ShaderRef ret = GpuApi::CreateShaderFromBinary( shaderType, shaderCode, codeSize );
	if( !ret )
	{
		ERR_GAT( TXT( "Failed to create vs from source!" ) );
		return GpuApi::ShaderRef::Null();
	}

	return ret;
}

#endif