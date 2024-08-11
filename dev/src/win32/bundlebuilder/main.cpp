/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

// Include for reading bundle definitions.
#include "../../common/core/core.h"
#include "../../common/core/bundledefinition.h"

// These stubs exists so that we can compile with core. Remove when bundle builder is no longer dependent on core.
void EntityHandleDataGetObjectHandle( const void*, THandle< CObject >& ) {}
void EntityHandleDataSetObjectHandle( void*, THandle< CObject >& ) {}

// Include the logging devices
#include "../../common/redSystem/windowsDebuggerWriter.h"

#include "bundleBuilderMemory.h"
#include "bundlewriter.h"
#include "bundle.h"
#include "compressionProfiler.h"
#include "feedback.h"
#include "options.h"
#include "autoCacheBuilder.h"

static const Float GCurrentVersion = 0.92f;

enum EProgramReturnCode
{
	PRC_Success = 0,
	PRC_CommandLineFailed,
	PRC_DirectoryValidationFailed,
	PRC_DefinitionReadFailed,
};

void PrintCopyright()
{
	// Print out program information
	printf("# Bundle Builder Tool.\n");
	printf("# Copyright© 2013 CD Projekt Red. All Rights Reserved.\n");
	printf("# Version %1.2f\n", GCurrentVersion );
	printf("\n");
}

int main( int argc, const char* argv[] )
{
	using namespace Red::Core;
	namespace Log = Red::System::Log;

	EProgramReturnCode errorCode = PRC_Success;

	RED_MEMORY_INITIALIZE( BundleBuilderMemoryParameters );

#if defined( RELEASE )
	Red::System::Error::Handler::GetInstance()->SetAssertFlag( Red::System::Error::AC_SilentCrashHook, true );
	Red::System::Error::Handler::GetInstance()->SetAssertFlag( Red::System::Error::AC_Break, false );
	Red::System::Error::Handler::GetInstance()->SetAssertFlag( Red::System::Error::AC_PopupHook, false );
	Red::System::Error::Handler::GetInstance()->SetAssertFlag( Red::System::Error::AC_ContinueAlways, true );
#else
	Red::System::Error::Handler::GetInstance()->SetAssertFlag( Red::System::Error::AC_Break, true );
	Red::System::Error::Handler::GetInstance()->SetAssertFlag( Red::System::Error::AC_PopupHook, false );
#endif

#ifndef RED_USE_NEW_MEMORY_SYSTEM

	INTERNAL_RED_MEMORY_CREATE_POOL
	(
		SRedMemory::GetInstance(),
		BundlePools,
		MemoryPool_BundleBuilder,
		Red::MemoryFramework::TLSFAllocatorThreadSafe::CreationParameters
		(
			c_oneGigabyte,
			c_oneGigabyte * 16,
			c_BundleBuilderPoolGranularity
		),
		0
	);

#endif 

	Bundler::COptions options;

	options.Parse( argc, argv );

	if( options.HasErrors() )
	{
		PrintCopyright();

		options.PrintErrors();

		options.PrintCommandLine();

		errorCode = PRC_CommandLineFailed;
	}
	else
	{
		options.ValidatePaths();

		if( options.HasError() )
		{
			PrintCopyright();

			options.PrintErrors();

			options.PrintCommandLine();

			errorCode = PRC_DirectoryValidationFailed;
		}
		else
		{
			if( !options.IsSilent() )
			{
				PrintCopyright();
			}

			Bundler::Feedback::SILENT = options.IsSilent();

			Log::WindowsDebuggerWriter debuggerLogDevice;

			BundleDefinition::CBundleDefinitionReader bundleDefinitionReader( options.GetDefinitionPath().AsChar() );
			BundleDefinition::CBundleDefinitionReader::EErrorCode success = bundleDefinitionReader.Read();

			if( success == BundleDefinition::CBundleDefinitionReader::EC_Success )
			{
				// We use a CBundleDefinitionFilePathResolver to munge the cooked paths to absolute paths here
				// This is so we can use a mix of cooked and raw files
				Red::Core::BundleDefinition::CBundleDefinitionFilePathResolver resolvedDefinition( std::move( bundleDefinitionReader ), options.GetCookedDirectory().AsChar() );

				if( options.IsProfileMode() )
				{
					Bundler::CCompressionProfiler profiler( resolvedDefinition, options.GetOutputDirectory().AsChar() );

					profiler.Run( options );
				}
				else if( options.BuildAutoCache() )
				{
					Bundler::CAutoCacheBuilder autoCacheBuilder( resolvedDefinition );

					autoCacheBuilder.Run( options );
				}
				else
				{
					Bundler::CBundleWriter bundleWriter( resolvedDefinition );

					bundleWriter.Run( options );
				}
			}
			else
			{
				if( !options.IsSilent() )
				{
					switch( success )
					{
					case BundleDefinition::CBundleDefinitionReader::EC_FileNotFound:
						printf( "Could not find definition file: %s", options.GetDefinitionPath().AsChar() );
						break;

					case BundleDefinition::CBundleDefinitionReader::EC_InvalidJSON:
						printf( "Definition file does not contain valid json data: %s", options.GetDefinitionPath().AsChar() );
						break;

					case BundleDefinition::CBundleDefinitionReader::EC_InvalidBundle:
						printf( "Definition file does not contain valid bundle data: %s", options.GetDefinitionPath().AsChar() );
						break;
					}
				}

				errorCode = PRC_DefinitionReadFailed;
			}
		}
	}

	RED_LOG_FLUSH();

	// Uncomment to check for memory leaks
	//INTERNAL_RED_MEMORY_GET_ALLOCATOR( SRedMemory::GetInstance(), BundlePools, MemoryPool_BundleBuilder )->DumpDebugOutput();

	return errorCode;
}
