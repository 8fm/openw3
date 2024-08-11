/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "layerGroup.h"

#include "../core/dependencyLoader.h"
#include "../core/depot.h"
#include "../core/feedback.h"
#include "../core/gameSave.h"
#include "../core/messagePump.h"

#include "baseEngine.h"
#include "game.h"
#include "gameSaveManager.h"
#include "layerInfo.h"
#include "pathlibWorld.h"
#include "world.h"

#define LAYER_INFO_MAGIC		'LYRI'

IMPLEMENT_ENGINE_CLASS( CLayerGroup );
IMPLEMENT_ENGINE_CLASS( CSystemLayerGroup );

namespace
{
	// Hacky function to determine if a layer group should be considered a "system" layer group
	// The system layer groups are right now only used by the streaming tiles
	// This hacky dependency on the IsSystemGroup() flag in the loading code is very bad :(
	static Bool IsSystemGroupName( const CLayerGroup* parentGroup, const String& groupName )
	{
		if ( parentGroup && parentGroup->GetParentGroup() == nullptr ) // world
		{
			if ( groupName == TXT("streaming_tiles") )
			{
				return true;
			}
		}

		return false;
	}

	// Load the old w2lg file
	static CLayerGroup* LoadLayerGroupFile( const String& layerGroupFileName )
	{
		IFile* reader = GFileManager->CreateFileReader( layerGroupFileName.AsChar() );
		if ( reader == nullptr ) 
		{
			return nullptr;
		}

		DependencyLoadingContext context;
		context.m_getAllLoadedObjects = true; // CLayerGroup is not longer a CObject so the stanard way of extracting the loaded pointer wont work

		CDependencyLoader loader( *reader, nullptr );
		if ( !loader.LoadObjects( context ) )
		{
			delete reader;
			return nullptr;
		}

		delete reader;

		// is the loaded object a layer group ?
		if ( context.m_loadedObjects.Size() >= 1 )
		{
			if ( context.m_loadedObjects[0].GetRuntimeClass()->IsA< CLayerGroup >() )
			{
				return static_cast< CLayerGroup* >( context.m_loadedObjects[0].GetSerializablePtr() );
			}
		}

		// not a layer group (potential memory leak)
		return nullptr;
	}
}

CLayerGroup::CLayerGroup()
	: m_world( NULL )
	, m_isVisibleOnStart( true )
	, m_oldSystemGroupFlag( false )
	, m_isDLC( false )
{
}

#ifndef NO_DEBUG_PAGES

Bool CLayerGroup::GetErrorFlag() const
{
	for( auto it = m_layers.Begin(); it != m_layers.End(); ++it )
	{
		CLayerInfo* info = *it;
		if ( info->GetErrorFlag() == true )
		{
			return true;
		}
	}

	for( auto it = m_subGroups.Begin(); it != m_subGroups.End(); ++it )
	{
		CLayerGroup* group = *it;
		if ( group->GetErrorFlag() == true )
		{
			return true;
		}
	}

	return false;
}

#endif 

Uint32 CLayerGroup::CalcObjectDynamicDataSize() const
{
	Uint32 size = 0;

	for( TLayerList::const_iterator it = m_layers.Begin(); it != m_layers.End(); ++it )
	{
		CLayerInfo* info = *it;

		size += info->CalcObjectDynamicDataSize();
	}

	for( TGroupList::const_iterator it = m_subGroups.Begin(); it != m_subGroups.End(); ++it )
	{
		CLayerGroup* group = *it;

		size += group->CalcObjectDynamicDataSize();
	}

	return size;
}

Uint32 CLayerGroup::CalcDataSize() const
{
	Uint32 size = 0;

	for( TLayerList::const_iterator it = m_layers.Begin(); it != m_layers.End(); ++it )
	{
		CLayerInfo* info = *it;

		if ( info->GetLayer() )
		{
			size += info->GetLayer()->CalcDataSize();
		}
	}

	for( TGroupList::const_iterator it = m_subGroups.Begin(); it != m_subGroups.End(); ++it )
	{
		CLayerGroup* group = *it;

		size += group->CalcDataSize();
	}

	return size;
}

void CLayerGroup::CalcMemSnapshot( SLayerGroupMemorySnapshot& snapshots ) const
{
	snapshots.m_name = GetName();
	snapshots.m_loaded = IsLoaded();

	snapshots.m_memStatic = GetClass()->GetSize();
	snapshots.m_memSerialize = 0; // layer groups are not longer saved
	snapshots.m_memDynamic = 0;

	snapshots.m_entitiesNum = 0;
	snapshots.m_attachedCompNum = 0;

	snapshots.m_subGroupsSnapshot.Reserve( m_subGroups.Size() );
	//snapshots.m_layersSnapshot.Reserve( m_layers.Size() );

	for( TLayerList::const_iterator it = m_layers.Begin(); it != m_layers.End(); ++it )
	{
		CLayerInfo* info = *it;

		if ( info->GetLayer() )
		{
			Uint32 index = static_cast< Uint32 >( snapshots.m_layersSnapshot.Grow( 1 ) );
			SLayerMemorySnapshot& s = snapshots.m_layersSnapshot[ index ];

			info->GetLayer()->CalcMemSnapshot( s );

			snapshots.m_memStatic += s.m_memStatic;
			snapshots.m_memSerialize += s.m_memSerialize;
			snapshots.m_memDynamic += s.m_memDynamic;

			snapshots.m_entitiesNum = s.m_entitiesNum;
			snapshots.m_attachedCompNum = s.m_attachedCompNum;
		}
	}

	for( TGroupList::const_iterator it = m_subGroups.Begin(); it != m_subGroups.End(); ++it )
	{
		CLayerGroup* group = *it;

		Uint32 index = static_cast< Uint32 >( snapshots.m_subGroupsSnapshot.Grow( 1 ) );
		SLayerGroupMemorySnapshot& s = snapshots.m_subGroupsSnapshot[ index ];

		group->CalcMemSnapshot( s );

		snapshots.m_memStatic += s.m_memStatic;
		snapshots.m_memSerialize += s.m_memSerialize;
		snapshots.m_memDynamic += s.m_memDynamic;

		snapshots.m_entitiesNum = s.m_entitiesNum;
		snapshots.m_attachedCompNum = s.m_attachedCompNum;
	}
}

void CLayerGroup::OnSerialize( IFile& file )
{
	ISerializable::OnSerialize( file );

	// Cooked stuff
	if ( m_hasEmbeddedLayerInfos )
	{
		file << m_world;
		file << m_parentGroup;
		file << m_subGroups;
		file << m_layers;
	}
	else if ( file.IsGarbageCollector() )
	{
		for ( Uint32 i=0; i<m_layers.Size(); ++i )
		{
			CLayerInfo* layer = m_layers[i];
			layer->OnSerialize( file );
		}

		for ( Uint32 i=0; i<m_subGroups.Size(); ++i )
		{
			CLayerGroup* group = m_subGroups[i];
			group->OnSerialize( file );
		}
	}
}

#ifndef NO_DATA_VALIDATION
void CLayerGroup::OnCheckDataErrors() const
{
	for ( auto it = m_layers.Begin(); it != m_layers.End(); ++it )
	{
		if ( (*it)->GetLayer() )
		{
			(*it)->GetLayer()->OnCheckDataErrors();
		}
	}

	for ( auto it = m_subGroups.Begin(); it != m_subGroups.End(); ++it )
	{
		(*it)->OnCheckDataErrors();
	}
}
#endif

void CLayerGroup::ComputeIDHash()
{
	m_idHash = Red::System::CalculateHash64( m_depotPath.Data(), m_depotPath.DataSize() );
}

CLayerGroup::CLayerGroup( CWorld* world, CLayerGroup* parentGroup, const String& name, const String& depotFilePath, const String& absoluteFilePath )
	: m_name( name )
	, m_parentGroup( parentGroup )
	, m_world( world  )
	, m_depotPath( depotFilePath )
	, m_absolutePath( absoluteFilePath )
	, m_isVisibleOnStart( true )
	, m_oldSystemGroupFlag( false )
{
	ComputeIDHash();
	m_world->RegisterLayerGroup( this );
	m_isDLC = parentGroup != nullptr && parentGroup->IsDLC();
}

CLayerGroup::~CLayerGroup()
{
	if ( m_world )
	{
		m_world->UnregisterLayerGroup( this );
	}

	TLayerList tempLayers;
	TGroupList tempGroups;
	
	Swap( m_layers, tempLayers );
	Swap( m_subGroups, tempGroups );

	for ( Uint32 i=0; i<tempLayers.Size(); ++i )
	{
		CLayerInfo* layer = tempLayers[i];
		delete layer;
	}

	for ( Uint32 i=0; i<tempGroups.Size(); ++i )
	{
		CLayerGroup* group = tempGroups[i];
		delete group;
	}
}

Bool CLayerGroup::CanSave() const
{
	return !IsRootGroup() && !IsDLC();
}

void CLayerGroup::LinkToWorld( CWorld* world )
{
	// Link to world
	ASSERT( !m_world || m_world == world );
	m_world = world;

	m_world->RegisterLayerGroup( this );

	// Link layer infos
	for ( Uint32 i=0; i<m_layers.Size(); ++i )
	{
		CLayerInfo* info = m_layers[i];
		info->LinkToWorld( world );
	}

	// Recurse to sub groups
	for ( Uint32 i=0; i<m_subGroups.Size(); ++i )
	{
		CLayerGroup* subGroup = m_subGroups[i];
		subGroup->LinkToWorld( world );
	}
}

void CLayerGroup::Populate()
{
	// Fill from disk structure
	CDirectory * groupDir = GDepot->FindPath( m_depotPath.AsChar() );
	if ( groupDir )
	{
		// Find sub directories
		for ( CDirectory * pDir : groupDir->GetDirectories() )
		{
			String name = pDir->GetName();

			String absoluteDirPathName = m_absolutePath + name + TXT("\\");
			String depotDirPathName = m_depotPath + name + TXT("\\");

			// If the directory is not empty create the layer group object
#if !defined( NO_EDITOR ) && !defined( RED_PLATFORM_CONSOLE )
			if ( !GFileManager->FileExist( absoluteDirPathName ) )
			{
				continue;
			}
#endif

			// Is this a system group
			const Bool isSystemGroupFlag = IsSystemGroupName( this, name );
			
			// Create new group
			CLayerGroup* subGroup = isSystemGroupFlag
				? new CSystemLayerGroup( GetWorld(), this, name, depotDirPathName, absoluteDirPathName )
				: new CLayerGroup( GetWorld(), this, name, depotDirPathName, absoluteDirPathName );

			// Old layer group enumeration scheme - extract data from old layer group file
			if ( !m_world->IsNewLayerGroupFormat() )
			{
				const String layerGroupFile = depotDirPathName + TXT("group.w2lg");
			
				// Try to open existing group
				Bool isSystemGroup = false;
				Bool isInitiallyVisible = true;

				// Load the layer group data
				CLayerGroup* existingGroup = LoadLayerGroupFile( layerGroupFile );
				if ( existingGroup )
				{
					isSystemGroup = existingGroup->IsOldSystemGroupFlagSet();
					isInitiallyVisible = existingGroup->IsVisibleOnStart();
					delete existingGroup;
				}

				// Remember the groups that are initially hidden
				if ( !isInitiallyVisible )
				{
					LOG_ENGINE( TXT("LayerGroup conversion: group '%ls' is initially hidden"), layerGroupFile.AsChar() );
					m_world->SetLayerGroupInitialVisibility( subGroup, false );
					subGroup->m_isVisibleOnStart = false;
				}

				// System group flag check
				if ( isSystemGroup )
				{
					LOG_ENGINE( TXT("LayerGroup conversion: group '%ls' is marked as system group"), layerGroupFile.AsChar() );
					if ( subGroup->IsSystemGroup() != isSystemGroup )
					{
						LOG_ENGINE( TXT("LayerGroup conversion: group '%ls' system flag mismatch (was: %d, is:%d)"), 
							layerGroupFile.AsChar(), isSystemGroup, subGroup->IsSystemGroup() );
					}
				}		
			}
			else
			{
				// extract the visibility flag from world layer group list
				subGroup->m_isVisibleOnStart = m_world->GetLayerGroupInitialVisibility( subGroup );
			}

			// Add to list of existing groups
			subGroup->Populate();	
			m_subGroups.PushBack( subGroup );			
		}

		// Scan for layer files
		for ( CDiskFile* file : groupDir->GetFiles() )
		{
			if ( file->GetFileName().EndsWith( TXT("w2l") ) )
			{
				CLayerInfo* info = GrabDynamicLayerInfo( file->GetDepotPath() );
				if ( info )
				{
					m_layers.PushBack( info );
				}
			}
		}

		// Sort layers and layer groups by name
		::Sort( m_subGroups.Begin(), m_subGroups.End(), []( CLayerGroup* a, CLayerGroup* b ) { return a->GetName() < b->GetName(); } );
		::Sort( m_layers.Begin(), m_layers.End(), []( CLayerInfo* a, CLayerInfo* b ) { return a->GetDepotPath() < b->GetDepotPath(); } );
	}
}

void CLayerGroup::PopulateFromDLC()
{
	ASSERT( IsRootGroup(), TXT("PopulateFromDLC is supposed to be called only from the root layergroup") );

	// Find the DLCs directory
	CDirectory* dlcDir = GDepot->FindPath( TXT("dlc\\") );
	if ( dlcDir == nullptr )
	{
		return;
	}

	// Toplevel DLC group, created on first use
	ASSERT( FindGroup( TXT("DLC") ) == nullptr, TXT("Cannot have a dlc toplevel group!") );
	CLayerGroup* dlc = nullptr;

	// Scan each subdirectory for content for this world
	String worldName = GetWorld()->GetFile()->GetDirectory()->GetName();
	for ( CDirectory* dlcSubdir : dlcDir->GetDirectories() )
	{
		CDirectory* dlcContent = GDepot->FindPath( String::Printf( TXT("dlc\\%ls\\levels\\%ls\\"), dlcSubdir->GetName().AsChar(), worldName.AsChar() ) );
		if ( dlcContent == nullptr )
		{
			continue;
		}

		LOG_ENGINE( TXT("Adding DLC world directory '%ls' for world '%ls'"), dlcContent->GetDepotPath().AsChar(), worldName.AsChar() );

		// Check if we need to create a toplevel DLC subgroup
		if ( dlc == nullptr )
		{
			// Note: we need to add DLC\\ to the paths (even though they are otherwise unused) for unique ID generation
			dlc = new CLayerGroup( GetWorld(), this, TXT("DLC"), GetDepotPath() + TXT("DLC\\"), GetAbsolutePath() + TXT("DLC\\") );
			dlc->m_isDLC = true;
			m_subGroups.PushBack( dlc );
		}

		// Add a subgroup for this DLC
		CLayerGroup* dlcGroup = new CLayerGroup( GetWorld(), dlc, dlcSubdir->GetName(), dlcSubdir->GetDepotPath(), dlcSubdir->GetAbsolutePath() );
		dlc->m_subGroups.PushBack( dlcGroup );

		// Scan the world DLC directories
		for ( CDirectory* groupDir : dlcContent->GetDirectories() )
		{
			// Create subgroup
			CLayerGroup* subGroup = new CLayerGroup( GetWorld(), dlcGroup, groupDir->GetName(), groupDir->GetDepotPath(), groupDir->GetAbsolutePath() );
			subGroup->Populate();
			subGroup->m_isVisibleOnStart = m_world->GetLayerGroupInitialVisibility( subGroup );
			dlcGroup->m_subGroups.PushBack( subGroup );
		}
	}

	// Sort groups added from DLCs
	if ( dlc != nullptr )
	{
		// Sort DLC entries (since these entries weren't added by dlc's own Populate)
		::Sort( dlc->m_subGroups.Begin(), dlc->m_subGroups.End(), []( CLayerGroup* a, CLayerGroup* b ) { return a->GetName() < b->GetName(); } );
		::Sort( dlc->m_layers.Begin(), dlc->m_layers.End(), []( CLayerInfo* a, CLayerInfo* b ) { return a->GetDepotPath() < b->GetDepotPath(); } );

		// Sort toplevel entries to put the dlc subgroup in the proper place
		::Sort( m_subGroups.Begin(), m_subGroups.End(), []( CLayerGroup* a, CLayerGroup* b ) { return a->GetName() < b->GetName(); } );
		::Sort( m_layers.Begin(), m_layers.End(), []( CLayerInfo* a, CLayerInfo* b ) { return a->GetDepotPath() < b->GetDepotPath(); } );
	}
}

CLayerInfo* CLayerGroup::GrabDynamicLayerInfo( const String& depotPath )
{
	CLayerInfo* layerInfo = GrabRawDynamicLayerInfo( depotPath );
	if ( layerInfo )
	{
		layerInfo->Init( GetWorld(), this, depotPath );
	}
	return layerInfo;
}

CLayerInfo* CLayerGroup::GrabRawDynamicLayerInfo( const String& depotPath )
{
	// Open file
	CDiskFile *diskFile = GDepot->FindFile( depotPath );
	if ( !diskFile )
	{
		// no file in depo
		return NULL;
	}

	// Create file reader
	Red::TScopedPtr< IFile > file( diskFile->CreateReader() );
	if ( !file )
	{
		// Invalid file to open
		return NULL;
	}

	// Extract the valid CLayerInfo if we have it
	{
		// Get the file marker
		Uint32 marker[ 2 ] = { 0, 0 };
		if( file->GetSize() >= sizeof( marker ) ) // this is not a valid layer for sure (the file is smaller than the marker).
		{
			file->Seek( file->GetSize() - 8 );
			file->Serialize( &marker, sizeof( marker ) );

			// Is this a valid layer ?
			if ( marker[0] == LAYER_INFO_MAGIC )
			{
				// Seek to layer info
				file->Seek( marker[1] );

				// Load layer info data
				CDependencyLoader loader( *file, NULL );
				DependencyLoadingContext loadingContext;
				loadingContext.m_getAllLoadedObjects = true;
				if ( loader.LoadObjects( loadingContext ) )
				{
					// Post load initialization
					loader.PostLoad();
		
					// Grab the layer info
					if ( !loadingContext.m_loadedObjects.Empty() )
					{
						CLayerInfo* info = SafeCast< CLayerInfo >( loadingContext.m_loadedObjects[0].GetSerializablePtr() );
						return info;
					}
				}
			}
		}
	}

	// No valid layer info
	ERR_CORE( TXT("No valid layer info found in '%ls', creating default one"), depotPath.AsChar() );

	// Create empty layer info
	CLayerInfo* newLayerInfo = new CLayerInfo();
	return newLayerInfo;
}

// Load list of layers
Bool LoadLayerList( const TDynArray< CLayerInfo* >& layersToLoad, LayerGroupLoadingContext& loadingContext )
{
	// Reset stats
	CFileLoadingStats loadingStats;

	// Begin user block with a progress bar
	GFeedback->BeginTask( TXT("Loading world"), true );
	
	// Load layers
	for ( Uint32 i=0; i<layersToLoad.Size(); ++i )
	{
		CLayerInfo* layer = layersToLoad[i];
		if ( layer )
		{
			// Update progress
			GFeedback->UpdateTaskInfo( TXT("Loading '%ls'..."), layer->GetShortName().AsChar() );
			GFeedback->UpdateTaskProgress( i, layersToLoad.Size() );

			// Dump stats for cooker
			if ( GIsCooker )
			{
				LOG_ENGINE( TXT("Loading layer '%ls'..."), layer->GetShortName().AsChar() );
			}

			// Load the layer
			LayerLoadingContext layerLoadingContext;
			layerLoadingContext.m_stats = loadingContext.m_dumpStats ? &loadingStats : NULL;
			layerLoadingContext.m_loadHidden = loadingContext.m_loadHidden;
			
			layer->SyncLoad( layerLoadingContext );
			// Canceled 
			if ( GFeedback->IsTaskCanceled() )
			{
				// Show log
				LOG_ENGINE( TXT("Canceling world load...") );

				// Unload already loaded layers
				GFeedback->EndTask();
				return false;
			}
		}
	}

	// End of load
	GFeedback->UpdateTaskInfo( TXT("World loaded!") );
	GFeedback->UpdateTaskProgress( 100, 100 );

	// Flush all pending jobs
	GEngine->FlushJobs();
	//SJobManager::GetInstance().FlushPendingJobs();

	// Dump loading stats grabbed during the loading
	if ( loadingContext.m_dumpStats )
	{
		loadingStats.Dump();
	}

	// Done
	GFeedback->EndTask();

	// Loaded without errors
	return true;
}

void CLayerGroup::ListAllLayers( TList< CLayerInfo* > &layers, Bool recursive )
{
	// Collect layers from group
	for ( Uint32 i=0; i<m_layers.Size(); i++ )
	{
		CLayerInfo* layer = m_layers[i];
		layers.PushBack( layer );
	}
	
	// Recurse to sub groups
	if ( recursive )
	{
		for ( Uint32 i=0; i<m_subGroups.Size(); i++ )
		{
			m_subGroups[ i ]->ListAllLayers( layers, recursive );
		}
	}
}

void CLayerGroup::ListAllLayers( TDynArray< CLayerInfo* > &layers, Bool recursive )
{
	// Collect layers from group
	for ( Uint32 i=0; i<m_layers.Size(); i++ )
	{
		CLayerInfo* layer = m_layers[i];
		layers.PushBack( layer );
	}

	// Recurse to sub groups
	if ( recursive )
	{
		for ( Uint32 i=0; i<m_subGroups.Size(); i++ )
		{
			m_subGroups[ i ]->ListAllLayers( layers, recursive );
		}
	}
}

void CLayerGroup::ListAllVisibleLayers( TDynArray< CLayerInfo* > &layers, Bool recursive )
{
	// Collect layers from group
	for ( Uint32 i=0; i<m_layers.Size(); i++ )
	{
		CLayerInfo* layer = m_layers[i];
		if (layer->GetMergedVisiblityFlag())
		{
			layers.PushBack( layer );
		}
	}

	// Recurse to sub groups
	if ( recursive )
	{
		for ( Uint32 i=0; i<m_subGroups.Size(); i++ )
		{
			m_subGroups[ i ]->ListAllVisibleLayers( layers, recursive );
		}
	}
}

#ifndef NO_EDITOR_WORLD_SUPPORT

void CLayerGroup::Sync()
{
	// Load layers
	LayerLoadingContext context;
	SyncLoad( context );
	m_world->DelayedActions();

	String dirLocalPath;
	GDepot->ConvertToLocalPath(m_absolutePath, dirLocalPath);
	CDirectory* dir = GDepot->FindPath( dirLocalPath.AsChar() );
	if (dir) dir->Sync();

	for ( Int32 i = (Int32)m_subGroups.Size()-1; i>=0 ; i-- )
	{
		m_subGroups[i]->Sync();
	}

	for ( Int32 i = (Int32)m_layers.Size()-1; i>=0; i-- )
	{
		m_layers[i]->GetLatest();
	}
}

void CLayerGroup::Refresh()
{
	for ( Int32 i = (Int32)m_subGroups.Size()-1; i>=0 ; i-- )
	{
		m_subGroups[i]->Refresh();
	}

	for ( Int32 i = (Int32)m_layers.Size()-1; i>=0; i-- )
	{
		m_layers[i]->Refresh();
	}
}

void CLayerGroup::Reload()
{
	for ( Uint32 i = 0; i < m_subGroups.Size(); i++ )
	{
		m_subGroups[i]->Reload();
	}

	for ( Uint32 i = 0; i < m_layers.Size(); i++ )
	{
		m_layers[i]->Reload();
	}
}

#endif

Bool CLayerGroup::IsLoaded( Bool recursive ) const
{
	// All visible layers in the group should be loaded
	for ( Uint32 i=0; i<m_layers.Size(); i++ )
	{
		CLayerInfo* info = m_layers[i];
		if ( info && info->IsVisible() && !info->IsLoaded() )
		{
			return false;
		}
	}

	if ( recursive )
	{
		for ( Uint32 i=0; i<m_subGroups.Size(); i++ )
		{
			if ( !m_subGroups[i]->IsLoaded( true ) )
				return false;
		}
	}

	// Assume group is loaded
	return true;
}

Bool CLayerGroup::IsFullyUnloaded() const
{
	// All visible layers in the group should be unloaded
	for ( Uint32 i=0; i<m_layers.Size(); i++ )
	{
		CLayerInfo* info = m_layers[i];
		if ( info && info->IsLoaded() )
		{
			return false;
		}
	}

	for ( Uint32 i=0; i<m_subGroups.Size(); i++ )
	{
		if ( !m_subGroups[i]->IsFullyUnloaded() )
		{
			return false;
		}
	}

	// The group is fully unloaded
	return true;
}

Bool CLayerGroup::IsLoading() const
{
	// If any layer is being loaded than the group is being loaded
	for ( Uint32 i=0; i<m_layers.Size(); i++ )
	{
		CLayerInfo* info = m_layers[i];
		if ( info && info->IsLoading() )
		{
			return true;
		}
	}

	// Assume group is not loading
	return false;
}

Bool CLayerGroup::IsLoadingAnyGroup() const
{
	// Check local layers
	if ( IsLoading() )
	{
		return true;
	}

	// Check sub groups
	for ( Uint32 i=0; i<m_subGroups.Size(); i++ )
	{
		if ( m_subGroups[i] && m_subGroups[i]->IsLoadingAnyGroup() )
		{
			return true;
		}
	}

	// Not loading
	return false;
}

Bool CLayerGroup::IsUnloading() const
{
	// If any layer is being unloaded than the group is being unloaded
	for ( Uint32 i=0; i<m_layers.Size(); i++ )
	{
		CLayerInfo* info = m_layers[i];
		if ( info && info->IsUnloading() )
		{
			return true;
		}
	}

	// Assume group is not unloading
	return false;
}

Bool CLayerGroup::IsSystemGroup() const
{
	if ( m_parentGroup != nullptr )
	{
			return m_parentGroup->IsSystemGroup();
	}

	return false;
}

void CLayerGroup::UpdateLoadingState()
{
	if ( m_layers.Size() > 0 )
	{
		Uint32 i=0;
		// Update all layers
		for ( ; i<m_layers.Size()-1; ++i )
		{
			m_layers[i]->UpdateLoadingState();
		}
		// Do the last
		m_layers[i]->UpdateLoadingState();
	}

	if ( m_subGroups.Size() > 0 )
	{
		Uint32 i=0;
		// Recurse to sub groups
		for ( ; i<m_subGroups.Size()-1; ++i )
		{
			m_subGroups[i]->UpdateLoadingState();
		}

		// Do the last
		m_subGroups[i]->UpdateLoadingState();
	}
}

void CLayerGroup::ResetVisiblityFlag( const CGameInfo& info )
{
	// Force base group to be loaded
	if ( !GetParentGroup() )
	{
		m_isVisibleOnStart = true;
	}

	// Reset
	m_isVisible = ( nullptr == info.m_gameLoadStream && false == info.m_isChangingWorldsInGame ) ? m_isVisibleOnStart : GGame->GetUniverseStorage()->ShouldLayerGroupBeVisible( this ); 
	//RED_LOG_SPAM( RED_LOG_CHANNEL( Engine ), TXT("Group '%ls' visibility: %d"), GetDepotPath().AsChar(), m_isVisible ? 1 : 0 );
	CPathLibWorld* pathlib = m_world->GetPathLibWorld();
	if ( pathlib )
	{
		pathlib->OnLayerEnabled( this, m_isVisible );
	}

	// Propagate
	for ( Uint32 i=0; i<m_subGroups.Size(); i++ )
	{
		CLayerGroup* group = m_subGroups[i];
		group->ResetVisiblityFlag( info );
	}
}

void CLayerGroup::SetInitialVisibility( Bool value )
{
	if ( m_isVisibleOnStart != value )
	{
		if ( m_world && m_world->SetLayerGroupInitialVisibility( this, value ) )
		{
			m_isVisibleOnStart = value;			
		}
	}
}

void CLayerGroup::GetLayerGroupPath( String& outPath ) const
{
	if ( m_parentGroup != nullptr )
	{
		m_parentGroup->GetLayerGroupPath( outPath );
		outPath += TXT("\\");
	}

	outPath += m_name;
}

void CLayerGroup::PullLayerGroupsAndLayerInfos()
{	
	for ( Uint32 i=0; i<m_layers.Size(); ++i )
	{
		CLayerInfo* layer = m_layers[i];
		layer->PullLayerGroupsAndLayerInfo();
	}

	for ( Uint32 i=0; i<m_subGroups.Size(); ++i )
	{
		CLayerGroup* group = m_subGroups[i];
		group->PullLayerGroupsAndLayerInfos();
	}

	m_hasEmbeddedLayerInfos = true;
}

#ifndef NO_EDITOR
Bool CLayerGroup::Rename( const String& newName )
{
	// Try to unload all layers (otherwise we won't be able to copy files)
	Bool wasLoaded = IsLoaded( true );
	if ( wasLoaded && !SyncUnload() )
	{
		return false;
	}

	// Create new directory
	CDirectory* dir = GetDepotDirectory();
	CDirectory* newDir = nullptr;
	if ( dir && dir->GetParent() )
	{
		newDir = dir->GetParent()->CreateNewDirectory( newName );
	}

	// failed to create new directory
	if ( !newDir )
	{
		return false;
	}

	// Update names
	MoveContents( newDir );

	// If the group was loaded before, load it
	if ( wasLoaded )
	{
		SyncLoad( LayerLoadingContext() );
	}

	return true;
}

void CLayerGroup::MoveContents( CDirectory* destDir )
{
	m_name = destDir->GetName();
	m_depotPath = destDir->GetDepotPath();
	m_absolutePath = destDir->GetAbsolutePath();

	for ( Uint32 i = 0; i < m_layers.Size(); ++i )
	{
		CLayerInfo* layer = m_layers[i];
		if ( CDiskFile* file = GDepot->FindFile( layer->GetDepotPath() ) )
		{
			if ( file->Copy( destDir, file->GetFileName() ) )
			{
				layer->UpdatePath();
				if ( CDiskFile* copiedFile = GDepot->FindFile( layer->GetDepotPath() ) )
				{
					copiedFile->Add();
					file->Delete( false, false );
				}
			}
		}
	}

	for ( Uint32 i = 0; i < m_subGroups.Size(); ++i )
	{
		if ( CDirectory* newDir = destDir->CreateNewDirectory( m_subGroups[i]->GetName() ) )
		{
			m_subGroups[i]->UpdatePath();
			m_subGroups[i]->MoveContents( newDir );
		}
	}
}

void CLayerGroup::UpdatePath()
{
	if ( m_parentGroup )
	{
		m_depotPath = m_parentGroup->GetDepotPath() + m_name + TXT("\\");
		m_absolutePath = m_parentGroup->GetAbsolutePath() + m_name + TXT("\\");
	}
}
#endif

void CLayerGroup::SetVisiblityFlag( Bool isVisible, Bool isInPartition, TDynArray< CLayerInfo* >& changedLayers )
{
	// Do not waste time doing nothing
	if ( m_isVisible != isVisible )
	{
		// Toggle shit
		m_isVisible = isVisible;

		// Log the changes
		String layerGroup = GetGroupPathName( NULL );
		LOG_ENGINE( TXT("LayerGroup '%ls' %s"), layerGroup.AsChar(), isVisible ? TXT("shown") : TXT("hidden") );

		// Get all layer within
		TDynArray< CLayerInfo* > layers;
		GetLayers( layers, false, true );

		// Reattach all layers
		for ( Uint32 i=0; i<layers.Size(); i++ )
		{
			CLayerInfo* info = layers[i];
			if ( info->IsLoaded() )
			{
				info->ConditionalAttachToWorld();
			}

			if( isInPartition )
			{
				if ( (info->IsLoaded() || info->IsLoading()) != info->GetMergedVisiblityFlag())
				{
					//already handled
					changedLayers.PushBack(info);
				}
			}
		}

		CPathLibWorld* pathlib = m_world->GetPathLibWorld();
		if ( pathlib )
		{
			pathlib->OnLayerEnabled( this, isVisible );
		}
	}
}

Bool CLayerGroup::SyncLoad( LayerLoadingContext& context )
{
	// Load all layers
	Bool result = true;
	for ( Uint32 i=0; i<m_layers.Size(); i++ )
	{
		CLayerInfo* info = m_layers[i];
		if ( info->IsVisible() )
		{
			result &= info->SyncLoad( context );
		}
	}
	
	// Update internals
	UpdateLoadingState();

	// Return the loading state
	return result;
}

Bool CLayerGroup::SyncUnload()
{
	// Unload all subgroups
	for ( Uint32 i=0; i<m_subGroups.Size(); i++ )
	{
		PUMP_MESSAGES_DURANGO_CERTHACK();
		CLayerGroup* subGroup = m_subGroups[i];
		subGroup->SyncUnload();
	}

	PUMP_MESSAGES_DURANGO_CERTHACK();

	// Unload all layers
	Bool result = true;
	for ( Uint32 i=0; i<m_layers.Size(); i++ )
	{
		CLayerInfo* info = m_layers[i];
		result &= info->SyncUnload();
		PUMP_MESSAGES_DURANGO_CERTHACK();
	}

	// Update internals
	UpdateLoadingState();

	// Return the loading state
	return result;
}

#ifndef NO_EDITOR_WORLD_SUPPORT

CLayerInfo* CLayerGroup::CreateLayer( const String& name, bool createAlways )
{
	return CreateCustomLayer( name, ResourceExtension< CLayer >(), createAlways );
}

CLayerInfo* CLayerGroup::CreateCustomLayer( const String& name, const String &ext, bool createAlways )
{
	// Search for duplicates
	CLayerInfo* layer = FindLayer( name );
	if ( layer )
	{
		// Recreate inner layer
		if ( createAlways )
		{
			// Layer was not found on disk, create empty layer
			if ( !layer->Create() )
			{
				WARN_ENGINE( TXT("Unable to create layer '%ls'"), name.AsChar() );
				return NULL;
			}

			// Return existing layer info
			return layer;
		}
		
		// Not created
		WARN_ENGINE( TXT("Unable to create layer '%ls': it already exists"), name.AsChar() );
		return NULL;
	}

	// Assemble layer path
	String layerAbsolutePath = String::Printf( TXT("%s%s.%s"), m_absolutePath.AsChar(), name.AsChar(), ext.AsChar() );

	// Create layer info
	CLayerInfo* newLayer = new CLayerInfo( GetWorld(), this, layerAbsolutePath );
	//newLayer->SetParent( this );

	// Try to load existing layer before creating new one
	LayerLoadingContext context;
	if ( !newLayer->SyncLoad( context ) )
	{
		// Layer was not found on disk, create empty layer
		if ( !newLayer->Create() )
		{
			WARN_ENGINE( TXT("Unable to create layer '%ls'"), name.AsChar() );
			return NULL;
		}
	}
	else
	{
		// Warn user that an existing layer was used instead of creating a new one
		WARN_ENGINE( TXT("Using existing layer '%ls'"), name.AsChar() );
	}

	// Add to layer list
	m_layers.PushBack( newLayer );

	// Done
	return newLayer;
}

void CLayerGroup::AddLayer( CLayerInfo *info )
{
	ASSERT( info );
	ASSERT( !m_layers.Exist( info ) );
	m_layers.PushBack( info );
}

#endif

CLayerInfo* CLayerGroup::FindLayer( const String& name )
{
	for ( Uint32 i=0; i<m_layers.Size(); i++ )
	{
		if ( m_layers[i] && m_layers[i]->GetShortName() == name )
		{
			return m_layers[i];
		}
	}
	return NULL;
}

CLayerInfo* CLayerGroup::FindLayer( const CGUID& guid )
{
	for ( Uint32 i = 0, count = m_layers.Size(); i < count; ++i )
	{
		if ( m_layers[i] && m_layers[i]->GetGUID() == guid )
		{
			return m_layers[i];
		}
	}

	for ( Uint32 i = 0, count = m_subGroups.Size(); i < count; ++i )
	{
		CLayerInfo* info = m_subGroups[i]->FindLayer( guid );
		if ( info != NULL )
		{
			return info;
		}
	}

	return NULL;
}

CLayerInfo* CLayerGroup::FindLayerCaseless( const String& name )
{
	for ( Uint32 i=0; i<m_layers.Size(); i++ )
	{
		if ( m_layers[i] && m_layers[i]->GetShortName().EqualsNC( name ) ) 
		{
			return m_layers[i];
		}
	}
	return NULL;
}

#ifndef NO_EDITOR_WORLD_SUPPORT

void CLayerGroup::RemoveLayer( CLayerInfo* layer )
{
	ASSERT( m_layers.Exist( layer ) );
	m_layers.Remove( layer );
}

CLayerGroup* CLayerGroup::CreateGroup( const String& name, Bool createAlways, Bool addToPerforceAlways )
{
	// Search for duplicates
	CLayerGroup* group = FindGroup( name );

	if ( group )
	{
		if ( createAlways )
		{
			return group;
		}
		WARN_ENGINE( TXT("Unable to create group '%ls': it already exists"), name.AsChar() );
		return NULL;
	}

	// Is this a "system group"
	const Bool isSystemGroup = IsSystemGroupName( this, name );

	// Create group, system group is created using different class
	String absoluteGroupPath, depotGroupPath;

	// If this is a new group created directly below a DLC group, make sure it gets the proper path
	if ( IsDLC() && GetParentGroup()->GetName() == TXT("DLC") )
	{
		depotGroupPath = String::Printf( TXT("dlc\\%ls\\levels\\%ls\\%ls\\"),
			GetName().AsChar(),											// DLC name
			GetWorld()->GetFile()->GetDirectory()->GetName().AsChar(),	// World directory name
			name.AsChar() );											// New group name
		absoluteGroupPath = GDepot->GetRootDataPath() + depotGroupPath;
	}
	else // normal group (or DLC group nested deep below the DLC groups)
	{
		absoluteGroupPath = m_absolutePath + name + TXT("\\");
		depotGroupPath = m_depotPath + name + TXT("\\");
	}
	CLayerGroup* newGroup = isSystemGroup
		? new CSystemLayerGroup( GetWorld(), this, name, depotGroupPath, absoluteGroupPath )
		: new CLayerGroup( GetWorld(), this, name, depotGroupPath, absoluteGroupPath );
	
	newGroup->Populate();
	m_subGroups.PushBack( newGroup );

	// Done
	return newGroup;
}

#endif

String CLayerGroup::GetGroupPathName( const CLayerGroup* base ) const
{
	// End
	if ( this == base )
	{
		return String::EMPTY;
	}

	// Get name from base
	CLayerGroup* baseGroup = GetParentGroup();
	String baseName = baseGroup ? baseGroup->GetGroupPathName( base ) : String::EMPTY;

	// Format final name
	if ( baseName.Empty() )
	{
		return GetName();
	}
	else
	{
		baseName += TXT("\\");
		baseName += GetName();
		return baseName;
	}
}

Uint32 CLayerGroup::GetGroupPathName( const CLayerGroup* base, AnsiChar* buffer, Uint32 bufferSize ) const
{
	if ( this == base )
	{
		return 0;
	}


	CLayerGroup* baseGroup = GetParentGroup();
	Uint32 counter = baseGroup ? baseGroup->GetGroupPathName( base, buffer, bufferSize ) : 0;

	if ( counter >= bufferSize )
	{
		return counter;
	}

	if ( 0 != counter )
	{
		buffer[ counter++ ] = '\\';
		if ( counter >= bufferSize )
		{
			return counter;
		}
	}

	const Uint32 len = m_name.GetLength();
	for ( Uint32 i = 0; i < len && counter < bufferSize; ++i )
	{
		buffer[ counter++ ] = ( AnsiChar ) m_name[ i ]; 
	}

	return counter;
}

CLayerGroup* CLayerGroup::FindGroupByPath( const String& path )
{
//TODO

	// Slice the path
	TDynArray< String > parts;
	path.Slice( parts, TXT("\\") );

	// Search for layer group
	CLayerGroup* cur = this;
	for ( Uint32 i=0; i<parts.Size(); i++ )
	{
		// Search for subgroup
		cur = cur->FindGroup( parts[i] );
		if ( !cur )
		{
			return NULL;
		}
	}

	// Found
	return cur;
}

CLayerInfo* CLayerGroup::FindLayerByPath( const String& path )
{
	// Slice the path
	TDynArray< String > parts;
	path.Slice( parts, TXT("\\") );

	// Groups ?
	CLayerGroup* cur = this;
	if ( parts.Size() > 1 )
	{
		// Search for layer group
		for ( Uint32 i=0; i<parts.Size()-1; i++ )
		{
			// Search for subgroup
			cur = cur->FindGroup( parts[i] );
			if ( !cur )
			{
				return NULL;
			}
		}
	}

	// Search for layer
	if ( cur )
	{
		const String& layerName = parts[ parts.Size()-1 ];
		return cur->FindLayerCaseless( layerName );
	}

	// Not found
	return NULL;
}

CLayerGroup* CLayerGroup::FindGroup( const String& name )
{
	for ( Uint32 i=0; i<m_subGroups.Size(); i++ )
	{
		if ( m_subGroups[i]->GetName() == name )
		{
			return m_subGroups[i];
		}
	}
	return NULL;
}

CLayerGroup* CLayerGroup::FindGroupCaseless( const String& name )
{
	for ( Uint32 i=0; i<m_subGroups.Size(); i++ )
	{
		if ( m_subGroups[i]->GetName().EqualsNC( name ) )
		{
			return m_subGroups[i];
		}
	}
	return NULL;
}

#ifndef NO_EDITOR_WORLD_SUPPORT

void CLayerGroup::RemoveGroup( CLayerGroup* layerGroup )
{
	ASSERT( m_subGroups.Exist( layerGroup ) );
	m_subGroups.Remove( layerGroup );
}

Bool CLayerGroup::Remove()
{
	// Remove all layers
	Bool allLayersRemoved = true;
	for ( Int32 i=m_layers.Size()-1; i>=0; i-- )
	{
		if ( !m_layers[i]->Remove() )
		{
			allLayersRemoved = false;
		}
	}

	// Client don't confirm all deleting decision so group layer still exists
	if ( !allLayersRemoved )
	{
		return false;
	}

	ASSERT( m_layers.Size() == 0 );

	// Recurse
	Bool allSubgroupsRemoved = true;
	for ( Int32 i=m_subGroups.Size()-1; i>=0; i-- )
	{
		if ( !m_subGroups[i]->Remove() )
		{
			allSubgroupsRemoved = false;
		}
	}

	if(!allSubgroupsRemoved)
	{
		return false;
	}

	ASSERT( m_subGroups.Size() == 0 );

	// remove the visibility flag from CSV
	SetInitialVisibility( true );

	// remove from parent
	//CLayerGroup* parentGroup = Cast< CLayerGroup >( GetParent() );
	CLayerGroup* parentGroup = GetParentGroup();
	if ( parentGroup )
	{
		ASSERT( parentGroup->m_subGroups.Exist( this ) );
		parentGroup->m_subGroups.Remove( this );
	}

	// Remove empty directory
	if (GSystemIO.RemoveDirectory( GetAbsolutePath().AsChar() ) == false)
	{
#ifdef RED_PLATFORM_WINPC
		if ( GetLastError() == ERROR_DIR_NOT_EMPTY )
		{
			WARN_ENGINE(TXT("Directory %s is not empty and cannot be removed."), GetAbsolutePath().AsChar());
		}
		else
		{
			WARN_ENGINE(TXT("Cannot remove directory %s."), GetAbsolutePath().AsChar());
		}
#endif
		
		return false;
	}
	
	return true;
}

Bool CLayerGroup::Save( Bool recursive, Bool onlyModified )
{
	// Save all layers
	if ( recursive )
	{
		Bool saved = false;
		for ( Uint32 i=0; i<m_layers.Size(); i++ )
		{
			if ( ! onlyModified || ( m_layers[i]->IsLoaded() && m_layers[i]->GetLayer()->IsModified() ) )
			{
				if ( m_layers[i]->Save() )
				{
					saved = true;
				}
			}
		}

		// Recurse
		for ( Uint32 i=0; i<m_subGroups.Size(); i++ )
		{
			if( m_subGroups[i]->Save( recursive, onlyModified ) )
			{
				saved = true;
			}
		}
		return saved;
	}
	
	return true;
}

#endif

CDirectory* CLayerGroup::GetDepotDirectory() const
{
	return GDepot->FindPath( m_depotPath.AsChar() );
}

void CLayerGroup::GetLayers(TDynArray< CLayerInfo * > &layers, Bool loadedOnly, Bool recursive, Bool nonSystem /*= false HACK STREAMING*/ )
{
	TLayerList :: iterator layer;
	for ( layer = m_layers.Begin(); layer != m_layers.End(); layer++ )
	{
		if ( !loadedOnly || (*layer)->IsLoaded() )
		{
			layers.PushBack( *layer );
		}
	}

	if( recursive )
	{
		TGroupList :: iterator group;
		for ( group = m_subGroups.Begin(); group != m_subGroups.End(); group++)
		{
			if ( !nonSystem || !(*group)->IsSystemGroup() )
			{
				(*group)->GetLayers( layers, loadedOnly, true );
			}
		}
	}
}

void CLayerGroup::GetPersistentLayers( TDynArray< CLayerInfo * > &layers, Bool loadedOnly, Bool recursive /*= true*/ )
{
	TLayerList :: iterator layer;
	for ( layer = m_layers.Begin(); layer != m_layers.End(); layer++ )
	{
		if ( !loadedOnly || (*layer)->IsLoaded() )
		{
			layers.PushBack( *layer );
		}
	}

	if( recursive )
	{
		TGroupList :: iterator group;
		for ( group = m_subGroups.Begin(); group != m_subGroups.End(); group++)
		{
			if ( (*group)->IsVisibleOnStart() )
			{
				(*group)->GetLayers( layers, loadedOnly, true );
			}
		}
	}
}

void CLayerGroup::GetLayerGroups( TDynArray< CLayerGroup* > &layerGroups )
{
	for ( Uint32 i=0; i<m_subGroups.Size(); ++i )
	{
		layerGroups.PushBack( m_subGroups[i] );
	}

	TGroupList :: iterator group;
	for ( group = m_subGroups.Begin(); group != m_subGroups.End(); group++)
	{
		(*group)->GetLayerGroups( layerGroups );
	}
}

Uint32 CLayerGroup::GetLayerGroups( CLayerGroup** layerGroups, Uint32 maxCount )
{
	Uint32 i = 0;
	for ( ; i < m_subGroups.Size() && i < maxCount; ++i )
	{													  
		layerGroups[ i ] = m_subGroups[ i ];
	}

	// recurse into subgroups
	TGroupList::iterator group;
	for ( group = m_subGroups.Begin(); group != m_subGroups.End() && i < maxCount; ++group )
	{
		i += ( *group )->GetLayerGroups( &layerGroups[ i ], maxCount - i );
	}

	return i;
}

void CLayerGroup::ConditionalAttachToWorld()
{
	// Propagate to layers
	for ( Uint32 i=0; i<m_layers.Size(); i++ )
	{
		if ( m_layers[i]->IsLoaded() )
		{
			m_layers[i]->ConditionalAttachToWorld();
		}
	}

	// Check in sub groups
	for ( Uint32 i=0; i<m_subGroups.Size(); i++ )
	{
		m_subGroups[i]->ConditionalAttachToWorld();
	}
}

void CLayerGroup::RemoveShadowsFromGroup()
{
	// Check in sub groups
	for ( Uint32 i=0; i<m_subGroups.Size(); i++ )
	{
		m_subGroups[i]->RemoveShadowsFromGroup();
	}

	// Propagate to layers
	for ( Uint32 i=0; i<m_layers.Size(); i++ )
	{
		if ( m_layers[i]->IsLoaded() )
		{
			m_layers[i]->GetLayer()->RemoveShadowsFromLayer();
		}
	}
}

void CLayerGroup::AddShadowsToGroup()
{
	// Check in sub groups
	for ( Uint32 i=0; i<m_subGroups.Size(); i++ )
	{
		m_subGroups[i]->AddShadowsToGroup();
	}

	// Propagate to layers
	for ( Uint32 i=0; i<m_layers.Size(); i++ )
	{
		if ( m_layers[i]->IsLoaded() )
		{
			m_layers[i]->GetLayer()->AddShadowsToLayer();
		}
	}
}

void CLayerGroup::AddShadowsFromLocalLightsToGroup()
{
	// Check in sub groups
	for ( Uint32 i=0; i<m_subGroups.Size(); i++ )
	{
		m_subGroups[i]->AddShadowsFromLocalLightsToGroup();
	}

	// Propagate to layers
	for ( Uint32 i=0; i<m_layers.Size(); i++ )
	{
		if ( m_layers[i]->IsLoaded() )
		{
			m_layers[i]->GetLayer()->AddShadowsFromLocalLightsToLayer();
		}
	}
}

#ifndef NO_EDITOR
void CLayerGroup::ConvertToStreamed(bool templatesOnly)
{
	// Check in sub groups
	for ( Uint32 i=0; i<m_subGroups.Size(); i++ )
	{
		m_subGroups[i]->ConvertToStreamed(templatesOnly);
	}

	// Propagate to layers
	for ( Uint32 i=0; i<m_layers.Size(); i++ )
	{
		if ( m_layers[i]->IsLoaded() )
		{
			m_layers[i]->GetLayer()->ConvertToStreamed(templatesOnly);
		}
	}
}
#endif

Bool CLayerGroup::CheckShouldSave()
{
	m_shouldSave = ( m_isVisible != m_isVisibleOnStart );

	for ( auto it = m_layers.Begin(); it != m_layers.End(); ++it )
	{
		m_shouldSave |= ( *it )->CheckShouldSave();
	}

	for ( auto it = m_subGroups.Begin(); it != m_subGroups.End(); ++it )
	{
		m_shouldSave |= ( *it )->CheckShouldSave();
	}

// 	if ( m_shouldSave )
// 	{
// 		RED_LOG( Save, TXT("Layer group %s should save"), GetName().AsChar() );
// 	}

	return m_shouldSave;
}


RED_INLINE Bool CLayerGroup::CheckShouldSave_NoRecurse()
{
	m_shouldSave = ( m_isVisible != m_isVisibleOnStart );

	for ( auto it = m_layers.Begin(); it != m_layers.End(); ++it )
	{
		m_shouldSave |= ( *it )->CheckShouldSave();
	}

	return m_shouldSave;
}

Uint32 CLayerGroup::GetLayerGroupsForSave( CLayerGroup** savedLayerGroups, Uint32 maxCount )
{
	Uint32 counter = 0;

	// Check this layer group first
	CheckShouldSave_NoRecurse();

	// iterate over subgroups
	for ( Uint32 i = 0; i < m_subGroups.Size() && counter < maxCount; ++i )
	{
		CLayerGroup* subgroup = m_subGroups[ i ]; 

		// recursive call goes first
		counter += subgroup->GetLayerGroupsForSave( &savedLayerGroups[ counter ], maxCount - counter );
			
		// if a subgroup should save, then this layer group should also save
		m_shouldSave |= subgroup->ShouldSave();

		// add this group to result also
		if ( subgroup->ShouldSave() && counter < maxCount )
		{
			savedLayerGroups[ counter++ ] = subgroup;
		}
	}

	return counter;
}

void CLayerGroup::SaveState( IGameSaver* saver )
{
	#ifndef NO_SAVE_VERBOSITY
		static Uint32 cnt = 0;
		RED_LOG( Save, TXT("Saving layer group %ld: %s%s"), cnt++, GetDepotPath().AsChar(), GetName().AsChar() ); 
	#endif

	CGameSaverBlock block( saver, CNAME( layerGroup ) );

	// Save layers
	const Uint32 numLayers = m_layers.Size();
	Uint32 numLayersToSave = 0;
	for ( auto it = m_layers.Begin(); it != m_layers.End(); ++it )
	{
		if ( ( *it )->ShouldSave() )
		{
			++numLayersToSave;
		}
	}

	saver->WriteValue( CNAME( numLayers ), numLayersToSave );
	for ( auto it = m_layers.Begin(); it != m_layers.End(); ++it )
	{
		if ( false == ( *it )->ShouldSave() )
		{
			continue;
		}

		// Save the name of the layer
		saver->WriteValue< String >( CNAME( Name ), ( *it )->GetShortName() );

		// Save layer info
		( *it )->SaveState( saver );
	}
}

void CLayerGroup::LoadState( IGameLoader* loader )
{
	CGameSaverBlock block( loader, CNAME( layerGroup ) );

	Bool hasVisiblityInfo = false;
	if ( loader->GetSaveVersion() < SAVE_VERSION_LAYERS_VISIBILITY_STORAGE )
	{
		// When layer group has different visibility value than when it started save the info
		loader->ReadValue( CNAME( hasVisiblityInfo ), hasVisiblityInfo );

		// Load visibility info
		if ( hasVisiblityInfo )
		{
			loader->ReadValue( CNAME( isVisible ), m_isVisible );
		}
	}

	// Load layers
	const Uint32 numLayers = loader->ReadValue< Uint32 >( CNAME( numLayers ) );
	if ( loader->GetSaveVersion() < SAVE_VERSION_LAYERGROUPS )
	{
		for ( Uint32 i = 0; i < numLayers; ++i )
		{
			CGameSaverBlock block( loader, CNAME(layer) );

			// Save the name of the layer
			String layerName;
			loader->ReadValue( CNAME(Name), layerName );

			// Find layer and load it
			CLayerInfo* layer = FindLayerCaseless( layerName );
			if ( !layer )
			{
				RED_LOG( Save, TXT("Layer '%ls' not found in '%ls'. Not loading state."), layerName.AsChar(), GetGroupPathName( nullptr ).AsChar() );
			}
			else
			{
				layer->LoadState( loader );
			}
		}
	}
	else
	{
		for ( Uint32 i = 0; i < numLayers; ++i )
		{
			String layerName;
			loader->ReadValue( CNAME( Name ), layerName );

			// Find layer and load it
			CLayerInfo* layer = FindLayerCaseless( layerName );
			if ( !layer )
			{
				RED_LOG( Save, TXT("Layer '%ls' not found in '%ls'. Not loading state."), layerName.AsChar(), GetGroupPathName( nullptr ).AsChar() );
			}
			else
			{
				layer->LoadState( loader );
			}
		}
	}

	if ( loader->GetSaveVersion() < SAVE_VERSION_LAYERGROUPS )
	{
		// Recurse to sub groups
		const Uint32 numSubGroups = loader->ReadValue< Uint32 >( CNAME(numGroups) );
		for ( Uint32 i=0; i<numSubGroups; i++ )
		{
			CGameSaverBlock block( loader, CNAME(subGroup) );

			// Save the name of the sub group
			String groupName;
			loader->ReadValue( CNAME(Name), groupName );

			// Find group and load it
			CLayerGroup* group = FindGroup( groupName );
			if ( !group )
			{
				RED_LOG( Save, TXT("Group '%ls' not found in '%ls'. Not loading state."), groupName.AsChar(), GetGroupPathName( NULL ).AsChar() );
			}
			else
			{
				group->LoadState( loader );
			}
		}
	}

	// Load/unload layers if necessary
	CWorld* activeWorld = GGame->GetActiveWorld();
	CLayerGroup* worldLayers = activeWorld->GetWorldLayers();
	String layerGroupPath = GetGroupPathName( worldLayers );

	if ( hasVisiblityInfo )
	{
		if ( !m_isVisible )
		{
			// unload the child layers
			for ( Uint32 i=0; i<m_layers.Size(); i++ )
			{
				CLayerInfo* info = m_layers[i];
				info->SyncUnload();
				info->UpdateLoadingState();
			}
		}
		else
		{
			for ( Uint32 i=0; i<m_layers.Size(); i++ )
			{
				CLayerInfo* info = m_layers[i];
					
				LayerLoadingContext context;
				info->SyncLoad( context );
				info->UpdateLoadingState();
			}
		}
	}
}

CSystemLayerGroup::CSystemLayerGroup()
	: CLayerGroup()
{
}

CSystemLayerGroup::CSystemLayerGroup( CWorld* world, CLayerGroup* parentGroup, const String& name, const String& depotFilePath, const String& absoluteFilePath )
	: CLayerGroup( world, parentGroup, name, depotFilePath, absoluteFilePath )
{
}
