/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "foliageResourceLoader.h"
#include "foliageResourceHandler.h"

#include "../core/depot.h"
#include "../core/resourcepaths.h"
#include "../core/resourceLoading.h"

#include "baseEngine.h"
#include "foliageCell.h"

const Vector2 cellDimension = Vector2( 64.0f, 64.0f ); // HACK, should be either somewhere else or provided.

using namespace Red::Core::ResourceManagement;

CSoftHandleProxy::CSoftHandleProxy()
{}

CSoftHandleProxy::CSoftHandleProxy( const String & filePath )
	: m_softHandle( filePath )
{}

CSoftHandleProxy::CSoftHandleProxy( CFoliageResource * resource )
	: m_softHandle( resource )
{}

CSoftHandleProxy::~CSoftHandleProxy()
{}

BaseSoftHandle::EAsyncLoadingResult CSoftHandleProxy::GetAsync() const
{
	return m_softHandle.GetAsync();
}

CFoliageResource * CSoftHandleProxy::Get() const
{
	return m_softHandle.Get();
}

const String& CSoftHandleProxy::GetPath() const
{
	return m_softHandle.GetPath();
}

bool CSoftHandleProxy::IsLoaded() const
{
	return m_softHandle.IsLoaded();
}

IFoliageResourceLoader::~IFoliageResourceLoader()
{}

CFoliageResourceLoader::CFoliageResourceLoader()
	:	m_resourcePath( nullptr ),
		m_handler( nullptr ),
		m_depot( nullptr ),
		m_resourceLoader( nullptr )
{}

CFoliageResourceLoader::~CFoliageResourceLoader()
{}

FoliageResourceHandle CFoliageResourceLoader::GetResourceHandle( const Vector2 & position ) const
{
	const String filePath = m_resourcePath->GetPath( CResourcePaths::Path_FoliageSourceData ) + GenerateFoliageFilename( position );
	if( m_depot->FileExist( filePath ) )
	{
		FoliageResourceHandle handle( new CSoftHandleProxy( filePath ) );
		return handle;
	}
	
	return FoliageResourceHandle();
}

void CFoliageResourceLoader::ResourceAcquired( CFoliageResource * resource, const Vector2 & position ) const
{
	resource->AddToRootSet();
	const Box box = Box( position, position + cellDimension );
	resource->SetGridBox( box );
	m_handler->DisplayFoliage( resource );
}

void CFoliageResourceLoader::ResourceReleased( CFoliageResource * resource ) const
{
	m_handler->HideFoliage( resource );
	resource->RemoveFromRootSet();
	resource->Discard();
}

FoliageResourceHandle CFoliageResourceLoader::CreateResource( const Vector2 & position ) const
{
	CFoliageResource * resource = new CFoliageResource;
	const Box box = Box( position, position + cellDimension );
	resource->SetGridBox( box );

	const String foliageDirName = m_resourcePath->GetPath( CResourcePaths::Path_FoliageSourceData );
	CDirectory* dir = m_depot->CreatePath( foliageDirName );

	const String foliageFileName = GenerateFoliageFilename( position );
	CDiskFile * file = new CDiskFile( dir, foliageFileName, resource );
	dir->AddFile( file );

#ifndef NO_FILE_SOURCE_CONTROL_SUPPORT
	file->Save(); // make the empty file appear on the disk and source control immediately
	file->Add();
#endif

	return FoliageResourceHandle( new CSoftHandleProxy( resource ) );
}

void CFoliageResourceLoader::PrefetchAllResource( const CellHandleContainer & container )
{
	TDynArray< CDiskFile * > depotEntryContainer;
	depotEntryContainer.Reserve( container.Size() );

	for( Uint32 index = 0, end = container.Size(); index != end; ++index )
	{
		const CellHandle & handle = container[ index ];

		if( handle->IsResourceValid() ) 
		{
			const String & filepath = handle->GetPath();
			CDiskFile * entry = m_depot->FindFile( filepath );
			RED_FATAL_ASSERT( entry, "CDiskFile is null. CellHandle should not be valid!" );
			depotEntryContainer.PushBack( entry );
		}
	}

	if( !depotEntryContainer.Empty() )
	{
		m_resourceLoader->Load( nullptr, depotEntryContainer.TypedData(), depotEntryContainer.Size(), eResourceLoadingPriority_Normal, nullptr );
	}
}

void CFoliageResourceLoader::SetInternalResourcePaths( const Red::Core::ResourceManagement::CResourcePaths * paths )
{
	m_resourcePath = paths;
}

void CFoliageResourceLoader::SetInternalHandler( CFoliageResourceHandler * handler )
{
	m_handler = handler;
}

void CFoliageResourceLoader::SetInternalDepot( CDepot * depot )
{
	m_depot = depot;
}

void CFoliageResourceLoader::SetInternalResourceLoader( CResourceLoader * loader )
{
	m_resourceLoader = loader;
}

String GenerateFoliageFilename( const Vector2 & position )
{
	return String::Printf( TXT( "foliage_%3.2f_%3.2f.flyr" ), position.X, position.Y );
}

Red::TUniquePtr< CFoliageResourceLoader > CreateFoliageResourceLoader( CFoliageResourceHandler * handler )
{
	Red::TUniquePtr< CFoliageResourceLoader > loader( new CFoliageResourceLoader );
	loader->SetInternalResourcePaths( &GEngine->GetPathManager() );
	loader->SetInternalHandler( handler );
	loader->SetInternalDepot( GDepot );
	loader->SetInternalResourceLoader( &SResourceLoader::GetInstance() );
	return loader;
}
