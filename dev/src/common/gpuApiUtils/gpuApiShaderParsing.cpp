
#include "gpuApiUtils.h"
#include "gpuApiShaderParsing.h"
#include "gpuApiMemory.h"
#include "../redSystem/hash.h"
#include "../redIO/redIO.h"
#include "../redSystem/crt.h"

// FIXME Windows and Durango: We're using WC memory for a CPU side hash!!!
// PS4 has this "Misc" memory pool, need something similar for Windows.
#ifdef RED_PLATFORM_ORBIS
#define GpuMemoryPool_Mojo GpuMemoryPool_SmallOnion
#else
#include "mojoshader/mojoshader.h"
# define GpuMemoryPool_Mojo  GpuMemoryPool_ShaderInclude
#endif

namespace GpuApi
{
#ifndef RED_PLATFORM_ORBIS

	// returns zero on error, non-zero on success
	static Int32 MOJO_SHADER_INCLUDE_open( MOJOSHADER_includeType inctype, const AnsiChar* fname, const AnsiChar* parent, const AnsiChar** outdata, Uint32* outbytes, MOJOSHADER_malloc m, MOJOSHADER_free f, void* d )
	{
		RED_UNUSED( parent );
		RED_UNUSED( inctype );
		RED_UNUSED( m );
		RED_UNUSED( f );
		RED_UNUSED( d );

		if ( !fname )
		{
			RED_HALT( "include filename is null" );
			return 0;
		}

		Red::System::AnsiChar absoluteFilePath [1024];
		Red::System::SNPrintF( absoluteFilePath, 1024, "%hs%hs", GetShaderIncludePathAnsi(), fname ); 

		Red::IO::CNativeFileHandle file;
		Red::System::Char unicodeFilePath[1024];
		Red::System::StdCharToWideChar( unicodeFilePath, absoluteFilePath, 1024 );
		if ( ! file.Open( unicodeFilePath, Red::IO::eOpenFlag_Read ) )
		{
			RED_HALT( "Failed to open '%hs'", absoluteFilePath );
			return 0;
		}

		file.Seek( 0, Red::IO::eSeekOrigin_End );
		GpuApi::Uint32 size = (Uint32)file.Tell(); // Why not just size? Keeping original test here: get file position == size of file
		file.Seek( 0, Red::IO::eSeekOrigin_Set ); // move to start	

		GpuApi::Uint32 bufferChunkSize = sizeof( AnsiChar ) * ( size + 1 );
		AnsiChar* code = reinterpret_cast< AnsiChar* >( GPU_API_ALLOCATE( GpuMemoryPool_Mojo, MC_Temporary, bufferChunkSize, 16 ) );

		Uint32 bytesRead = 0;
		file.Read( code, sizeof( AnsiChar ) * size, bytesRead );
		if ( bytesRead != sizeof(AnsiChar)*size )
		{
			RED_HALT( "Can't read whole file '%hs'", absoluteFilePath );
			file.Close();
			return 0;
		}

		code[ size ] = 0;

		file.Close();

		*outdata = code;
		*outbytes = size;

		return 1;
	}

	static void MOJO_SHADER_INCLUDE_close( const AnsiChar* data, MOJOSHADER_malloc m, MOJOSHADER_free f, void* d )
	{
		RED_UNUSED( m );
		RED_UNUSED( f );
		RED_UNUSED( d );
		GPU_API_FREE( GpuMemoryPool_Mojo, MC_Temporary, (AnsiChar*)data );
	}
#endif // !RED_PLATFORM_ORBIS

	Bool IsCompilerShaderDefine( const ShaderDefine &define )
	{
		if ( define.Name && define.Name[0] && define.Definition && define.Definition[0] )
		{
			if ( define.Name == strstr( define.Name, "__XBOX" ) )
			{
				return true;
			}
		}

		return false;
	}

	Bool Preprocess( const char* code, const ShaderDefine* defines, Uint32 numDefines, const char* fileName, char*& preprocessedCode, Uint32& preprocessedLength, void* (*Allocate)( Uint32 size ) /*=nullptr*/, void (*Free)( void* ptr ) /*=nullptr*/ )
	{
#ifdef RED_PLATFORM_ORBIS
		//switching off preprocess for now because we don't want mojoshader to be compiled on PS4 (too many warnings)
		return false;
#else
		MOJOSHADER_preprocessorDefine* mojo_defines = nullptr;
		if ( Allocate )
		{
			mojo_defines = static_cast< MOJOSHADER_preprocessorDefine* >( Allocate( sizeof( MOJOSHADER_preprocessorDefine ) * numDefines ) );
		}
		else
		{
			mojo_defines = static_cast< MOJOSHADER_preprocessorDefine* >( GPU_API_ALLOCATE( GpuMemoryPool_Mojo, MC_Temporary, sizeof( MOJOSHADER_preprocessorDefine ) * numDefines, 16 ) );
		}
		MOJOSHADER_preprocessorDefine* dstDefine = mojo_defines;
		const ShaderDefine* srcDefine = defines;
		for ( Uint32 i = 0; i < numDefines; ++i )
		{
			dstDefine->identifier = srcDefine->Name;
			dstDefine->definition = srcDefine->Definition;
			++dstDefine;
			++srcDefine;
		}

		Uint32 codeSize = (Uint32)Red::System::StringLength( code )+1;

		MOJOSHADER_malloc mallocFunc = nullptr;
		MOJOSHADER_free freeFunc = nullptr;

		const MOJOSHADER_preprocessData* ppData = MOJOSHADER_preprocess( fileName, code, codeSize, mojo_defines, numDefines, MOJO_SHADER_INCLUDE_open, MOJO_SHADER_INCLUDE_close, mallocFunc, freeFunc, nullptr );

		if ( ppData->output != nullptr )
		{
			if ( ppData->error_count > 0 )
			{
				GPUAPI_LOG( TXT("Shader preprocess warnings") );
				for (Int32 errorI = 0; errorI < ppData->error_count; ++errorI)
				{
					GPUAPI_LOG( TXT("%hs (%d): %hs"), ppData->errors[errorI].filename, ppData->errors[errorI].error_position, ppData->errors[errorI].error );
				}
			}

			preprocessedLength = ppData->output_len;
			if ( Allocate )
			{
				preprocessedCode = static_cast< char* >( Allocate( preprocessedLength ) );
			}
			else
			{
				preprocessedCode = static_cast< char* >(GPU_API_ALLOCATE( GpuMemoryPool_Mojo, MC_Temporary, preprocessedLength, 16 ));
			}

			Red::System::MemoryCopy( preprocessedCode, ppData->output, preprocessedLength );

			MOJOSHADER_freePreprocessData( ppData );
			if ( Free )
			{
				Free( mojo_defines );
			}
			else
			{
				GPU_API_FREE( GpuMemoryPool_Mojo, MC_Temporary, mojo_defines );
			}

			return true;
		}
		else
		{
			GPUAPI_ASSERT( TXT("Shader preprocess errors") );
			for (Int32 errorI = 0; errorI < ppData->error_count; ++errorI)
			{
				GPUAPI_LOG( TXT("%hs (%d): %hs"), ppData->errors[errorI].filename, ppData->errors[errorI].error_position, ppData->errors[errorI].error );
			}

			MOJOSHADER_freePreprocessData( ppData );
			if ( Free )
			{
				Free( mojo_defines );
			}
			else
			{
				GPU_API_FREE( GpuMemoryPool_Mojo, MC_Temporary, mojo_defines );
			}

			return false;
		}
#endif
	}

	Bool GetShaderHash( Uint64& hash, const AnsiChar* code, const AnsiChar* entryPoint, const ShaderDefine* defines, Uint32 numDefines, const char* fileName, AnsiChar** preprocessedCode /*=nullptr*/, Uint32* preprocessedLength /*=nullptr*/, void* (*Allocate)( Uint32 size ) /*=nullptr*/, void (*Free)( void* ptr ) /*=nullptr*/ )
	{
		Bool ret = false;

		AnsiChar* codeAfterPreprocessing = nullptr;
		Uint32 codeAfterPreprocessingLength = 0;

		AnsiChar** outCode = preprocessedCode ? preprocessedCode : &codeAfterPreprocessing;
		Uint32* outLength = preprocessedLength ? preprocessedLength : &codeAfterPreprocessingLength;

		if ( Preprocess( code, defines, numDefines, fileName, *outCode, *outLength, Allocate, Free ) )
		{
			if ( *outCode )
			{
				hash = Red::System::CalculateHash64SkipWhitespaces( *outCode, *outLength, hash );
				hash = Red::System::CalculateHash64SkipWhitespaces( entryPoint, static_cast< GpuApi::Uint32 >( Red::System::StringLength( entryPoint ) ), hash );
				
				// Include defines, because some of them may be used by the shader compiler (not only by the shader preprocessor)
				hash = Red::System::CalculateHash64( "DEFINES", hash );
				for ( Uint32 def_i=0; def_i<numDefines; ++def_i )
				{
					const ShaderDefine &def = defines[def_i];
					if( IsCompilerShaderDefine( def ) )
					{
						hash = Red::System::CalculateHash64( def.Name ? def.Name : "", hash );
						hash = Red::System::CalculateHash64( def.Definition ? def.Definition : "", hash );
					}
				}

				ret = true;
			}
		}

		// cleanup locally allocated string
		if ( codeAfterPreprocessing ) 
		{
			if ( Free )
			{
				Free( codeAfterPreprocessing );
			}
			else
			{
				GPU_API_FREE( GpuMemoryPool_Mojo, MC_Temporary, codeAfterPreprocessing );
			}
		}

		return ret;
	}

	Uint64 GetFilenameHash( const AnsiChar* fileName, const ShaderDefine* defines, Uint32 numDefines, eShaderType shaderType )
	{
		Uint64 hash = Red::System::CalculateHash64( fileName );
		for ( Uint32 i = 0; i < numDefines; ++i )
		{
			hash = Red::System::CalculateHash64( defines[ i ].Name, hash );
			hash = Red::System::CalculateHash64( defines[ i ].Definition, hash );
		}
		hash = Red::System::CalculateHash64( &shaderType, sizeof( shaderType ), hash );
		return hash;
	}
}
