#include "build.h"
#include "streamingFoliageConversionPlugin.h"
#include "foliageScene.h"
#include "foliageGrid.h"
#include "foliageCell.h"
#include "foliageResourceLoader.h"
#include "grassMask.h"
#include "../core/dependencyLoader.h"
#include "../core/depot.h"
#include "../core/dependencySaver.h"
#include "../core/garbageCollector.h"

#if 0

const Int32 GGarbageCollectionCountdown = 10;
const Vector2 GCellDimension = Vector2( 64.0f, 64.0f );

IMPLEMENT_ENGINE_CLASS( CFoliageConversionGroup );

class FoliageResourceCreator : public IFoliageResourceLoader
{
public:

	FoliageResourceCreator( SChangelist* changelist )
		: m_changelist( changelist )
	{}

	virtual FoliageResourceHandle GetResourceHandle( const Vector2 & position ) const
	{
		return CreateResource( position );
	}

	virtual void ResourceAcquired( CFoliageResource * resource ) const
	{}

	virtual void ResourceReleased( CFoliageResource * resource ) const
	{
		resource->Save();
		CDiskFile * file = resource->GetFile();
		file->SetChangelist( *m_changelist );
		file->Add();
	}

	virtual FoliageResourceHandle CreateResource( const Vector2 & position ) const
	{
		CFoliageResource * resource = new CFoliageResource;
		const Box box = Box( position, position + GCellDimension );
		resource->SetBox( box );

		CDiskFile * file = GDepot->CreateFile( GEngine->GetPathManager().GetPath( Red::Core::ResourceManagement::CResourcePaths::Path_FoliageSourceData ) + GenerateFoliageFilename( position ) );
		file->BindResource( resource );
		return FoliageResourceHandle( new CSoftHandleProxy( resource ) );
	}

private:

	SChangelist * m_changelist;
};

///////////////////////////////////////////////////////////////
CFoliageStreamingConversionPlugin::CFoliageStreamingConversionPlugin()
	: m_changelist( nullptr )
	, m_gcCollectionCounter( 0 )
{

}

///////////////////////////////////////////////////////////////
CFoliageStreamingConversionPlugin::~CFoliageStreamingConversionPlugin()
{

}

///////////////////////////////////////////////////////////////
String CFoliageStreamingConversionPlugin::GenerateCombinedInstanceDataPath( CLayer* parentLayer, CLayerFoliage* foliageLayer )
{
	String filename = GEngine->GetPathManager().GetPath( Red::Core::ResourceManagement::CResourcePaths::Path_WorkingDirectoryDepot );
	String boxString = String::Printf( TXT( "( %f, %f )( %f, %f )" ), foliageLayer->GetBox().Min.X, foliageLayer->GetBox().Min.Y, foliageLayer->GetBox().Max.X, foliageLayer->GetBox().Max.Y );
	filename += parentLayer->GetFile()->GetName();
	filename += TXT( "_" );
	filename += boxString;
	filename += TXT( ".foliage.tmp" );
	return filename;
}

///////////////////////////////////////////////////////////////
void CFoliageStreamingConversionPlugin::Initialise( CWorld* world, CGuidValidator* guidValidator, SChangelist* changelist )
{
	RED_UNUSED( guidValidator );

	m_world = world;
	m_changelist = changelist;

	m_processedStreamingTileFiles.Clear();
}

///////////////////////////////////////////////////////////////
TDynArray< CFoliageConversionGroup* >* CFoliageStreamingConversionPlugin::LoadCombinedInstanceData( String path )
{
	TDynArray< CFoliageConversionGroup* >* groups = nullptr;
	IFile* fileReader = GFileManager->CreateFileReader( path, FOF_Buffered );
	if( fileReader )
	{
		CDependencyLoader loader( *fileReader, nullptr );
		DependencyLoadingContext loadingContext;
		if ( !loader.LoadObjects( loadingContext ) )
		{
			RED_HALT( "Something went wrong while loading merged instance groups" );
			delete fileReader;
			return nullptr;
		}

		groups = new TDynArray< CFoliageConversionGroup* >();
		groups->Reserve( loadingContext.m_loadedRootObjects.Size() );
		for( auto it = loadingContext.m_loadedRootObjects.Begin(); it != loadingContext.m_loadedRootObjects.End(); ++it )
		{
			RED_ASSERT( (*it), TXT( "Bad conversion group object" ) );
			groups->PushBack( Cast<CFoliageConversionGroup>( *it ) );
		}

		delete fileReader;
	}

	return groups;
}

///////////////////////////////////////////////////////////////
void CFoliageStreamingConversionPlugin::SaveCombinedInstanceData( TDynArray< CFoliageConversionGroup* >* data, String path )
{
	IFile* fileWriter = GFileManager->CreateFileWriter( path, FOF_Buffered );
	Uint32 groupCount = data->Size();
	if( fileWriter )
	{
		TDynArray< CObject* > objectsToSave;
		for( auto it = data->Begin(); it != data->End(); ++it )
		{
			RED_ASSERT( (*it), TXT( "Bad conversion group object" ) );
			objectsToSave.PushBack( Cast<CObject>(*it) );
		}

		CDependencySaver saver( *fileWriter, nullptr );
		DependencySavingContext context( objectsToSave );
		context.m_saveReferenced = false;
		context.m_saveTransient = false;

		if ( !saver.SaveObjects( context ) )
		{
			RED_HALT( "Something went wrong while saving merged instance groups" );
		}
	}
	delete fileWriter;
}

///////////////////////////////////////////////////////////////
void CFoliageStreamingConversionPlugin::RefreshGc()
{
	++m_gcCollectionCounter;
	if( m_gcCollectionCounter > GGarbageCollectionCountdown )
	{
		m_gcCollectionCounter = 0;
		SGarbageCollector::GetInstance().CollectNow();
	}
}

///////////////////////////////////////////////////////////////
void CFoliageStreamingConversionPlugin::TakeOwnershipOfLayerFoliage( CLayer* sourceLayer )
{
	// Handle foliage data on global layers
	// In this case, all we can really do is strip the foliage from the layers
	if( sourceLayer->GetFoliageData() )
	{
		// Remove the foliage layer object from the layer
		sourceLayer->RemoveFoliageData();

		// We need to ensure the foliage data is stripped from the layer
		sourceLayer->GetFile()->SetChangelist( *m_changelist );
		Bool checkoutOk = sourceLayer->GetFile()->SilentCheckOut();
		RED_ASSERT( checkoutOk, TXT( "Failed to check out file silently" ) );
		sourceLayer->MarkModified();

		// Switch the layer info flags. We then need to mark the layer info / group as modified so the layer info data gets updated
		sourceLayer->GetLayerInfo()->SetContainstVegetationData( false );
// ChrisH: Layer group files are being removed so this shouldn't be required anymore.
#if 0
		CLayerGroup* layerInfoParentGroup = sourceLayer->GetLayerInfo()->GetLayerGroup();
		layerInfoParentGroup->GetFile()->SetChangelist( *m_changelist );
		checkoutOk = layerInfoParentGroup->GetFile()->SilentCheckOut();
		RED_ASSERT( checkoutOk, TXT( "Failed to check out file silently" ) );
		layerInfoParentGroup->MarkModified();
#endif
		sourceLayer->GetLayerInfo()->MarkModified();
	}
}

///////////////////////////////////////////////////////////////
void CFoliageStreamingConversionPlugin::OnWorldLayer( CLayer* loadedLayer )
{
	if( loadedLayer->GetFoliageData() )
	{
		// This gets all the instances out and recombines them to one bucket (strips duplicates over LODs)
		TDynArray< CFoliageConversionGroup* >* layerInstanceGroups = ExtractAllInstanceGroups( loadedLayer->GetFoliageData() );

		// Save them to a temp directory for later (we don't really care about the layers any more)
		String dataPath = GenerateCombinedInstanceDataPath( loadedLayer, loadedLayer->GetFoliageData() );
		SaveCombinedInstanceData( layerInstanceGroups, dataPath );
		m_processedInstanceData.PushBack( dataPath );
		delete layerInstanceGroups;

		RefreshGc();

		// Strip the foliage data from the source layer
		TakeOwnershipOfLayerFoliage( loadedLayer );
	}
}

///////////////////////////////////////////////////////////////
void CFoliageStreamingConversionPlugin::OnWorldStreamingTile( CLayer* worldLayer, CLayer* streamingTileLayer )
{
	String currentTileFilename = streamingTileLayer->GetFile()->GetDepotPath();
	if( m_processedStreamingTileFiles.Exist( currentTileFilename ) )
	{
		return ;
	}

	// This gets all the instances out and recombines them to one bucket (strips duplicates over LODs)
	if( streamingTileLayer->GetFoliageData() )
	{
		TDynArray< CFoliageConversionGroup* >* layerInstanceGroups = ExtractAllInstanceGroups( streamingTileLayer->GetFoliageData() );
		String dataPath = GenerateCombinedInstanceDataPath( streamingTileLayer, streamingTileLayer->GetFoliageData() );
		SaveCombinedInstanceData( layerInstanceGroups, dataPath );
		m_processedInstanceData.PushBack( dataPath );
		delete layerInstanceGroups;

		RefreshGc();
	}

	// Process shit
	m_processedStreamingTileFiles.PushBack( currentTileFilename );
}

///////////////////////////////////////////////////////////////
CFoliageConversionGroup* CFoliageStreamingConversionPlugin::FindOrCreateGroup( CSRTBaseTree* baseTree, TDynArray< CFoliageConversionGroup* >& sourceGroups, const Box& bounds )
{
	CFoliageConversionGroup* result = nullptr;
	for( auto it = sourceGroups.Begin(); it != sourceGroups.End(); ++it )
	{
		if( (*it)->m_baseTree == baseTree )
		{
			result = (*it);
			break;
		}
	}

	if( !result )
	{
		CFoliageConversionGroup* newGroup = CreateObject< CFoliageConversionGroup >( );
		newGroup->m_baseTree = baseTree;
		newGroup->m_bounds = bounds;
		sourceGroups.PushBack( newGroup );
		result = sourceGroups[ sourceGroups.Size() - 1 ];
	}

	return result;
}

///////////////////////////////////////////////////////////////
TDynArray< CFoliageConversionGroup* >* CFoliageStreamingConversionPlugin::ExtractAllInstanceGroups( CLayerFoliage* sourceLayer )
{
	TDynArray< CFoliageConversionGroup* >* newGroupList = new TDynArray< CFoliageConversionGroup* >();

	Uint32 layerInstanceCount = 0;
	Uint32 totalInstancesConverted = 0;

	// Combine all the LODs into one mega-bucket for each group
	TDynArray< CFoliageInstance* > tempInstanceBuffer;			// Don't keep resizing this thing
	for( Uint32 lodindex = 0; lodindex < 3; ++lodindex )
	{
		Uint32 lodInstanceCount = 0;
		sourceLayer->LoadBucketSync( lodindex );
		if( sourceLayer->GetBucket( lodindex ) )
		{
			const CFoliageInstancesBucket::TGroupList& sourceGroups = sourceLayer->GetBucket( lodindex )->GetGroups();
			for( auto groupIt = sourceGroups.Begin(); groupIt != sourceGroups.End(); ++groupIt )
			{
				// Extract the instances from the group for this LOD
				CFoliageContainer* instanceContainer = (*groupIt).m_container;
				CSRTBaseTree* baseTree = (*groupIt).m_baseTree.Get();

				// Find the matching group in the target data
				CFoliageConversionGroup* targetGroup = FindOrCreateGroup( baseTree, *newGroupList, sourceLayer->GetBox() );

				// Add unique instances to the target group. This is pretty slow!
				Uint32 instanceCount = instanceContainer->GetInstanceCount();
				lodInstanceCount += instanceCount;
				tempInstanceBuffer.ClearFast();
				tempInstanceBuffer.Reserve( instanceCount );
				instanceContainer->GetInstances( tempInstanceBuffer );

				// Reserve the max required
				targetGroup->m_instances.Reserve( targetGroup->m_instances.Size() + instanceCount );
				for( auto it = tempInstanceBuffer.Begin(); it != tempInstanceBuffer.End(); ++it )
				{
					// If an instance with this position already exists for a lower lod, then we skip it if they are identical
					Uint32 instanceIndex = targetGroup->m_instances.Size();
					Uint32 groupInstanceHash = CalculateVectorHash( (*it)->m_position );
					if( !targetGroup->m_tempInstancesMap.Insert( groupInstanceHash, instanceIndex ) )
					{
						Uint32 existingInstance = targetGroup->m_tempInstancesMap[ groupInstanceHash ];

						// The hashes match, do the positions match too (i.e. could be a collision)
						if( targetGroup->m_instances[ existingInstance ].m_position == (*it)->m_position )
						{
							// If the positions match but other data is different, we will accept the instance anyway
							if( targetGroup->m_instances[ existingInstance ] == *(*it) )
							{
								CFoliageInstance& thisInstance = *(*it);
								CFoliageInstance& otherInstance = targetGroup->m_instances[ existingInstance ];
								LOG_CORE( TXT( "Identical instances at the same position! Some will be skipped" ) );
								continue;
							}
						}
					}
					targetGroup->m_instances.PushBack( *(*it) );
					++totalInstancesConverted;
				}

				// Shrink so we don't keep too much memory allocated
				targetGroup->m_instances.Shrink();
			}
		}
		sourceLayer->UnloadBucket( lodindex );
		layerInstanceCount += lodInstanceCount;
		LOG_CORE( TXT( "%d instances in LOD %d" ), lodInstanceCount, lodindex );
	}

	LOG_CORE( TXT( "%d instances total in source data; %d instances added to converted data" ), layerInstanceCount, totalInstancesConverted );

	return newGroupList;
}


CFoliageScene * CreateFoliageScene( CWorld * world )
{
	CFoliageScene* foliageScene = CreateObject< CFoliageScene >( world, 0 );
	foliageScene->SetWorldDimensions( world->GetWorldDimensions() );
	foliageScene->SetCellDimensions( GCellDimension );

	const String worldFileNameNoExt = world->GetFile()->GetFileName().StringBefore( TXT(".") );
	const String grassMaskFilename = String::Printf( TXT("%s.%s"), worldFileNameNoExt.AsChar(), CGenericGrassMask::GetFileExtension() );
	CDiskFile* grassMaskFile = world->GetFile()->GetDirectory()->FindFile( grassMaskFilename );
	if( grassMaskFile && grassMaskFile->Load() )
	{
		CGenericGrassMask * grassMask = SafeCast< CGenericGrassMask >( grassMaskFile->GetResource() );
		foliageScene->SetGenericGrassMask( grassMask );
	}

	return foliageScene;
}

///////////////////////////////////////////////////////////////
void CFoliageStreamingConversionPlugin::OnFinaliseConversion()
{
	LOG_CORE( TXT( "%d foliage layers to process..." ), m_processedInstanceData.Size() );

	FoliageResourceCreator creator( m_changelist );

	SFoliageGridSetupParameters gridparam = 
		{ 
			Vector2( m_world->GetWorldDimensions() ),
			GCellDimension,
			&creator
		};

	CFoliageGrid grid;
	grid.Setup( gridparam );

	m_world->SetFoliageScene( CreateFoliageScene( m_world ) );
	m_world->GetFile()->SetChangelist( *m_changelist );
	Bool checkoutOk = m_world->GetFile()->SilentCheckOut();
	RED_ASSERT( checkoutOk, TXT( "Failed to check out world file!" ) );
	m_world->MarkModified();

	Uint32 progress = 0;
	for( auto convertedData = m_processedInstanceData.Begin(); convertedData != m_processedInstanceData.End(); ++convertedData )
	{
		RefreshGc();

		TDynArray< CellHandle > generatedCell;

		GFeedback->UpdateTaskProgress( progress, m_processedInstanceData.Size() );
		TDynArray< CFoliageConversionGroup* >* layerSourceData = LoadCombinedInstanceData( *convertedData );
		if( layerSourceData && layerSourceData->Size() > 0 )
		{
			for( auto groupIt = layerSourceData->Begin(); groupIt != layerSourceData->End(); ++groupIt )
			{
				CSRTBaseTree* baseTree = (*groupIt)->m_baseTree.Get();
				const TDynArray< CFoliageInstance >& sourceInstances = (*groupIt)->m_instances;

				for( auto instanceIt = sourceInstances.Begin(); instanceIt != sourceInstances.End(); ++instanceIt )
				{
					CellHandle cell = grid.AcquireCell( (*instanceIt).m_position );
					cell->Tick();
					CFoliageResource * resource = cell->GetFoliageResource();

					CFoliageContainer::SInstanceInfo instance;
					instance.m_position = (*instanceIt).m_position;
					instance.m_color = (*instanceIt).m_color;
					instance.m_rotation = (*instanceIt).m_rotation;
					instance.m_scale = (*instanceIt).m_scale;

					resource->InsertInstance( baseTree, instance, 0 );
					generatedCell.PushBack( cell );
				}
			}
		}
		delete layerSourceData;
	}
}

///////////////////////////////////////////////////////////////
void CFoliageStreamingConversionPlugin::OnShutdown()
{
}

#endif