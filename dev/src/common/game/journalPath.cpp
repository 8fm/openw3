/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "journalPath.h"

#include "../core/depot.h"

#include "../core/dataError.h"

IMPLEMENT_ENGINE_CLASS( CJournalPath );

THashMap< CGUID, THandle< CJournalPath > > CJournalPath::g_cache;
TSortedMap< CGUID, THandle< CJournalResource > > CJournalPath::g_resources;

CJournalPath::CJournalPath()
{
	EnableReferenceCounting();
}

CJournalPath::~CJournalPath()
{

}

THandle< CJournalPath > CJournalPath::ConstructPathFromTargetEntry( CJournalBase* entry, CJournalResource* resource, Bool forceCreation )
{
	THandle< CJournalPath > path = nullptr;

	Bool found = g_cache.Find( entry->GetGUID(), path );

	if( found )
	{
		// Validity check to make sure the contents of the cache are valid
		found = ( path->IsValid() && path->GetTarget()->GetGUID() == entry->GetGUID() );
	}

	if( !found || forceCreation )
	{
		path = ConstructPathFromTargetEntryInternal( entry, resource, nullptr );

		// Add this path to the cache
		g_cache.Set( entry->GetGUID(), path );
	}

	return path;
}

THandle< CJournalPath > CJournalPath::ConstructPathFromTargetEntryInternal( CJournalBase* entry, CJournalResource* resource, THandle< CJournalPath > child )
{
	THandle< CJournalPath > path	= new CJournalPath;
	path->m_resource				= resource;
	path->m_guid					= entry->GetGUID();
	path->m_child					= child;

	// Do we have a parent?
	CObject* parentObject = entry->GetParent();

	if ( parentObject )
	{
		// if parent object is just a link, let's find real object
		if ( parentObject->IsA< CJournalLink >() )
		{
			parentObject = static_cast< CJournalLink* >( parentObject )->GetLinkedObject();
			CJournalChildBase* childEntry = Cast< CJournalChildBase >( entry );
			if ( childEntry != nullptr )
			{
				childEntry->SetLinkedParentGUID( static_cast< CJournalLink* >( parentObject )->GetGUID() );
			}
		} 

		// Is the parent simply another journal item?
		if( parentObject->IsA< CJournalContainer >() )
		{
			CJournalContainer* container = static_cast< CJournalContainer* >( parentObject );

			path = ConstructPathFromTargetEntryInternal( container, resource, path );
		}

		// Are we at the top of this particular file?
		else if( parentObject->IsA< CJournalResource >() )
		{
			CJournalResource* resource = static_cast< CJournalResource* >( parentObject );

			path->m_resource = resource;

			CJournalBase* parentBase = resource->Get();

			// Do we have parent in another file?
			if( parentBase->IsA< CJournalChildBase >() )
			{
				CJournalChildBase* parent = static_cast< CJournalChildBase* >( parentBase );

#ifdef JOURNAL_PATH_PRECACHE_RESOURCES
				ConstructPathFromTargetEntryInternalGame( path, parent );
#else
				ConstructPathFromTargetEntryInternalEditor( path, resource, parent );
#endif
			}
		}
	}

	return path;
}

#ifdef JOURNAL_PATH_PRECACHE_RESOURCES
void CJournalPath::ClearResources()
{
	g_resources.Clear();
}

Bool CJournalPath::LoadResources( const Char* journalDepotPath )
{
	RED_FATAL_ASSERT( g_resources.Size() == 0, "Journal Path resources have already been loaded!" );
	return UpdateResources( journalDepotPath );
}

Bool CJournalPath::UpdateResources( const Char* journalDepotPath )
{
	CDirectory* journalDirectory = GDepot->FindPath( journalDepotPath );

	if( journalDirectory )
	{
		const Uint32 totalNumFiles = CountResources( journalDirectory );

		g_resources.Reserve( g_resources.Size() + totalNumFiles );

		LoadResources( journalDirectory );

		return true;
	}

	return false;
}

Uint32 CJournalPath::CountResources( const CDirectory* directory )
{
	const TDirs& subDirectories = directory->GetDirectories();

	Uint32 count = 0;

	for( const CDirectory* subDir : subDirectories )
	{
		count += CountResources( subDir );
	}

	const TFiles& files = directory->GetFiles();

	count += files.Size();

	return count;
}

void CJournalPath::LoadResources( const CDirectory* directory )
{
	// Recurse to sub directories
	const TDirs& subDirectories = directory->GetDirectories();

	for( const CDirectory* subDir : subDirectories )
	{
		LoadResources( subDir );
	}

	const TFiles& files = directory->GetFiles();
	const Uint32 numFiles = files.Size();

	for( CDiskFile* file : files )
	{
		THandle< CResource > resource = file->Load();

		if ( resource )
		{
			if( resource->IsA< CJournalResource >() )
			{
				THandle< CJournalResource > journalResource = Cast< CJournalResource >( resource );

				CJournalBase* topLevelEntryInFile = journalResource->Get();

				RED_FATAL_ASSERT( !g_resources.KeyExist( topLevelEntryInFile->GetGUID() ), "Journal resource already cached (%ls)", resource->GetDepotPath().AsChar() );
				RED_FATAL_ASSERT( g_resources.FindByValue( journalResource ) == g_resources.End(), "Journal resource already cached (%ls)", resource->GetDepotPath().AsChar() );

				g_resources.Insert( topLevelEntryInFile->GetGUID(), journalResource );
			}
			else
			{
				DATA_HALT( DES_Major, resource, TXT( "Journal" ), TXT( "Non-Journal related file in the journal directory: %ls" ), directory->GetAbsolutePath().AsChar() );
				file->Unload();
			}
		}
	}
}

void CJournalPath::ConstructPathFromTargetEntryInternalGame( THandle< CJournalPath >& path, CJournalChildBase* parent )
{
	THandle< CJournalResource >* journalResource = g_resources.FindPtr( parent->GetParentGUID() );

	RED_FATAL_ASSERT( journalResource, "Journal resources have not been loaded (Please make sure CJournalPath::LoadResources has been called) or specified journal resource is missing" );

	CJournalBase* base = (*journalResource)->Get();

	RED_FATAL_ASSERT( base->GetGUID() == parent->GetParentGUID(), "CJournalPath::g_resources is invalid" );

	path = ConstructPathFromTargetEntryInternal( base, *journalResource, path );
}

#else

 void CJournalPath::ConstructPathFromTargetEntryInternalEditor( THandle< CJournalPath >& path, CJournalResource* parentObjectResource, CJournalChildBase* parent )
{
	// We'll need to search the directory manually and cache them as we go along
	const TFiles& filesInDirectory = parentObjectResource->GetFile()->GetDirectory()->GetFiles();

	Bool success = false;

	for ( CDiskFile* file : filesInDirectory )
	{
		THandle< CResource > resource = GDepot->LoadResource( file->GetDepotPath() );
		ASSERT( resource );
		if ( !resource )
		{
			continue;
		}
		ASSERT( resource->IsA< CJournalResource >() );

		THandle< CJournalResource > journalResource = Cast< CJournalResource >( resource );
		CJournalBase* base = journalResource->Get();

		ASSERT( journalResource->Get()->IsA< CJournalBase >() );

		// TODO: check to see if these files contain multiple levels?

		if( base->GetGUID() == parent->GetParentGUID() )
		{
			if ( base->IsA< CJournalLink >() )
			{
				journalResource = static_cast< CJournalLink* >( base )->GetLinkedResource();
				base = journalResource->Get();
				parent->SetLinkedParentGUID( base->GetGUID() );
			}
			path = ConstructPathFromTargetEntryInternal( base, journalResource, path );
			success = true;
			break;
		}
	}

	// RED_FATAL_ASSERT( success, "Didn't find resource" );
	// why was it fatal? and why was it an assert in the first place? assertions are for checking code correctness, not the data
	#ifdef RED_LOGGING_ENABLED
		if ( false == success )
		{
			RED_LOG( Journal, TXT("ConstructPathFromTargetEntryInternalEditor(): did not found the resource, parentObjectResource was '%s'"), parentObjectResource ? parentObjectResource->GetFriendlyName().AsChar() : TXT("nullptr") );
		}
	#endif
}
#endif // JOURNAL_PATH_PRECACHE_RESOURCES

Bool CJournalPath::IsValid() const
{
	if( !m_resource.IsEmpty() )
	{
		if( GDepot->FindFileUseLinks( m_resource.GetPath(), 0 ) )
		{
			const_iterator iter = Begin();

			while( iter != End() )
			{
				if( iter->GetGUID() != iter.GetCurrentPosition()->m_guid )
				{
					return false;
				}

				++iter;
			}

			return true;
		}
	}

	return false;
}

void CJournalPath::OnPreSave()
{
}

void CJournalPath::OnPostLoad()
{
}

Bool CJournalPath::OnPropertyTypeMismatch( CName propertyName, IProperty* existingProperty, const CVariant& readValue )
{
	RED_LOG_ERROR( JournalPath, TXT( "%ls" ), propertyName.AsChar() );

	return false;
}

void CJournalPath::DebugPrintTree( Uint32 recursionLevel )
{
	String guid = ToString( m_guid );

	const String& resource = m_resource.GetPath();

#define INDENTATION_CHARS TXT( "               " )

	RED_LOG_SPAM( JournalPath, TXT( "%.*ls-- %ls (%ls)" ), recursionLevel, INDENTATION_CHARS, guid.AsChar(), resource.AsChar() );

	if( m_child.IsValid() )
	{
		m_child->DebugPrintTree( recursionLevel + 2 );
	}
}

void CJournalPath::SaveGame( IGameSaver* saver ) 
{
	CGameSaverBlock block( saver, CNAME( path ) );
	SaveGameRecursively( saver );
}


void CJournalPath::SaveGameRecursively( IGameSaver* saver )
{
	saver->WriteValue( CNAME( guid ), m_guid );

	const String& res = m_resource.GetPath();
	saver->WriteValue( CNAME( resource ), res );

	CJournalPath* child = m_child.Get();
	Uint8 flags = child ? FLAG_Nothing : FLAG_NoMoreChildren;
	saver->WriteValue( CNAME( flags ), flags );

	if ( child )
	{
		child->SaveGameRecursively( saver ); 
	}
}

/* static */ CJournalPath* CJournalPath::LoadGame( IGameLoader* loader )
{
	CGameSaverBlock block( loader, CNAME( path ) );

	CGUID targetEntryGUID;
	CJournalPath* loadedPath = LoadGameRecursively( loader, targetEntryGUID, 0 );
	if ( loadedPath && false == targetEntryGUID.IsZero() )
	{
		g_cache.Set( targetEntryGUID, loadedPath );
		return loadedPath;
	}

	return nullptr;
}

/* static */ CJournalPath* CJournalPath::LoadGameRecursively( IGameLoader* loader, CGUID& targetEntryGUID, Uint32 recursionLevel )
{
	// sanity check
	if ( recursionLevel > 32 )
	{
		RED_ASSERT( false, TXT("LoadGameRecursively(): STOP to prevent infinite recursion.") );
		return nullptr;
	}

	CGUID guid;
	loader->ReadValue( CNAME( guid ), guid );

	String res;
	loader->ReadValue( CNAME( resource ), res );

	Uint8 flags = 0;
	loader->ReadValue( CNAME( flags ), flags );

	if ( flags & FLAG_NoMoreChildren )
	{
		targetEntryGUID = guid;
		return CJournalPath::Create( guid, res, nullptr ); 
	}
	else
	{
		return CJournalPath::Create( guid, res, LoadGameRecursively( loader, targetEntryGUID, recursionLevel + 1 ) ); 
	}
}

/* static */ CJournalPath* CJournalPath::Create( const CGUID& guid, const String& res, CJournalPath* child )
{
	CJournalPath* path = new CJournalPath;
	path->m_guid = guid;
	path->m_resource = res;
	path->m_child = child;
	return path;
}
