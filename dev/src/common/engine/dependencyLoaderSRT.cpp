/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "../core/dependencyLinkerFactory.h"
#include "dependencyLoaderSRT.h"
#include "baseTree.h"

Red::Threads::CMutex CDependencyLoaderSRT::m_lock;

CDependencyLoaderSRT::CDependencyLoaderSRT( IFile& file, const CDiskFile* diskFile )
	: IDependencyLoader( file, diskFile )
	, m_filePath( diskFile->GetDepotPath() )
{
}

Bool CDependencyLoaderSRT::LoadObjects( DependencyLoadingContext& context )
{
	RED_FATAL_ASSERT( context.m_firstExportMemory != nullptr, "No resource memory specified" );
	RED_FATAL_ASSERT( context.m_firstExportMemorySize >= sizeof(CSRTBaseTree), "Resource memory is to small" );

	RED_FATAL_ASSERT( context.m_firstExportMemory->IsA< CSRTBaseTree >(), "Invalid resource class passed to dependency loader" );
	CSRTBaseTree* tree = static_cast< CSRTBaseTree* >( context.m_firstExportMemory );

	RED_FATAL_ASSERT( tree->IsLoading(), "Resource is not loading yet we are in LoadObjects" );
	RED_FATAL_ASSERT( tree->GetFile() != nullptr, "Resource is not bound yet we are in LoadObjects" );

	// Serialize SRT
	const Uint32 fileSize = (const Uint32) m_file->GetSize();
	Red::UniqueBuffer buffer = Red::CreateUniqueBuffer( fileSize, 16, MC_FoliageTree );

	m_file->Serialize( buffer.Get(), fileSize );
	tree->SetSRTData( std::move( buffer ) );

	{
		// ctremblay Speed Tree is everything but thread safe.
		// This lock is made to fix one specific issue I found. See GraphicsApiAbstractionRI.h line 301
		// In rare case, 2 threads can modified that static variable, m_pCache ( one thread prefetching foliage, another thread loading herb for example )
		// Real fix would be to make this specific part in Speed Tree thread safe. However I fear there are other Thread safety issue there. 
		// I do not want to be in an infinite loop of memory stomp or other issue. 
		// Except if this fix has major performance issue, I most likely won't revisit this fix for Witcher3.
		Red::Threads::CScopedLock< Red::Threads::CMutex > scopedLock( m_lock ); 
		tree->OnPostLoad();
	}

	
	// return loaded objects
	context.m_loadedRootObjects.PushBack( tree );
	context.m_loadedResources.PushBack( tree );

	// Done
	return true;
}

////////////////////////////////////////////////////////////////////////////////////////

CDependencyLoaderSRTFactory::CDependencyLoaderSRTFactory()
{
	SDependencyLinkerFactory::GetInstance().RegisterLoaderFactory( ResourceExtension< CSRTBaseTree >(), this );
}

CDependencyLoaderSRTFactory::~CDependencyLoaderSRTFactory()
{
	SDependencyLinkerFactory::GetInstance().UnregisterLoaderFactory( this );
}

IDependencyLoader* CDependencyLoaderSRTFactory::CreateLoader( IFile& file, const CDiskFile* diskFile ) const
{
	return new CDependencyLoaderSRT( file, diskFile );
}

////////////////////////////////////////////////////////////////////////////////////////


Bool CDependencySaverSRT::SaveObjects( DependencySavingContext& context )
{
	CSRTBaseTree* tree = Cast< CSRTBaseTree >( context.m_initialExports[0] );

#ifndef NO_RESOURCE_COOKING

	// cooking support, manual
	if ( context.m_cooker )
	{
		tree->OnCook( *context.m_cooker );
	}

#endif

	// cast away const because IFile::Serialize() needs it.
	void* data = const_cast< void* >( tree->GetSRTData() );
	size_t dataSize = tree->GetSRTDataSize();
	m_file->Serialize( data, dataSize );

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////

CDependencySaverSRTFactory::CDependencySaverSRTFactory()
{
	SDependencyLinkerFactory::GetInstance().RegisterSaverFactory( ResourceExtension< CSRTBaseTree >(), this );
}

CDependencySaverSRTFactory::~CDependencySaverSRTFactory()
{
	SDependencyLinkerFactory::GetInstance().UnregisterSaverFactory( this );
}

IDependencySaver* CDependencySaverSRTFactory::CreateSaver( IFile& file, const CDiskFile* ) const
{
	return new CDependencySaverSRT( file );
}
