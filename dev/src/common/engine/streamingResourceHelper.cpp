#include "build.h"

#ifdef USE_RED_RESOURCEMANAGER

#include "../core/bundleHeader.h"
#include "../core/bundledefinition.h"
#include "../engine/meshComponent.h"
#include "meshStreaming.h"



//////////////////////////////////////////////////////////////////////////
CStreamingResourceHelper::~CStreamingResourceHelper()
{
}

//////////////////////////////////////////////////////////////////////////
void CStreamingResourceHelper::SaveComponentToCollection( CMeshComponent* component, CComponentResourceCollection* collection, Uint8 streamingTreeDepth )
{
	component->ClearFlag( OF_Transient );
	component->SetStreamingTreeDepth( static_cast< Uint8 >( streamingTreeDepth ) );

	// Serialize to memory
	TDynArray< Uint8 > tempData;
	CMemoryFileWriter writer( tempData );
	CDependencySaver saver( writer, nullptr );

	// Save
	DependencySavingContext context( component );
	context.m_saveTransient = true;
	context.m_saveReferenced = true;
	saver.SaveObjects( context );

	// Copy data
	DataBuffer dataBuffer( MC_ResourceBuffer, static_cast< void* > ( tempData.TypedData() ), tempData.Size() );
	collection->AddDataBuffer( dataBuffer );

	component->SetFlag( OF_Transient );
}

Red::Core::ResourceManagement::CResourceBuildDatabase::ResourceIndex CStreamingResourceHelper::ProcessMeshComponent( CMeshComponent* component, Red::Core::ResourceManagement::CResourceBuildDatabase& buildDb, CDirectory& workingDirectory, BundleFileDescriptions& bundleDescriptions )
{
	// Here, we will strip out the mesh as an import from the component. We will also save the mesh and its VB + IB as
	// separate resources in the working directory for bundling
	CMesh* meshResource = component->GetMesh();
	if( meshResource == nullptr )
	{
		return Red::Core::ResourceManagement::CResourceBuildDatabase::c_InvalidIndex;
	}

	// Grab the meshes disk file.
	CDiskFile* meshFile = meshResource->GetFile();

	String meshDepotPath = workingDirectory.GetDepotPath() + meshFile->GetFileName();

	Red::Core::ResourceManagement::CResourceId meshResourceId( meshDepotPath );

	// Check to see if the mesh already exists in the database. We can check by looking for the specific name of the mesh.
	Red::Core::ResourceManagement::CResourceBuildDatabase::ResourceIndex meshResourceIndex = buildDb.FindResourceById( meshResourceId );

	if( meshResourceIndex == Red::Core::ResourceManagement::CResourceBuildDatabase::c_InvalidIndex )
	{
		// Create database information object for the mesh.
		// We alter some of this via processing.
		SResourceDatabaseItemCreationData meshResourceItemData;
		meshResourceItemData.m_typeName = String( TXT("CMesh") );
		meshResourceItemData.m_resourceName = meshFile->GetFileName();
		meshResourceItemData.m_sourcePath = meshFile->GetDepotPath();

		// First, pull out all the VBs and IBs for streaming
		TDynArray< CStreamedMeshChunks > streamingMeshChunks;
		MeshStreamingUtils::GenerateChunksFromMesh( *meshResource, streamingMeshChunks );

		// Next, strip the chunk data from the existing mesh in memory
		meshResource->StripChunkData();

		// Finally, save the new mesh resource to the working directory for streaming
		MeshStreamingUtils::SaveStrippedMeshToDisk( workingDirectory, *meshResource );
		meshResourceItemData.m_resourceIDName = meshDepotPath;
		
		// We don't have a GUID for the mesh, but we'll create one for the database.
		meshResourceItemData.m_guid = Red::System::GUID::Create();

		meshResourceIndex = CStreamingResourceHelper::AddNewResourceToDatabase( buildDb, meshResourceItemData );

		CDiskFile tempMeshPath( &workingDirectory, meshFile->GetFileName() );
		CreateBundleFileDescription( &tempMeshPath, GET_FOURCC( CMesh ), Red::Core::ResourceManagement::CT_Default, meshResourceIndex, buildDb, bundleDescriptions );

		// Now save each mesh chunk
		for( Uint32 i = 0; i < streamingMeshChunks.Size(); ++i )
		{
			// Create a database item for the mesh chunk.
			SResourceDatabaseItemCreationData meshChunkResourceDatabaseItem;
			
			streamingMeshChunks[i].SetMeshResource( buildDb.GetResourceId( meshResourceIndex ) );
			meshChunkResourceDatabaseItem.m_resourceName = MeshStreamingUtils::SaveMeshChunksToDisk( workingDirectory, meshFile->GetName(), i, streamingMeshChunks[i] );
			meshChunkResourceDatabaseItem.m_resourceIDName = workingDirectory.GetDepotPath() + meshChunkResourceDatabaseItem.m_resourceName;
			meshChunkResourceDatabaseItem.m_typeName = String( TXT("CStreamedMeshChunks") );
			meshChunkResourceDatabaseItem.m_sourcePath = meshChunkResourceDatabaseItem.m_resourceIDName;
			// We don't have a GUID for the mesh chunks, so we'll create one for the database here.
			meshChunkResourceDatabaseItem.m_guid = Red::System::GUID::Create();
			meshChunkResourceDatabaseItem.m_dependencies.PushBack( meshResourceIndex );

			Red::Core::ResourceManagement::CResourceBuildDatabase::ResourceIndex chunkResourceIndex = CStreamingResourceHelper::AddNewResourceToDatabase( buildDb, meshChunkResourceDatabaseItem );

			CDiskFile chunkDiskFile( &workingDirectory, meshChunkResourceDatabaseItem.m_resourceName );CreateBundleFileDescription( &chunkDiskFile, GET_FOURCC( CStreamedMeshChunks ), Red::Core::ResourceManagement::CT_Default, chunkResourceIndex, buildDb, bundleDescriptions );
		}

		// Now, remove the mesh link from the component so it does not get saved as an import
		component->SetMesh( nullptr );

		// Set the streaming mesh hash for fix-up on component load.
		component->SetStreamedMeshResourceId( buildDb.GetResourceId( meshResourceIndex ) );
	}
	
	// Finally add the mesh and its chunks to the bundle
	return meshResourceIndex;
}

//////////////////////////////////////////////////////////////////////////
String CStreamingResourceHelper::BuildLayerGroupString( CLayerGroup* layerGroup )
{
	CLayerGroup* parentGroup = layerGroup->GetParentGroup();
	if( parentGroup != nullptr )
	{
		return BuildLayerGroupString( parentGroup ) + TXT( "_" ) + layerGroup->GetName();
	}
	return layerGroup->GetName();
}


//////////////////////////////////////////////////////////////////////////
void CStreamingResourceHelper::CreateBundleFileDescription( CDiskFile* diskFile, const Uint32 fourCC, const Red::Core::ResourceManagement::ECompressionType compressionType, Uint32 thisResource,  Red::Core::ResourceManagement::CResourceBuildDatabase& resourceDatabase, BundleFileDescriptions& bundleDescriptions )
{
	Red::Core::ResourceManagement::CResourceId fileHash( diskFile->GetDepotPath() );
	for( Uint32 i = 0; i < bundleDescriptions.Size(); ++i )
	{
		if( bundleDescriptions[i]->m_resourceId == fileHash )
		{
			return ;
		}
	}

	// Create a bundle file description object to store the results in.
	Red::Core::BundleDefinition::SBundleFileDesc* bundleFileDesc = new Red::Core::BundleDefinition::SBundleFileDesc();

	String diskfileDepotPath = diskFile->GetDepotPath();

	// Create a reader for this file, so we can tell how large it is.
	IFile* reader = diskFile->CreateReader();
	RED_ASSERT( reader != nullptr, TXT("Unable to read file %s, prepare for crashes" ), diskfileDepotPath.AsChar() );
	
	// Fill out the bundle file description object
	bundleFileDesc->m_resourceId = fileHash; // <- Really need to address this hashing at some point.
	bundleFileDesc->m_fourCC = fourCC;
	bundleFileDesc->m_cookedResourcePath = StringAnsi( UNICODE_TO_ANSI( diskfileDepotPath.AsChar() ) );
	bundleFileDesc->m_rawResourcePath = StringAnsi::EMPTY;
	bundleFileDesc->m_fileSize = static_cast< Uint32 >( GFileManager->GetFileSize( diskFile->GetAbsolutePath() ) );
	bundleFileDesc->m_compressionType = compressionType;

	TSortedSet< Red::Core::ResourceManagement::CResourceBuildDatabase::ResourceIndex > dependencies;
	resourceDatabase.GetImmediateDependencies( thisResource, dependencies );
	Uint32 currentDependencyCount = 0;
	auto it = dependencies.Begin();
	while( it != dependencies.End() )
	{
		RED_ASSERT( currentDependencyCount < Red::Core::ResourceManagement::SBundleHeaderItem::MAX_DEPENDENCY_ITEMS );
		Red::Core::ResourceManagement::CResourceId resourceID = resourceDatabase.GetResourceId( *it );
		bundleFileDesc->m_resourceDependencies[currentDependencyCount++] = resourceID;
		++it;
	}
	bundleDescriptions.PushBack( bundleFileDesc );

	// Clean up memory allocation.
	delete reader;
}

//////////////////////////////////////////////////////////////////////////
void CStreamingResourceHelper::PrepareEntityForResaved( CEntity* entity )
{
	const TDynArray<CComponent*>& components = entity->GetComponents();
	// Prepare entities for re-save.
	Bool hasMeshComponents = false;
	for( Uint32 i = 0; i < components.Size(); ++i )
	{
		if(components[i]->IsA< CMeshComponent >() )
		{
			hasMeshComponents = true;
			break;
		}
	}

	if( hasMeshComponents )
	{
		entity->SetStreamed( true );
	}

	entity->RemoveStreamedComponents();
}

//////////////////////////////////////////////////////////////////////////
Red::Core::ResourceManagement::CResourceBuildDatabase::ResourceIndex CStreamingResourceHelper::ResaveEntityGroupResources( CLayerInfo* layerInfo, Red::Core::ResourceManagement::CResourceBuildDatabase& database, Red::Core::ResourceManagement::CResourceBuildDatabase::ResourceIndex entityGroupResourceIndex, CDirectory& workingDirectory, BundleFileDescriptions& bundleDescriptions )
{
	RED_ASSERT( layerInfo->IsLoaded(), TXT("Layer %s should be loaded in order resave the entity group resource associated with it!"), layerInfo->GetFriendlyName().AsChar() );

	CLayer* currentLayer = layerInfo->GetLayer();
	currentLayer->SetOwnEntities( false );
	currentLayer->SetSaveRaw( false );

	SResourceDatabaseItemCreationData intermediateEntityGroupResourceDatabaseItem;
	intermediateEntityGroupResourceDatabaseItem.m_typeName = String( TXT("CEntityGroupResource") );
	intermediateEntityGroupResourceDatabaseItem.m_sourcePath = ANSI_TO_UNICODE( database.GetResourceSourcePath( entityGroupResourceIndex ) );
	intermediateEntityGroupResourceDatabaseItem.m_resourceName = workingDirectory.GetDepotPath() + layerInfo->GetShortName() + String( TXT( ".ents" ) );
	intermediateEntityGroupResourceDatabaseItem.m_resourceIDName = intermediateEntityGroupResourceDatabaseItem.m_resourceName;
	intermediateEntityGroupResourceDatabaseItem.m_guid = Red::System::GUID::Create();
	Red::Core::ResourceManagement::CResourceBuildDatabase::ResourceIndex intermediateEntityGroupResourceIndex = CStreamingResourceHelper::AddNewResourceToDatabase( database, intermediateEntityGroupResourceDatabaseItem );

	CDiskFile streamedEntityGroupResourceFile( &workingDirectory,  layerInfo->GetShortName() + String( TXT( ".ents" ) ) );
	currentLayer->StoreStreamedEntityGroupResource( streamedEntityGroupResourceFile );

	CStreamingResourceHelper::CreateBundleFileDescription( &streamedEntityGroupResourceFile, GET_FOURCC( CEntityGroupResource ), Red::Core::ResourceManagement::CT_Default, intermediateEntityGroupResourceIndex, database, bundleDescriptions );

	layerInfo->Save();
	return intermediateEntityGroupResourceIndex;
}

//////////////////////////////////////////////////////////////////////////
Red::Core::ResourceManagement::CResourceBuildDatabase::ResourceIndex CStreamingResourceHelper::AddNewResourceToDatabase( Red::Core::ResourceManagement::CResourceBuildDatabase& resourceDatabase, const SResourceDatabaseItemCreationData& resourceCreationData )
{
	RED_ASSERT( !resourceCreationData.m_typeName.Empty(), TXT( "Resource data creation *MUST* have the type name filled out." ) );
	RED_ASSERT( !resourceCreationData.m_sourcePath.Empty(), TXT( "Resource data creation *MUST* have the source path filled out." ) );
	RED_ASSERT( !resourceCreationData.m_resourceIDName.Empty(), TXT( "Resource data creation *MUST* have the resource ID filled out." ) );
	RED_ASSERT( !resourceCreationData.m_resourceName.Empty(), TXT( "Resource data creation *MUST* have the resource Name filled out." ) );
	RED_ASSERT( !resourceCreationData.m_guid.IsZero(), TXT( "Resource data creation *MUST* have it's GUID filled out." ) );

	Red::Core::ResourceManagement::CResourceId resourceId( resourceCreationData.m_resourceIDName );

	// See if we can find the resource by it's name.
	Red::Core::ResourceManagement::CResourceBuildDatabase::ResourceIndex resourceIndex = resourceDatabase.FindResourceById( resourceId );

	// If we don't find it we need to add it.
	if( resourceIndex == Red::Core::ResourceManagement::CResourceBuildDatabase::c_InvalidIndex )
	{
		resourceIndex = resourceDatabase.AddNewResource( UNICODE_TO_ANSI( resourceCreationData.m_typeName.AsChar() ), resourceId );

		resourceDatabase.SetResourceName( resourceIndex, UNICODE_TO_ANSI( resourceCreationData.m_resourceName.AsChar() ) );
		resourceDatabase.SetResourcePath( resourceIndex, UNICODE_TO_ANSI( resourceCreationData.m_sourcePath.AsChar() ) );
		resourceDatabase.SetResourceGUID( resourceIndex, resourceCreationData.m_guid );

		Uint32 dependencySize = resourceCreationData.m_dependencies.Size();
		for( Uint32 i = 0; i < dependencySize; ++i )
		{
			resourceDatabase.AddDependency( resourceIndex, resourceCreationData.m_dependencies[i] );
		}
	}
	return resourceIndex;
}
#endif
