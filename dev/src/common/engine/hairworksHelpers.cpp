/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "hairworksHelpers.h"


#ifdef USE_NVIDIA_FUR

namespace HairWorksHelpers
{

	void DefaultLogHandler::Log(GFSDK_HAIR_LOG_TYPES logType, const char* message, const char* file, int line)
	{
		switch ( logType )
		{
		case GFSDK_HAIR_LOG_ERROR:
			RED_LOG_ERROR( Hairworks, ANSI_TO_UNICODE( message ) );
			break;
		case GFSDK_HAIR_LOG_WARNING:
			RED_LOG_WARNING( Hairworks, ANSI_TO_UNICODE( message ) );
			break;
		default:
			RED_LOG( Hairworks, ANSI_TO_UNICODE( message ) );
			break;
		}
	}



	static void* HairWorksNew( size_t size )
	{
		return RED_MEMORY_ALLOCATE_ALIGNED( MemoryPool_Default, MC_RenderData, size, 16 );
	}
	static void HairWorksDelete( void* ptr )
	{
		RED_MEMORY_FREE( MemoryPool_Default, MC_RenderData, ptr );
	}


	static gfsdk_new_delete_t s_defaultAlloc = { HairWorksNew, HairWorksDelete };



	GFSDK_HairSDK* InitSDK( GFSDK_HAIR_LogHandler* logHandler )
	{
		GFSDK_HairSDK* sdk = nullptr;

#if defined(RED_ARCH_X64)
		sdk = GFSDK_LoadHairSDK( "GFSDK_HairWorks.win64.dll", GFSDK_HAIRWORKS_VERSION, &s_defaultAlloc, logHandler );
#elif defined(RED_ARCH_X86)
		sdk = GFSDK_LoadHairSDK( "GFSDK_HairWorks.win32.dll", GFSDK_HAIRWORKS_VERSION, &s_defaultAlloc, logHandler );
#endif
		if ( sdk == nullptr )
		{
			RED_LOG_ERROR( Hairworks, TXT("HairWorks library not initialized" ) );
		}

		return sdk;
	}

	void ShutdownSDK( GFSDK_HairSDK*& sdk )
	{
		if ( sdk != nullptr )
		{
			sdk->FreeRenderResources();
			sdk->Release();
			sdk = nullptr;
		}
	}


	Bool SaveShaderCache( GFSDK_HairSDK* sdk, const String& absolutePath )
	{
		if ( sdk == nullptr )
		{
			RED_LOG_ERROR( Hairworks, TXT("Cannot save HairWorks shader cache without an SDK." ) );
			return false;
		}

		void* shaderCacheData = nullptr;
		size_t shaderCacheSize = 0;
		GFSDK_HAIR_RETURNCODES result = sdk->SaveShaderCacheToMemory( &shaderCacheData, shaderCacheSize );
		if ( result != GFSDK_RETURN_OK )
		{
			RED_LOG_ERROR( Hairworks, TXT("Failed to build shader cache in memory." ) );
			return false;
		}
		RED_FATAL_ASSERT( shaderCacheData != nullptr, "SaveShaderCacheToMemory succeeded, but don't have any data" );


		IFile* file = GFileManager->CreateFileWriter( absolutePath, FOF_AbsolutePath );
		if ( file == nullptr )
		{
			RED_LOG_ERROR( Hairworks, TXT("Failed to open %ls for writing"), absolutePath.AsChar() );
			HairWorksDelete( shaderCacheData );
			return false;
		}

		file->Serialize( shaderCacheData, shaderCacheSize );
		delete file;

		HairWorksDelete( shaderCacheData );

		RED_LOG( Hairworks, TXT("Saved Hairworks shader cache, %") RED_PRIWsize_t TXT(" bytes to %ls."), shaderCacheSize, absolutePath.AsChar() );

		return true;
	}


	Bool LoadShaderCache( GFSDK_HairSDK* sdk, const String& absolutePath, Bool append )
	{
		if ( sdk == nullptr )
		{
			RED_LOG_ERROR( Hairworks, TXT("Cannot load HairWorks shader cache without an SDK." ) );
			return false;
		}

		IFile* file = GFileManager->CreateFileReader( absolutePath, FOF_AbsolutePath );
		if ( file == nullptr )
		{
			RED_LOG_ERROR( Hairworks, TXT("Failed to open fur shader cache at %ls. Skipping."), absolutePath.AsChar() );
			return false;
		}

		const Uint32 size = (Uint32)file->GetSize();
		void* shaderCacheData = HairWorksNew( size );
		if ( shaderCacheData == nullptr )
		{
			RED_LOG_ERROR( Hairworks, TXT("Failed to allocate %u bytes for loading fur shader cache %ls"), size, absolutePath.AsChar() );
			delete file;
			return false;
		}

		file->Serialize( shaderCacheData, size );
		delete file;

		GFSDK_HAIR_RETURNCODES result = sdk->LoadShaderCacheFromMemory( shaderCacheData, true );

		HairWorksDelete( shaderCacheData );

		if ( result != GFSDK_RETURN_OK )
		{
			RED_LOG_ERROR( Hairworks, TXT("LoadShaderCacheFromMemory failed. Failed to load existing shader cache from %ls"), absolutePath.AsChar() );
			return false;
		}

		RED_LOG( Hairworks, TXT("Loaded fur shader cache from %ls, %u bytes"), absolutePath.AsChar(), size );

		return true;
	}


}

#endif

