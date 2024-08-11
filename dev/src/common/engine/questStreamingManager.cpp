#include "build.h"

#include "questStreamingManager.h"

#ifdef USE_RED_RESOURCEMANAGER
#include "entityManager.h"
#include "../core/resourcemanager.h"
#include "../core/fileloadedcallback.h"

#define ANGLE_INC 20
#define GRIDLINERESERVE ( 1024 * 48 )

using namespace Red::Core::ResourceManagement;

//////////////////////////////////////////////////////////////////////////
// File Loaded Callback
//////////////////////////////////////////////////////////////////////////
void CManagedLayer::Callback( CResourceHandleCollection& handleCollection )
{
	// After the bundle is loaded, we need to actually load the layer as we don't stream the layers yet.
	// This is in fact a slightly hacky solution.
	if( !m_layerInfo.IsLoaded() )
	{
		ReadyForLayer();
	}
}
//////////////////////////////////////////////////////////////////////////
void CManagedLayer::LoadLayer()
{
	if( m_needsLayerLoading.GetValue() == true && !m_layerInfo.IsLoaded() )
	{
		LayerLoadingContext loadingContext;
		m_layerInfo.SyncLoad( loadingContext );
		m_needsLayerLoading.SetValue( false );
	}
}

//////////////////////////////////////////////////////////////////////////
void CManagedLayer::UnloadLayer()
{
	m_layerInfo.SyncUnload();
}

//////////////////////////////////////////////////////////////////////////
CQuestStreamingManager::CQuestStreamingManager( Red::Core::ResourceManagement::CResourceManager& resourceManager, const CDiskFile& worldFile )
	: m_resourceManager( resourceManager )
	, m_metadataResourceHandle( Red::Core::ResourceManagement::CResourceHandle::INVALIDHANDLE )
	, m_worldFilename( worldFile.GetFileName() )
	, m_directory( worldFile.GetDirectory() )
{
}

//////////////////////////////////////////////////////////////////////////
CQuestStreamingManager::~CQuestStreamingManager()
{
}

//////////////////////////////////////////////////////////////////////////
Bool CQuestStreamingManager::Initialize()
{
	Bool wasSuccessful = false;

	const Red::Core::ResourceManagement::CResourcePaths& pathManager = GEngine->GetPathManager();
	const String& questMetadataFilename = pathManager.GetPath( Red::Core::ResourceManagement::CResourcePaths::Path_QuestMetadataDepot );

	// Load the metadata file - via an immediate load. This loads the file SYNCHRONOUSLY!
	CResourceLoadRequest metadataLoadRequest( questMetadataFilename, GET_FOURCC( CQuestStreamingMetadata) );
	m_metadataResourceHandle = m_resourceManager.ImmediateLoadResource( metadataLoadRequest );

	// Either the resource handle couldn't be made for some reason - or it isn't valid i.e. No data.
	if( m_metadataResourceHandle.IsValid() )
	{
		// Grab the resource back as the type we want it as.
		CQuestStreamingMetadata* worldStreamingMetadata = m_metadataResourceHandle.Get< CQuestStreamingMetadata >();
		RED_ASSERT( worldStreamingMetadata != nullptr, TXT("Was unable to load the quest streaming metadata") );
		wasSuccessful = true;
	}

	return wasSuccessful;
}


#ifndef NO_EDITOR
//////////////////////////////////////////////////////////////////////////
// EDITOR ONLY FUNCTIONALITY
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
void CQuestStreamingManager::Generate( const CWorld* world )
{
	CQuestStreamingMetadataCreator streamingMetadataCreator( m_resourceManager );
	streamingMetadataCreator.Initialize( m_worldFilename, *m_directory );
	m_metadataResourceHandle = streamingMetadataCreator.Generate( world, GIsCooker );
}

#endif // !NO_EDITOR
//////////////////////////////////////////////////////////////////////////
void CQuestStreamingManager::Update( const Vector& position, const TDynArray< Float >& streamingDistances )
{
	// Return if we don't have any metadata.
	if( !m_metadataResourceHandle.IsValid() )
	{
		return;
	}
	
	FlushLayers();

	//%%% testing loading bundles, just load anything that's not yet loaded
	CQuestStreamingMetadata* qsm = m_metadataResourceHandle.Get< CQuestStreamingMetadata >();
	const Uint32 managedLayersSize = m_managedLayers.Size();
	for ( Uint32 i = 0; i < managedLayersSize; i++ )
	{
		CManagedLayer& layer = (*m_managedLayers[i]);
		if ( !layer.BundleLoadRequested() )
		{

			CQuestMetadataNodeInfo* nodeInfo = qsm->GetNodeInfo( layer.GetLayerGUID() );
			m_bundlesToLoad.PushBack( nodeInfo );
			layer.SetBundleLoadRequested( true );
		}
	}

	LoadBundle();
	UnloadBundle();

	m_bundlesToLoad.ClearFast();
	m_bundlesToUnload.ClearFast();
}

//////////////////////////////////////////////////////////////////////////
void CQuestStreamingManager::LoadBundle()
{
	const Uint32 bundlesCount = m_bundlesToLoad.Size();
	for( Uint32 i = 0; i < bundlesCount; ++i )
	{
		Red::System::GUID bundleGuid = m_bundlesToLoad[i]->m_bundleGUID;

		if( !m_activeResourceHandles.KeyExist( bundleGuid ) )
		{
			const Red::Core::ResourceManagement::CResourcePaths& pathManager = GEngine->GetPathManager();

			String bundleName	=  ANSI_TO_UNICODE( m_bundlesToLoad[i]->m_depotFile.AsChar() );
			String depotPath	= pathManager.GetPath( Red::Core::ResourceManagement::CResourcePaths::Path_BundlesDirectoryAbsolute ) + bundleName;

			void* addr = RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_ResourceBuffer, sizeof( CResourceHandleCollection ) );
			CResourceHandleCollection* resourceHandleCollection = new (addr)CResourceHandleCollection();
			Uint32 managedLayerIndex = Uint32( -1 );
			const Uint32 managedLayerSize = m_managedLayers.Size();
			for( Uint32 p = 0; p < managedLayerSize; ++p )
			{
				if( m_managedLayers[p]->GetLayerGUID() == m_bundlesToLoad[i]->m_layerGUID )
				{
					managedLayerIndex = p;
					break;
				}
			}

			// Setup Bundle Load Request.
			CBundleLoadRequest loadRequest( depotPath, m_bundlesToLoad[i]->m_itemCount, *resourceHandleCollection, m_managedLayers[managedLayerIndex] );
			if( m_resourceManager.LoadBundle( loadRequest ) )
			{
				m_activeResourceHandles.Insert( bundleGuid, resourceHandleCollection );
			}
			else
			{
				resourceHandleCollection->~CResourceHandleCollection();
				RED_MEMORY_FREE( MemoryPool_Default, MC_ResourceBuffer, addr );
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CQuestStreamingManager::UnloadBundle()
{
	const Uint32 bundleCount = m_bundlesToUnload.Size();
	for( Uint32 i = 0; i < bundleCount; ++i )
	{
		if( !m_activeResourceHandles.KeyExist( m_bundlesToUnload[i]->m_bundleGUID ) )
		{
			continue;
		}
		
		CResourceHandleCollection* resourceHandleCollection = m_activeResourceHandles[ m_bundlesToUnload[i]->m_bundleGUID ];
		CResourceHandleCollection::THandleMap& handleMap = resourceHandleCollection->GetHandles();
		CResourceHandleCollection::THandleMap::iterator it = handleMap.Begin();
		CResourceHandleCollection::THandleMap::iterator end = handleMap.End();
		TDynArray< CResourceId, MC_ResourceBuffer > m_resourceIDsToKill( handleMap.Size() );
		Uint32 idIndex = 0;
		
		while( it != end )
		{
			const CResourceId& resourceID = it->m_second.GetResourceID();
			m_resourceIDsToKill[idIndex++] = resourceID;
			
			++it;
		}

		for( Uint32 p = 0; p < m_resourceIDsToKill.Size(); ++p )
		{
			const CResourceId& resourceID = m_resourceIDsToKill[ p ];
			resourceHandleCollection->RemoveHandle( resourceID );

			CResourceUnloadRequest unloadRequest( resourceID );
			m_resourceManager.RequestUnload( unloadRequest );
		}

		RED_MEMORY_FREE( MemoryPool_Default, MC_ResourceBuffer, m_activeResourceHandles[ m_bundlesToUnload[ i ]->m_bundleGUID ] );
		m_activeResourceHandles.Erase( m_bundlesToUnload[i]->m_bundleGUID );
	}
}

void CQuestStreamingManager::FlushLayers()
{
	const Uint32 managedLayerSize = m_managedLayers.Size();
	for( Uint32 i = 0; i < managedLayerSize; ++i )
	{
		m_managedLayers[i]->LoadLayer();
	}
}

//////////////////////////////////////////////////////////////////////////
void CQuestStreamingManager::AddLayers( TDynArray< CLayerInfo* >& layerInfos )
{
	const Uint32 layerInfosCount = layerInfos.Size();
	Uint32 cycleCount = 0;
	Uint32 layerIndex = 0;
	while( cycleCount < layerInfosCount )
	{
		++cycleCount;
		CLayerInfo* currentLayerInfo = layerInfos[ layerIndex++ ];
		if( currentLayerInfo->GetLayerBuildTag() == LBT_Quest )
		{
			CManagedLayer* managedLayer = new CManagedLayer( *currentLayerInfo );
			managedLayer->SetBundleLoadRequested( false );
			m_managedLayers.PushBack( managedLayer );
			layerInfos.RemoveAt(--layerIndex);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CQuestStreamingManager::RemoveLayers( TDynArray< CLayerInfo* >& layerInfos )
{
	const Uint32 layerInfosCount = layerInfos.Size();
	const Uint32 managedLayerCount =  m_managedLayers.Size();
	for ( Uint32 layerIdx = 0; layerIdx < layerInfosCount; layerIdx++ )
	{
		for ( Uint32 managedIdx = 0; managedIdx < managedLayerCount; managedIdx++ )
		{
			if ( ( *m_managedLayers[managedIdx] )== layerInfos[layerIdx] )
			{
				CManagedLayer& layer = (*m_managedLayers[managedIdx]);
				if ( layer.BundleLoadRequested() && layer.HasLayer() )
				{
					CQuestStreamingMetadata* qsm = m_metadataResourceHandle.Get< CQuestStreamingMetadata >();
					CQuestMetadataNodeInfo* nodeInfo = qsm->GetNodeInfo( layer.GetLayerGUID() );
					m_bundlesToUnload.PushBack( nodeInfo );

					layer.SetBundleLoadRequested( false );
					layer.UnloadLayer();
				}
				delete m_managedLayers[managedIdx];
				m_managedLayers.RemoveAt( managedIdx );
				break;
			}
		}
	}
}

#endif // USE_RED_RESOURCEMANAGER