#include "build.h"
#include "../../common/core/bundledefinition.h"

#ifdef USE_RED_RESOURCEMANAGER
#include "../core/bundledefinition.h"
#include "../core/resourcedatabasegraphgeneration.h"
#include "worldstreaminggraphgenerator.h"

//////////////////////////////////////////////////////////////////////////
// CQuestMetadataNodeInfo
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
CQuestMetadataNodeInfo::CQuestMetadataNodeInfo()
	: m_depotFile( StringAnsi::EMPTY )
	, m_itemCount( 0 )
	, m_headerSize( 0 )
	, m_layerGUID( CGUID::ZERO )
	, m_bundleGUID( CGUID::ZERO )
{
}

//////////////////////////////////////////////////////////////////////////
CQuestMetadataNodeInfo::CQuestMetadataNodeInfo( const StringAnsi& depotFile, const Uint32 itemCount, const CGUID& layerGuid, const CGUID& bundleGuid )
	: m_depotFile( depotFile )
	, m_itemCount( itemCount )
	, m_headerSize( itemCount * sizeof( Red::Core::ResourceManagement::SBundleHeaderItem ) )
	, m_layerGUID( layerGuid )
	, m_bundleGUID( bundleGuid )
{

}

//////////////////////////////////////////////////////////////////////////
CQuestMetadataNodeInfo::CQuestMetadataNodeInfo( const CQuestMetadataNodeInfo& other )
	: m_depotFile( other.m_depotFile )
	, m_itemCount( other.m_itemCount )
	, m_headerSize( other.m_headerSize )
	, m_layerGUID( other.m_layerGUID)
	, m_bundleGUID( other.m_bundleGUID )
{

}

//////////////////////////////////////////////////////////////////////////
CQuestMetadataNodeInfo::~CQuestMetadataNodeInfo()
{

}

//////////////////////////////////////////////////////////////////////////
// CQuestStreamingMetadata
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
CQuestStreamingMetadata::CQuestStreamingMetadata()
{
}

//////////////////////////////////////////////////////////////////////////
CQuestStreamingMetadata::~CQuestStreamingMetadata()
{

}

//////////////////////////////////////////////////////////////////////////
void CQuestStreamingMetadata::Serialize( IFile& file )
{
	// TODO:
	// Really quick hacked serialize.
	// THIS NEEDS TO BE CLEANED UP LATER! AND DONE PROPERLY!
	Uint32 metdataNodesSize = m_metadataNodes.Size();
	file << metdataNodesSize;
	m_metadataNodes.Resize( metdataNodesSize );
	for( Uint32 i = 0; i < metdataNodesSize; ++i )
	{
		if( file.IsWriter() )
		{
			file << m_metadataNodes[i].m_depotFile;
			file << m_metadataNodes[i].m_itemCount;
			file << m_metadataNodes[i].m_headerSize;
			file << m_metadataNodes[i].m_layerGUID;
			file << m_metadataNodes[i].m_bundleGUID;
		}
		else if ( file.IsReader() )
		{
			CQuestMetadataNodeInfo nodeInfo;
			file << nodeInfo.m_depotFile;
			file << nodeInfo.m_itemCount;
			file << nodeInfo.m_headerSize;
			file << nodeInfo.m_layerGUID;
			file << nodeInfo.m_bundleGUID;
			m_metadataNodes.PushBack( nodeInfo );
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CQuestStreamingMetadata::AddNodeInfo( CQuestMetadataNodeInfo& nodeInfo )
{
	m_metadataNodes.PushBack( nodeInfo );
}

//////////////////////////////////////////////////////////////////////////
CQuestMetadataNodeInfo* CQuestStreamingMetadata::GetNodeInfo( const CGUID& guid )
{
	for ( Uint32 i = 0; i < m_metadataNodes.Size(); i++ )
	{
		if ( m_metadataNodes[i].m_layerGUID == guid )
		{
			return &(m_metadataNodes[i]);
		}
	}
	return NULL;
}

//////////////////////////////////////////////////////////////////////////
// CQuestStreamingMetadataCreator
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
CQuestStreamingMetadataCreator::CQuestStreamingMetadataCreator( Red::Core::ResourceManagement::CResourceManager& resourceManager ) 
	: m_resourceManager( resourceManager )
	, m_buildDb( resourceManager.GetBuildDatabase() )
{
}

//////////////////////////////////////////////////////////////////////////
CQuestStreamingMetadataCreator::~CQuestStreamingMetadataCreator()
{
	delete m_bundleDefinitionWriter;
}

#ifndef NO_EDITOR
//////////////////////////////////////////////////////////////////////////
void CQuestStreamingMetadataCreator::Initialize( const String& worldFilename, CDirectory& directory )
{
	// Create the working directory to store temporary data
	Red::Core::ResourceManagement::CResourcePaths& pathManager = GEngine->GetPathManager();
	m_workingDataDirectory = pathManager.GetDirectory( Red::Core::ResourceManagement::CResourcePaths::Directory_WorkingQuest );

	// Create the file names.
	m_metadataFilename = pathManager.GetPath( Red::Core::ResourceManagement::CResourcePaths::Path_QuestMetadataDepot );
	m_bundleDefinitionFilename = UNICODE_TO_ANSI( pathManager.GetPath( Red::Core::ResourceManagement::CResourcePaths::Path_BundleDefinitionFilename ).AsChar() );

	Red::Core::ResourceManagement::CResourceId resourceId( m_metadataFilename );
	Red::Core::ResourceManagement::CResourceBuildDatabase::ResourceIndex index = m_buildDb->AddNewResource( "CQuestStreamingMetadata", resourceId );

	m_buildDb->SetResourceName( index, UNICODE_TO_ANSI( worldFilename.AsChar() ) );
	m_buildDb->SetResourcePath( index, UNICODE_TO_ANSI( m_metadataFilename.AsChar() ) );

	// Create the bundle definition writer
	StringAnsi filePath = UNICODE_TO_ANSI( m_workingDataDirectory->GetAbsolutePath().AsChar() );
	filePath += m_bundleDefinitionFilename;
	m_bundleDefinitionWriter = new Red::Core::BundleDefinition::CBundleDefinitionWriter( filePath.AsChar() );
}

//////////////////////////////////////////////////////////////////////////
Red::Core::ResourceManagement::CResourceHandle& CQuestStreamingMetadataCreator::Generate( const CWorld* world, Bool loadGlobalLayers )
{
	SRedMemory::GetInstance().GetMetricsCollector().AddTrackingTag("Start_GenerateQuestMetadata");
	// Disable mesh component scene attachment as we don't want or need anything to be registered with the renderer in here!
	CMeshComponent::DisableSceneAttachment();
	GFeedback->UpdateTaskProgress( 0, 100 );
	GFeedback->UpdateTaskInfo( TXT( "Generating Quest Streaming Metadata" ) );

	// If we've already created the resource, destroy it - maybe we're regenerating the data.
	if( m_questStreamingMetadataResourceHandle.IsValid() )
	{
		Red::Core::ResourceManagement::CResourceUnloadRequest unloadRequest( m_questStreamingMetadataResourceHandle.GetResourceID() );
		m_resourceManager.ImmediateDestroyResource( unloadRequest );
	}

	// Construct a CQuestStreamingMetadata object.
	Red::Core::ResourceManagement::CResourceCreationRequest metadataCreationRequest( m_metadataFilename, GET_FOURCC( CQuestStreamingMetadata ) );
	
	m_questStreamingMetadataResourceHandle = m_resourceManager.ImmediateConstructResource( metadataCreationRequest );
	ASSERT( m_questStreamingMetadataResourceHandle != Red::Core::ResourceManagement::CResourceHandle::INVALIDHANDLE, TXT("Quest streaming metadata couldn't be created -- generation will fail") );

	GFeedback->UpdateTaskInfo( TXT( "Collecting Layers" ) );
	GFeedback->UpdateTaskProgress( 1, 100 );

	// Grab the world layer info's from the world.
	TDynArray< CLayerInfo* > worldLayerInfos;
	CollectWorldLayerInfos( world, worldLayerInfos );

	GFeedback->UpdateTaskInfo( TXT( "Filtering Layers" ) );
	GFeedback->UpdateTaskProgress( 2, 100 );

	// Filter the world layer info's for the world streaming metadata types
	TDynArray< CLayerInfo* > filteredLayerInfos;
	FilterLayers( worldLayerInfos, filteredLayerInfos, LBT_Quest );
	RED_ASSERT( filteredLayerInfos.Size() > 0, TXT("Quest streaming generation cannot proceed no layers are marked with the layer build tag LBT_QUEST") );
	
	if( filteredLayerInfos.Size() == 0 )
	{
		return m_questStreamingMetadataResourceHandle;
	}

	GFeedback->UpdateTaskInfo( TXT( "Adding layers to database" ) );
	GFeedback->UpdateTaskProgress( 3, 100 );

	GFeedback->UpdateTaskInfo( TXT( "Processing Layers" ) );
	GFeedback->UpdateTaskProgress( 10, 100 );

	// For each layer we're going to gather the entities and make a bundle that can be loaded by the manager later
	const Uint32 filterLayerCount = filteredLayerInfos.Size();
	// Reserve the same number of SLayerNode objects as we have layers to process.
	m_layerNodes.Resize( filterLayerCount );
	for ( Uint32 i = 0; i < filterLayerCount; i++ )
	{
		ProcessLayer( filteredLayerInfos[i], i );
	}
	
	GFeedback->UpdateTaskInfo( TXT( "Saving components to disk" ) );
	GFeedback->UpdateTaskProgress( 70, 100 );

	SaveNodeFiles();

	GFeedback->UpdateTaskInfo( TXT( "Gathering bundle descriptions" ) );
	GFeedback->UpdateTaskProgress( 80, 100 );
	
	// Add the bundle file descriptions to the bundle definition writer, and add nodes to the Quest streaming metadata.
	const Uint32 layerNodeSize = m_layerNodes.Size();
	for ( Uint32 layerIdx = 0; layerIdx < layerNodeSize; layerIdx++ )
	{
		SLayerNode& layerNode = m_layerNodes[layerIdx];
		// Gather up the SBundleFileInfo objects and add them to bundle definition.
		StringAnsi bundleName = StringAnsi::Printf( "QuestStreaming_%s.bundle", UNICODE_TO_ANSI( layerNode.m_LayerShortName.AsChar() ) );
		m_bundleDefinitionWriter->AddBundle( bundleName );

		const Uint32 bundleDscSize = layerNode.m_bundleDescriptions.Size();
		for ( Uint32 bundleDscIdx = 0; bundleDscIdx < bundleDscSize; bundleDscIdx ++ )
		{
			m_bundleDefinitionWriter->AddBundleFileDesc( bundleName, (*layerNode.m_bundleDescriptions[bundleDscIdx]) );
		}

		// Create the metadata node information description.
		CQuestMetadataNodeInfo metadataNode( bundleName, bundleDscSize, layerNode.m_layerGuid, Red::System::GUID::Create() );
		CQuestStreamingMetadata* qsm = m_questStreamingMetadataResourceHandle.Get< CQuestStreamingMetadata >();
		qsm->AddNodeInfo( metadataNode );
	}

	// Write out the bundle definition JSON file.
	m_bundleDefinitionWriter->Write();

	GFeedback->UpdateTaskInfo( TXT( "Creating quest metadata" ) );
	GFeedback->UpdateTaskProgress( 85, 100 );

	// Fill out the world streaming metadata object.
	CreateMetadata();

	// Re-enable scene attachment of mesh components
	CMeshComponent::EnableSceneAttachment();
	
	SRedMemory::GetInstance().GetMetricsCollector().AddTrackingTag("End_GenerateQuestMetadata");

	return m_questStreamingMetadataResourceHandle;
}

//////////////////////////////////////////////////////////////////////////
void CQuestStreamingMetadataCreator::CreateMetadata()
{
	if( m_questStreamingMetadataResourceHandle.IsValid() )
	{
		CQuestStreamingMetadata* worldStreamingMetadata = m_questStreamingMetadataResourceHandle.Get< CQuestStreamingMetadata >();
		CreateMetadataFile();
	}
}

//////////////////////////////////////////////////////////////////////////
Bool CQuestStreamingMetadataCreator::CreateMetadataFile()
{
	IFile* fileWriter = GFileManager->CreateFileWriter( m_metadataFilename );
	if( fileWriter )
	{
		CQuestStreamingMetadata* qsm = m_questStreamingMetadataResourceHandle.Get< CQuestStreamingMetadata >();
		qsm->Serialize( *fileWriter );
		delete fileWriter;
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////
void CQuestStreamingMetadataCreator::CollectWorldLayerInfos( const CWorld* world, TDynArray< CLayerInfo* >& worldLayerInfos )
{
	// Get the world layer group.
	CLayerGroup* worldLayerGroup = world->GetWorldLayers();
	if( worldLayerGroup == nullptr )
	{
		RED_ASSERT(  worldLayerGroup != nullptr, TXT("World layer group appears to be invalid. Check the level.") );
		return;
	}

	Uint32 layerCount = CountLayers( worldLayerGroup, false );
	// Reserve the memory at the correct size.
	worldLayerInfos.Reserve( layerCount );
	// Gather up all the non-system layers.
	// Pull out any layers on the root world layer group level.
	const TDynArray< CLayerInfo* >& rootWorldLayers = worldLayerGroup->GetLayers();
	const Uint32 rootWorldLayersSize = rootWorldLayers.Size();
	for( Uint32 i = 0; i < rootWorldLayersSize; ++i )
	{
		worldLayerInfos.PushBack( rootWorldLayers[i] );
	}

	// Grab only the global layers - We're no longer interested in the streaming layers.
	TDynArray< CLayerGroup* > globalLayerGroups;
	worldLayerGroup->GetGlobalLayerGroups( globalLayerGroups );
	for( Uint32 i = 0; i < globalLayerGroups.Size(); ++i )
	{
		globalLayerGroups[i]->GetLayers( worldLayerInfos, false, false, true );
	}
}

//////////////////////////////////////////////////////////////////////////
Uint32 CQuestStreamingMetadataCreator::CountLayers( const CLayerGroup* layerGroup, Bool countSystemGroupLayer ) const
{
	// Counts all the available layers
	Uint32 layerCount = 0;
	if( layerGroup->IsSystemGroup() == countSystemGroupLayer )
	{	
		layerCount += layerGroup->GetLayers().Size();
	}

	const TDynArray<CLayerGroup*>& subGroups = layerGroup->GetSubGroups();
	const Uint32 subGroupsCount = subGroups.Size();
	for( Uint32 i = 0; i < subGroupsCount; ++i )
	{
		layerCount += CountLayers( subGroups[i], countSystemGroupLayer );
	}
	
	return layerCount;
}

//////////////////////////////////////////////////////////////////////////
void CQuestStreamingMetadataCreator::FilterLayers( TDynArray< CLayerInfo* >& srcLayers, TDynArray< CLayerInfo* >& dstLayers, ELayerBuildTag filterTag )
{
	// Store only the layers with the correct streaming data tag.
	Uint32 layerCount = srcLayers.Size();
	Uint32 layerIndex = 0;
	while( layerCount-- > 0 )
	{
		if( srcLayers[layerIndex]->GetLayerBuildTag() != filterTag )
		{
			++layerIndex;
		}
		else
		{
			RED_LOG(RED_LOG_CHANNEL(WorldStreaming), TXT( "Adding Layer: %s as it matches the layerInfoTag{%d}" ), srcLayers[layerIndex]->GetDepotPath().AsChar(), static_cast<Uint32>( filterTag ) );
			// We don't want duplicate layers.
			dstLayers.PushBackUnique( srcLayers[layerIndex] );
			srcLayers.Remove( srcLayers[layerIndex] );
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CQuestStreamingMetadataCreator::GatherEntities( CLayerInfo* srcLayer, TDynArray< CEntity* >& entities )
{
	// Cycle the layers, and load them if they aren't, pulling out the entities on the layer.
	Bool done = false;
	while( !done )
	{
		if( srcLayer->IsLoaded() )
		{
			if( !srcLayer->GetLayer()->OwnsEntities() )
			{
				srcLayer->GetLayer()->GetEntities( entities );
			}
			done = true;
		}
		else if( !srcLayer->IsLoading() )
		{
			LayerLoadingContext layerLoadingContext;
			layerLoadingContext.m_loadHidden = true;
			srcLayer->SyncLoad( layerLoadingContext );
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CQuestStreamingMetadataCreator::InsertComponent( CComponent* component, SLayerNode& layerNode, const Uint32 entityIdx )
{
	// Create a database entry for the component. NOTE:( This is not a file ).
	SResourceDatabaseItemCreationData componentResourceDatabaseItem;
	componentResourceDatabaseItem.m_typeName = component->GetClass()->GetName().AsString();
	componentResourceDatabaseItem.m_resourceName = String( component->GetFriendlyName() );
	componentResourceDatabaseItem.m_sourcePath = component->GetLayer()->GetRawEntityGroupResourceName() + String( TXT( " -> " ) )  + component->GetEntity()->GetName() + String( TXT( " -> " ) ) + component->GetName();
	componentResourceDatabaseItem.m_resourceIDName = componentResourceDatabaseItem.m_sourcePath;
	componentResourceDatabaseItem.m_guid = component->GetGUID();
	CStreamingResourceHelper::AddNewResourceToDatabase( *m_buildDb, componentResourceDatabaseItem );

	// Cast the component to being a mesh component.
	CMeshComponent* meshComponent = static_cast< CMeshComponent* >( component );
	
	// Process the mesh component, this results in the CMesh and Mesh Chunk being altered and re-saved to the working directory.
	Red::Core::ResourceManagement::CResourceBuildDatabase::ResourceIndex meshResourceIndex = CStreamingResourceHelper::ProcessMeshComponent( meshComponent, *m_buildDb, *m_workingDataDirectory, layerNode.m_bundleDescriptions );
	
	// It could be that there is no mesh component resource that we're actually interested 
	if( meshResourceIndex == Red::Core::ResourceManagement::CResourceBuildDatabase::c_InvalidIndex )
	{
		return;
	}

	CComponentResourceCollection* componentResourceCollection = nullptr;
	for( Uint32 i = 0; i < layerNode.m_componentCollections.Size(); ++i )
	{
		if( layerNode.m_componentCollections[i]->GetOwner() == component->GetEntity()->GetGUID() )
		{
			componentResourceCollection = layerNode.m_componentCollections[i];
			break;
		}	
	}

	if( componentResourceCollection == nullptr )
	{
		// Create a resource collection object - we'll use this to store the components for this CEntity
		componentResourceCollection = new CComponentResourceCollection( component->GetEntity()->GetGUID() );
		componentResourceCollection->Initialize();
		layerNode.m_componentCollections.PushBack( componentResourceCollection );
	}

	
	String componentResourceCollectionName = layerNode.m_nodeName + String( TXT( ".wpn" ) );
	String componentResourceCollectionIdName = m_workingDataDirectory->GetDepotPath() + componentResourceCollectionName;
	Red::Core::ResourceManagement::CResourceId componentResourceCollectionId( componentResourceCollectionIdName );

	Red::Core::ResourceManagement::CResourceBuildDatabase::ResourceIndex componentResourceCollectionIndex = m_buildDb->FindResourceById( componentResourceCollectionId );
	if( componentResourceCollectionIndex == Red::Core::ResourceManagement::CResourceBuildDatabase::c_InvalidIndex )
	{
		// Create a component resource collection database entry item. (This will be a file on disk).
		SResourceDatabaseItemCreationData componentResourceCollectionDatabaseItem;
		componentResourceCollectionDatabaseItem.m_typeName = String( TXT( "CComponentResourceCollection" ) );
		componentResourceCollectionDatabaseItem.m_resourceName = componentResourceCollectionName;
		componentResourceCollectionDatabaseItem.m_sourcePath = component->GetLayer()->GetRawEntityGroupResourceName() + String( TXT( " -> " ) )  + component->GetEntity()->GetName() + String( TXT( " -> " ) ) + component->GetName();
		componentResourceCollectionDatabaseItem.m_resourceIDName = componentResourceCollectionIdName;
		componentResourceCollectionDatabaseItem.m_guid = Red::System::GUID::Create();
		componentResourceCollectionIndex = CStreamingResourceHelper::AddNewResourceToDatabase( *m_buildDb, componentResourceCollectionDatabaseItem );
	}
	m_buildDb->AddDependency( componentResourceCollectionIndex, meshResourceIndex );
	
	// Store the component ready for writing to the bundle file
	CStreamingResourceHelper::SaveComponentToCollection( meshComponent, componentResourceCollection );

}

//////////////////////////////////////////////////////////////////////////
void CQuestStreamingMetadataCreator::InsertEntity( CEntity* entity, SLayerNode& layerNode, const Uint32 entityidx )
{
	// Create a database item for the entity. NOTE:( This is not a file ).
	SResourceDatabaseItemCreationData entityResourceDatabaseItem;
	entityResourceDatabaseItem.m_typeName = String( TXT("CEntity") );
	entityResourceDatabaseItem.m_sourcePath = entity->GetLayer()->GetRawEntityGroupResourceName() + String( TXT( " - " ) ) + entity->GetName();
	entityResourceDatabaseItem.m_resourceName = entity->GetFriendlyName();
	entityResourceDatabaseItem.m_resourceIDName = entityResourceDatabaseItem.m_sourcePath;
	entityResourceDatabaseItem.m_guid = entity->GetGUID();
	CStreamingResourceHelper::AddNewResourceToDatabase( *m_buildDb, entityResourceDatabaseItem );

	// Grab the entities components, and cycle through them.
	const TDynArray<CComponent*>& components = entity->GetComponents();

	auto it = components.Begin();
	while( it != components.End() )
	{
		CComponent* currentComponent = (*it++);

		// NOTE: This is temp until all component types can be streamed! We're only working with MeshComponents for now,
		// so we ignore the others.
		if( !currentComponent || !currentComponent->IsA<CMeshComponent>() )
		{
			RED_LOG( QuestStreaming, TXT("Component: {%s} of entity {%s} is not a mesh component."), currentComponent->GetName().AsChar(), entity->GetName().AsChar() );
			continue;
		}

		InsertComponent( currentComponent, layerNode, entityidx );
	}

	CStreamingResourceHelper::PrepareEntityForResaved( entity );
}

//////////////////////////////////////////////////////////////////////////
void CQuestStreamingMetadataCreator::SaveNodeFiles()
{
	const Uint32 layerNodesCount = m_layerNodes.Size();
	for( Uint32 layerIdx = 0; layerIdx < layerNodesCount; ++layerIdx )
	{
		SLayerNode& layerNode = m_layerNodes[layerIdx];
		String nodeName = layerNode.m_nodeName;

		String filename = nodeName + String( TXT(".wpn") );
		CDiskFile diskFile( m_workingDataDirectory, filename );
		IFile* writer = diskFile.CreateWriter( m_workingDataDirectory->GetAbsolutePath() + filename );
		RED_ASSERT( writer != nullptr, TXT( "Unable to create file writer to write file: %s to disk" ), m_workingDataDirectory->GetDepotPath() + filename );
		if( writer == nullptr )
		{
			continue;
		}
		// Serialize out the component resource collections to the file.
		Uint32 componentResourceCollectionSize = layerNode.m_componentCollections.Size();
		(*writer) << componentResourceCollectionSize;
		for( Uint32 componentResourceCollectionIdx = 0; componentResourceCollectionIdx < componentResourceCollectionSize; ++componentResourceCollectionIdx )
		{
			layerNode.m_componentCollections[componentResourceCollectionIdx]->Serialize( *writer );
		}

		// Delete the writer after writing.
		delete writer;
		Red::Core::ResourceManagement::CResourceId resourceId( diskFile.GetDepotPath() );
		// Grab the resource from the database via it's hashed ID.
		Red::Core::ResourceManagement::CResourceBuildDatabase::ResourceIndex resourceNodeIndex = m_buildDb->FindResourceById( resourceId );
		if( resourceNodeIndex != Red::Core::ResourceManagement::CResourceBuildDatabase::c_InvalidIndex )
		{
			// We need to grab the node back from the resource database for creating the bundle file description.
			CStreamingResourceHelper::CreateBundleFileDescription( &diskFile, GET_FOURCC( CComponentResourceCollection ), Red::Core::ResourceManagement::CT_Default, resourceNodeIndex, *m_buildDb, layerNode.m_bundleDescriptions );
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CQuestStreamingMetadataCreator::ProcessLayer( CLayerInfo* layerInfo, Uint32 layerIdx )
{
	// Gather up all the entities for this layer.
	TDynArray< CEntity* > worldEntities;
	GatherEntities( layerInfo, worldEntities );
	if( layerInfo->GetLayer()->OwnsEntities() )
	{
		return;
	}
	m_layerNodes[layerIdx].m_LayerShortName = layerInfo->GetShortName();
	m_layerNodes[layerIdx].m_layerGuid		= layerInfo->GetGUID();

	// the node name for this component
	m_layerNodes[layerIdx].m_nodeName = String::Printf( TXT( "QuestNode_%s" ), m_layerNodes[layerIdx].m_LayerShortName.AsChar() );

	const Uint32 worldEntitiesSize = worldEntities.Size();
	m_layerNodes[layerIdx].m_componentCollections.Reserve( worldEntitiesSize );

	// Create a database item for the initial RAW entity group resource. (File on disk)
	SResourceDatabaseItemCreationData entityResourceGroupDatabaseItem;
	entityResourceGroupDatabaseItem.m_typeName = String( TXT("CEntityGroupResource") );
	entityResourceGroupDatabaseItem.m_sourcePath = layerInfo->GetLayer()->GetRawEntityGroupResourceName();
	entityResourceGroupDatabaseItem.m_resourceName = layerInfo->GetLayer()->GetDepotPath() + String( TXT( ".ents" ) );
	entityResourceGroupDatabaseItem.m_resourceIDName = entityResourceGroupDatabaseItem.m_sourcePath;
	entityResourceGroupDatabaseItem.m_guid = Red::System::GUID::Create();
	Red::Core::ResourceManagement::CResourceBuildDatabase::ResourceIndex entityResourceGroupIndex = CStreamingResourceHelper::AddNewResourceToDatabase( *m_buildDb, entityResourceGroupDatabaseItem );
	
	// Process all the entities in the layer.
	for ( Uint32 i = 0; i < worldEntitiesSize; i++ )
	{
		InsertEntity( worldEntities[i], m_layerNodes[layerIdx], i );
	}

	Red::Core::ResourceManagement::CResourceBuildDatabase::ResourceIndex intermediateEntityGroupResourceIndex = CStreamingResourceHelper::ResaveEntityGroupResources( layerInfo, *m_buildDb, entityResourceGroupIndex, *m_workingDataDirectory, m_layerNodes[layerIdx].m_bundleDescriptions );

	String componentResourceCollectionName = m_layerNodes[layerIdx].m_nodeName + String( TXT( ".wpn" ) );
	String componentResourceCollectionIdName = m_workingDataDirectory->GetDepotPath() + componentResourceCollectionName;
	Red::Core::ResourceManagement::CResourceId componentResourceCollectionId( componentResourceCollectionIdName );

	Red::Core::ResourceManagement::CResourceBuildDatabase::ResourceIndex componentResourceCollectionIndex = m_buildDb->FindResourceById( componentResourceCollectionId );

	// Component resource collection *MAY* not actually exist
	if( componentResourceCollectionIndex != Red::Core::ResourceManagement::CResourceBuildDatabase::c_InvalidIndex )
	{
		m_buildDb->AddDependency( componentResourceCollectionIndex, intermediateEntityGroupResourceIndex );
	}

	// Be sure to unload the layer.
	layerInfo->SyncUnload();
}

#endif

#endif
