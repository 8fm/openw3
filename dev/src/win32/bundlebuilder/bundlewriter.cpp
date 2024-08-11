// header Include
#include "bundlewriter.h"
#include "bundleBuilderMemory.h"
#include "bundle.h"
#include "autoCache.h"

#include "../../common/redSystem/cpuid.h"

#include "../../common/core/bundledefinition.h"
#include "../../common/core/sortedset.h"
#include "../../common/core/string.h"
#include <direct.h>

#define INVALID_BUNDLE_FILE_ENTRY_SIZE static_cast< Uint32 >( -1 )

namespace Bundler {

using namespace Red::Core;

//////////////////////////////////////////////////////////////////////////
CBundleWriter::CBundleWriter( Red::Core::BundleDefinition::IBundleDefinition& bundleDef )
:	CProducer< CBundleWriterWorker, Payload >( bundleDef )
,	m_definitionAutoCache( nullptr )
{
}

//////////////////////////////////////////////////////////////////////////
CBundleWriter::~CBundleWriter()
{

}

void CBundleWriter::Initialize( const COptions& options )
{
	InitializeAutoCache( options );

	InitializeBundleDirectories( options );
}

void CBundleWriter::InitializeAutoCache( const COptions& options )
{
	RED_FATAL_ASSERT( m_definitionAutoCache == nullptr, "Autocache already initialised" );

	if( options.UseAutoCache() )
	{
		m_definitionAutoCache = new AutoCache::Definition;

		m_definitionAutoCache->Initialize( m_bundleDefinition.GetBundleCount() );
		
		TDynArray< StringAnsi > bundleNames( m_bundleDefinition.GetBundleCount() );
		m_bundleDefinition.GetBundleNames( bundleNames );
		m_definitionAutoCache->Load( options.GetAutoCachePath().AsChar(), bundleNames );
	}
}

void CBundleWriter::InitializeBundleDirectories( const COptions& options )
{
	TSortedSet< StringAnsi > paths;

	using BundleDefinition::TBundleDataContainers;

	// Extract all the directories paths (removing the filenames) and place them into a sorted set
	// This will catch all duplicate directories and only create them once
	const TBundleDataContainers& bundleDataContainer( m_bundleDefinition.GetBundles() );
	for( TBundleDataContainers::const_iterator iBundle = bundleDataContainer.Begin(); iBundle != bundleDataContainer.End(); ++iBundle )
	{
		size_t index = 0;
		if( !iBundle->m_first.FindCharacter( DIRECTORY_SEPARATOR_LITERAL, index, true ) )
		{
			iBundle->m_first.FindCharacter( ALTERNATIVE_DIRECTORY_SEPARATOR_LITERAL, index, true );
		}

		if( index != 0 )
		{
			paths.Insert( std::move( iBundle->m_first.LeftString( index + 1 ) ) );
		}
	}

	for( TSortedSet< StringAnsi >::const_iterator iter = paths.Begin(); iter != paths.End(); ++iter )
	{
		Utility::CreateDirectory( options.GetOutputDirectory().AsChar(), iter->AsChar() );
	}
}

void CBundleWriter::Shutdown( const COptions& options )  
{
}

void CBundleWriter::FillPayload( Payload& payload, const StringAnsi& bundleName, Red::Core::BundleDefinition::CBundleDataContainer* bundleData )  
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

//////////////////////////////////////////////////////////////////////////
// Worker
//////////////////////////////////////////////////////////////////////////
CBundleWriterWorker::CBundleWriterWorker( Red::Threads::CSemaphore* lock, const COptions& options )
:	CConsumer< Payload >( "Bundle Writer", lock )
,	m_options( options )
,	m_inputBuffer( nullptr )
,	m_inputBufferSize( 0 )
{
	m_writer.Initialise( c_oneGigabyte / 4 );

	static_assert( c_BundleBuilderPoolGranularity > ( 64 * c_oneMegabyte ), "Invalid size specified for input pool" );
	m_inputBufferSize = c_BundleBuilderPoolGranularity - ( 64 * c_oneMegabyte );
	m_inputBuffer = static_cast< Uint8* >( BUNDLER_MEMORY_ALLOCATE( MC_BundlerInputBuffer, m_inputBufferSize ) );
}

//////////////////////////////////////////////////////////////////////////
CBundleWriterWorker::~CBundleWriterWorker()
{
	BUNDLER_MEMORY_FREE( MC_BundlerInputBuffer, m_inputBuffer );
}

void CBundleWriterWorker::Do()
{
	const BundleDefinition::CBundleDataContainer* bundleDataContainer = m_payload.m_bundle;

	m_feedback.NewProgressbar( bundleDataContainer->GetBundleName() );

	BundleCreationParameters params( m_options, m_writer, m_crcCalculator );
	params.m_bundleDataContainer	= bundleDataContainer;
	params.m_feedback				= &m_feedback;
	params.m_autoCache				= m_payload.m_autoCache;
	params.m_inputBuffer			= m_inputBuffer;
	params.m_inputBufferSize		= m_inputBufferSize;

	CBundleWriterCommon writerCommon;
	writerCommon.CreateBundle( params );

	m_payload.m_bundle = nullptr;
	m_payload.m_autoCache = nullptr;
}

//////////////////////////////////////////////////////////////////////////
// Common
//////////////////////////////////////////////////////////////////////////
void CBundleWriterCommon::CreateBundle( const BundleCreationParameters& params ) const
{
	CBundle currentBundle( params );

	const AnsiChar* bundleName = params.m_bundleDataContainer->GetBundleName();
	const Uint32 bundleFileCount = params.m_bundleDataContainer->GetFileCount();

	for( Uint32 i = 0; i < bundleFileCount; ++i )
	{
		const BundleDefinition::SBundleFileDesc* bundleFileDesc = params.m_bundleDataContainer->GetBundleFileDescription( i );

		Bundle::SBundleHeaderItem bundleHeaderItem;
		Red::System::StringCopy( bundleHeaderItem.m_rawResourcePath, bundleFileDesc->m_resourcePath.AsChar(), Bundle::SBundleHeaderItem::RAW_RESOURCE_PATH_MAX_SIZE );

		bundleHeaderItem.m_resourceId		= bundleFileDesc->m_resourceId;
		bundleHeaderItem.m_resourceType		= bundleFileDesc->m_fourCC;
		bundleHeaderItem.m_dataOffset		= 0;
		bundleHeaderItem.m_compressionType	= bundleFileDesc->m_compressionType;

		// Override the compression type, if requested
		if( params.m_options.IsCompressionOverridden() )
		{
			bundleHeaderItem.m_compressionType = params.m_options.GetCompressionType();
		}

		/*for( Uint32 dependencyIndex = 0; dependencyIndex < Bundle::SBundleHeaderItem::MAX_DEPENDENCY_ITEMS; ++dependencyIndex )
		{
			bundleHeaderItem.m_resourceDependencies[dependencyIndex] = bundleFileDesc->m_resourceDependencies[dependencyIndex];
		}*/

		if( !FillInDetails( bundleFileDesc->m_resolvedResourcePath, bundleHeaderItem ) )
		{
			BUNDLE_WARNING( params.m_feedback, TXT( "File (%hs) does not exist, removing from bundle: %hs" ), bundleHeaderItem.m_rawResourcePath, bundleName );

			// Do not include file in bundle
			continue;
		}

		if( bundleHeaderItem.m_dataSize == 0 )
		{
			// Probably an empty .txt file (meaning we get a valid file handle but reported size of 0 bytes)
			BUNDLE_WARNING( params.m_feedback, TXT( "File (%hs) is empty and has no size, removing from bundle: %hs" ), bundleHeaderItem.m_rawResourcePath, bundleName );

			// Do not include file in bundle
			continue;
		}

		// File is safe for bundling
		currentBundle.AddItem( bundleHeaderItem, bundleFileDesc->m_resolvedResourcePath );
	}

	if( currentBundle.GetNumItems() > 0 )
	{
		currentBundle.Construct( params.m_inputBuffer, params.m_inputBufferSize );
	}
	else
	{
		BUNDLE_ERROR( params.m_feedback, TXT( "Either the bundle has no file entries in the definition or they were all removed for being invalid: %hs" ), bundleName );
	}
}

Bool CBundleWriterCommon::FillInDetails( const StringAnsi& path, Red::Core::Bundle::SBundleHeaderItem& header ) const
{
	HANDLE fileHandle = ::CreateFileA( path.AsChar(), 0, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );

	if ( fileHandle != INVALID_HANDLE_VALUE )
	{
		FillInDataSize( fileHandle, header );
		FillInModificationTime( fileHandle, header );

		CloseHandle( fileHandle );

		return true;
	}

	return false;
}

void CBundleWriterCommon::FillInDataSize( HANDLE& fileHandle, Red::Core::Bundle::SBundleHeaderItem& header ) const
{
	LARGE_INTEGER size;
	Red::System::MemorySet( &size, 0, sizeof( LARGE_INTEGER ) );
	::GetFileSizeEx( fileHandle, &size );

	header.m_dataSize = static_cast< Uint32 >( size.QuadPart );
}

void CBundleWriterCommon::FillInModificationTime( HANDLE& fileHandle, Red::Core::Bundle::SBundleHeaderItem& header ) const
{
	// Initialize the time
	FILETIME systemFormat;
	Red::System::MemoryZero( &systemFormat, sizeof( FILETIME ) );

	// Get the time of last write access to the file ( modification time )
	::GetFileTime( fileHandle, NULL, NULL, &systemFormat );

	// Convert the file time from UTC to whatever timezone the computer is set to
	FILETIME localSystemFormat;
	Red::System::MemoryZero( &localSystemFormat, sizeof( FILETIME ) );

	FileTimeToLocalFileTime( &systemFormat, &localSystemFormat );

	// Convert the value into a format compatible with our DateTime
	SYSTEMTIME dateFormat;
	Red::System::MemoryZero( &dateFormat, sizeof( FILETIME ) );

	::FileTimeToSystemTime( &localSystemFormat, &dateFormat );

	// Fill in the return value
	header.m_lastModified.SetYear			( static_cast< Uint32 >( dateFormat.wYear ) );
	header.m_lastModified.SetMonth			( static_cast< Uint32 >( dateFormat.wMonth ) - 1 );
	header.m_lastModified.SetDay			( static_cast< Uint32 >( dateFormat.wDay ) - 1 );
	header.m_lastModified.SetHour			( static_cast< Uint32 >( dateFormat.wHour ) );
	header.m_lastModified.SetMinute			( static_cast< Uint32 >( dateFormat.wMinute ) );
	header.m_lastModified.SetSecond			( static_cast< Uint32 >( dateFormat.wSecond ) );
	header.m_lastModified.SetMilliSeconds	( static_cast< Uint32 >( dateFormat.wMilliseconds ) );
}


} // namespace Bundler
