/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "autoCacheBuilder.h"

#include "autoCache.h"
#include "bundleBuilderMemory.h"

#include "../../common/core/bundledefinition.h"


namespace Bundler {

//////////////////////////////////////////////////////////////////////////
CAutoCacheBuilderWorker::CAutoCacheBuilderWorker( Red::Threads::CSemaphore* lock, const COptions& options )
:	CConsumer< Payload >( "Auto Cache Builder", lock )
,	m_numTestIterations( options.GetCompressionIterations() )
{

}

CAutoCacheBuilderWorker::~CAutoCacheBuilderWorker()
{

}

void CAutoCacheBuilderWorker::Do()
{
	Uint32 smallFilesCount = 0;
	const Uint32 bundleFileCount = m_payload.m_bundle->GetFileCount();

	m_feedback.NewProgressbar( m_payload.m_bundle->GetBundleName() );
	CProfilerFeedback feedbackHelper( m_feedback, bundleFileCount );

	for( Uint32 iBundleItem = 0; iBundleItem < bundleFileCount; ++iBundleItem )
	{
		const Red::Core::BundleDefinition::SBundleFileDesc* bundleFileDesc = m_payload.m_bundle->GetBundleFileDescription( iBundleItem );

		if( bundleFileDesc->m_compressionType == Red::Core::Bundle::CT_Auto )
		{
			const StringAnsi& path = bundleFileDesc->m_resolvedResourcePath;

			FILE* file = fopen( path.AsChar(), "rb" );

			if( file )
			{
				// Get size of file
				RED_VERIFY( fseek( file, 0, SEEK_END ) == 0, TXT( "Unable to seek to end of file" ) );
				Uint32 size = ftell( file );

				if( size >= MINIMUM_SIZE_THRESHOLD )
				{
					if( size <= c_defaultPoolGranularity )
					{
						// Move read point to start of file
						RED_VERIFY( fseek( file, 0, SEEK_SET ) == 0, TXT( "Unable to seek to start of file" ) );

						// Allocate memory for file
						void* buffer = BUNDLER_MEMORY_ALLOCATE( MC_BundlerAutoCache, size );

						RED_VERIFY( fread( buffer, sizeof( Uint8 ), size, file ) == size, TXT( "Amount Read doesn't match amount expected" ) );

						{
							Red::Core::Bundle::ECompressionType compression;
							if ( !m_payload.m_autoCache->GetResult( bundleFileDesc->m_resourcePath, compression ) )
							{
								SProfilerItem profilerData;
								CProfilerCommon profiler( &feedbackHelper );

								profiler.Profile( buffer, size, profilerData, m_numTestIterations );

								m_payload.m_autoCache->AddResult( bundleFileDesc->m_resourcePath, profilerData.m_best );
							}
						}

						// Free file buffer
						BUNDLER_MEMORY_FREE( MC_BundlerAutoCache, buffer );
					}
					else
					{
						BUNDLE_ERROR( ( &m_feedback ), TXT( "File is larger than the memory system granularity: %hs (%u vs %u)" ), path.AsChar(), size, static_cast< Uint32 >( c_defaultPoolGranularity ) );
					}
				}
				else
				{
					smallFilesCount++;
				}

				fclose( file );
			}
			else
			{
				BUNDLE_ERROR( ( &m_feedback ), TXT( "Failed to open bundle file for writing: %hs" ), path.AsChar() );
			}
		}
	}
	BUNDLE_WARNING( ( &m_feedback ), TXT( "%u files are smaller than minimum size permitted for compression: %u" ), smallFilesCount, MINIMUM_SIZE_THRESHOLD );

	m_feedback.MarkCompleted( Feedback::CS_Success );
}

//////////////////////////////////////////////////////////////////////////
CAutoCacheBuilder::CAutoCacheBuilder( Red::Core::BundleDefinition::IBundleDefinition& bundleDef )
:	CProducer< CAutoCacheBuilderWorker, Payload >( bundleDef )
,	m_definitionAutoCache( nullptr )
{

}

CAutoCacheBuilder::~CAutoCacheBuilder()
{

}

void CAutoCacheBuilder::Initialize( const COptions& options )
{
	m_definitionAutoCache = new AutoCache::Definition;

	m_definitionAutoCache->Initialize( m_bundleDefinition.GetBundleCount() );

	m_bundleDefinition.GetBundleCount();
	if ( !options.GetBaseAutoCachePath().Empty() )
	{
		TDynArray< StringAnsi > bundleNames( m_bundleDefinition.GetBundleCount() );
		m_bundleDefinition.GetBundleNames( bundleNames );
		m_definitionAutoCache->Load( options.GetBaseAutoCachePath().AsChar(), bundleNames );
	}
}

void CAutoCacheBuilder::FillPayload( Payload& payload, const StringAnsi& bundleName, Red::Core::BundleDefinition::CBundleDataContainer* bundleData )
{
	AutoCache::Bundle* bundleAutoCache = nullptr;
	if( m_definitionAutoCache )
	{
		bundleAutoCache = &( m_definitionAutoCache->GetResults( bundleName ) );
	}

	// Free worker found
	payload.m_autoCache	= bundleAutoCache;
	payload.m_bundle	= bundleData;
}

void CAutoCacheBuilder::Shutdown( const COptions& options )
{
	m_definitionAutoCache->Normalize();
	m_definitionAutoCache->Save( options.GetAutoCachePath().AsChar() );
	delete m_definitionAutoCache;
}

} // namespace Bundler {
