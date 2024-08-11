#include "build.h"
#include "pathlibAreaDescription.h"

#include "../core/versionControl.h"
#include "../core/directory.h"

#include "baseEngine.h"
#include "pathlibAreaProcessingJob.h"
#include "pathlibConnectorsBin.h"
#include "pathlibCookerData.h"
#include "pathlibMetalink.h"
#include "pathlibNavgraph.h"
#include "pathlibNavmeshArea.h"
#include "pathlibObstacle.h"
#include "pathlibObstaclesMap.h"
#include "pathlibWorld.h"
#include "pathlibSimpleBuffers.h"
#include "pathlibStreamingManager.h"
#include "pathlibTerrain.h"



namespace PathLib
{


////////////////////////////////////////////////////////////////////////////
// CAreaDescription
////////////////////////////////////////////////////////////////////////////
CAreaDescription::CAreaDescription()
	: m_pathlib( NULL )
	, m_id( INVALID_AREA_ID )
	, m_areaFlags( FLAGS_Default )
	, m_usedCategories( 0xff )
	, m_isDirty( 0 )
	, m_isProcessed( false )
	, m_bbox( Vector(0,0,0), Vector(0,0,0) )

{
}
CAreaDescription::CAreaDescription( CPathLibWorld& pathlib, Id id )
	: m_pathlib( &pathlib )
	, m_id( id )
	, m_areaFlags( FLAGS_Default )
	, m_usedCategories( 0xff )
	, m_isDirty( 0 )
	, m_isProcessed( false )
	, m_bbox( Vector(0,0,0), Vector(0,0,0) )
{
}
CAreaDescription::~CAreaDescription()
{
}
void CAreaDescription::Clear()
{
}
void CAreaDescription::SetPathLib( CPathLibWorld* pathlib )
{
	m_pathlib = pathlib;
}

void CAreaDescription::GatherNeighbourAreas( TSortedArray< AreaId >& outAreas ) const
{
	// Look through global inter-area connectors and determine areas we are connected 2.
	if ( CNavigationCookingContext* cookingContext = m_pathlib->GetCookingContext() )
	{
		CGlobalConnectorsBin* globalData = cookingContext->GetPathlibCookerData()->GetGlobalConnections();

		// NOTICE: we assume that we will iterate through area connections area-by-area. So all connections to specyfic area will come one-by-one, and all areas will come sorted by their AreaId
		PathLib::AreaId lastArea = INVALID_AREA_ID;
		CGlobalConnectorsBin::Iterator it( *globalData, m_id );
		while ( it )
		{
			CGlobalConnectorsBin::Connection& c = *it;

			if ( c.m_areaTo != lastArea )
			{
				lastArea = c.m_areaTo;
				// NOTICE: we assume areas come in proper order - so we can use pushback interface instead of insertions
				ASSERT( outAreas.Find( lastArea ) == outAreas.End() );
				outAreas.PushBack( lastArea );
			}

			++it;
		}

		ASSERT( outAreas.IsSorted() );
	}
}

void CAreaDescription::VLocalToWorld( Box& v ) const		{}
void CAreaDescription::VWorldToLocal( Box& v ) const		{}
void CAreaDescription::VLocalToWorld( Vector3& v ) const	{}
void CAreaDescription::VWorldToLocal( Vector3& v ) const	{}
void CAreaDescription::VLocalToWorld( Vector2& v ) const	{}
void CAreaDescription::VWorldToLocal( Vector2& v ) const	{}
Float CAreaDescription::VLocalToWorldZ( Float z ) const		{ return z; }
Float CAreaDescription::VWorldToLocalZ( Float z ) const		{ return z; }
void CAreaDescription::ConnectWithNeighbours()
{
	CAreaNavgraphs* res = m_graphs.Get();
	if ( !res )
	{
		return;
	}

	auto fun = [] ( CNavGraph* g )
	{
		g->ConnectWithNeighbours();
	};

	res->IterateGraphs( fun );
}
void CAreaDescription::DetachFromNeighbours()
{
	CAreaNavgraphs* res = m_graphs.Get();
	if ( !res )
	{
		return;
	}
	auto fun = [] ( CNavGraph* g )
	{
		g->DetachFromNeighbours();
	};

	res->IterateGraphs( fun );
}
void CAreaDescription::Initialize()
{
	MarkDirty( DIRTY_ALL );
}
void CAreaDescription::OnRemoval()
{

}
Bool CAreaDescription::IsNavgraphsModified() const
{
	CAreaNavgraphs* res = m_graphs.Get();
	if ( !res )
	{
		return false;
	}
	Bool isModified = false;
	auto fun = [ &isModified ] ( CNavGraph* g )
	{
		isModified = isModified || !g->IsInitialVersion();
	};

	res->IterateGraphs( fun );

	return isModified;
}

CNavModyficationMap* CAreaDescription::LazyInitializeModyficationSet()
{
	return LazyInitializeObstaclesMap();
}
CObstaclesMap* CAreaDescription::GetObstaclesMap() const
{
	return m_obstaclesMap.Get();
}
CNavModyficationMap* CAreaDescription::GetMetalinks() const
{
	return m_obstaclesMap.Get();
}
CObstaclesMap* CAreaDescription::LazyInitializeObstaclesMap()
{
	CObstaclesMap* obstaclesMap = m_obstaclesMap.Get();
	if ( !obstaclesMap )
	{
		// mostly its editor - only functionality it will happen in extremely rare situations
		obstaclesMap = m_obstaclesMap.Construct();
		obstaclesMap->OnPreLoad( this );
		obstaclesMap->OnPostLoad( this );
		m_areaFlags &= ~FLAG_NoObstaclesMap;
	}

	return obstaclesMap;
}

void CAreaDescription::GetAreaFileList( TDynArray< CDiskFile* >& fileList, Bool onlyChanged )
{
	CDirectory* dir = m_pathlib->GetSourceDataDirectory();
	if ( !dir )
	{
		return;
	}

	String fileName;

	CObstaclesMap* obstaclesMap = GetObstaclesMap();
	if ( obstaclesMap && (!onlyChanged || !obstaclesMap->IsInitialVersion()) )
	{
		m_pathlib->GetObstaclesFileName( m_id, fileName );
		CDiskFile* file = dir->FindLocalFile( fileName );
		if ( file )
		{
			fileList.PushBack( file );
		}
	}

	if ( !onlyChanged || IsNavgraphsModified() )
	{
		m_pathlib->GetNavgraphFileName( m_id, fileName );
		CDiskFile* file = dir->FindLocalFile( fileName );
		if ( file )
		{
			fileList.PushBack( file );
		}
	}
}

Bool CAreaDescription::IterateAreaResources( ResourceFunctor& functor )
{
	// navgraphs
	if ( !functor.Handle( m_graphs, m_graphs.GetResType() ) )
	{
		return false;
	}

	// obstacles
	if ( !functor.Handle( m_obstaclesMap, m_obstaclesMap.GetResType() ) )
	{
		return false;
	}

	return true;
}

CAreaDescription::EType CAreaDescription::GetType() const
{
	if ( IsTerrainArea() )
	{
		return TYPE_TERRAIN;
	}
	else
	{
		const CNavmeshAreaDescription* naviArea = static_cast< const CNavmeshAreaDescription* >( this );
		return naviArea->IsUsingTransformation() ? TYPE_INSTANCE_TRANSFORMED : TYPE_INSTANCE_INWORLD;
	}
}

CAreaDescription* CAreaDescription::NewFromBuffer( CSimpleBufferReader& reader )
{
	Int8 type = -1;
	reader.Get( type );
	CAreaDescription* area;
	switch ( type )
	{
	default:
		return NULL;
	case TYPE_TERRAIN:
		area = new CTerrainAreaDescription();
		break;
	case TYPE_INSTANCE_INWORLD:
		area = new CNavmeshAreaDescription();
		break;
	case TYPE_INSTANCE_TRANSFORMED:
		area = new CNavmeshTransformedAreaDescription();
		break;
	}
	if ( !area->ReadFromBuffer( reader ) )
	{
		delete area;
		return NULL;
	}
	return area;
}

void CAreaDescription::WriteToBuffer( CSimpleBufferWriter& writer )
{
	Int8 type = Int8( GetType() );
	writer.Put( type );
	writer.Put( m_id );
	writer.SmartPut( m_obstaclesMap );
	writer.SmartPut( m_graphs );
	AreaFlags flagsToSave = m_areaFlags & FLAGS_Saveable;
	writer.Put( flagsToSave );
	Uint16 dirty = m_isDirty & DIRTY_MASK_SERIALIZABLE;
	writer.Put( dirty );
}

Bool CAreaDescription::ReadFromBuffer( CSimpleBufferReader& reader )
{
	if ( !reader.Get( m_id ) )
		return false;

	if ( !reader.SmartGet( m_obstaclesMap ) )
	{
		return false;
	}
	if ( !reader.SmartGet( m_graphs ) )
	{
		return false;
	}

	if ( !reader.Get( m_areaFlags ) )
	{
		return false;
	}

	if ( !reader.Get( m_isDirty ) )
	{
		return false;
	}

	return true;
}

Bool CAreaDescription::ReadIdFromBuffer( CSimpleBufferReader& reader, EType& areaType, Id& areaId )
{
	Int8 type;
	if ( !reader.Get( type ) )
		return false;

	areaType = EType(type);

	if ( !reader.Get( areaId ) )
		return false;

	return true;
}

template < class TArea, class TQuery >
Bool CAreaDescription::TSpatialQuery( TQuery& query ) const
{
	const TArea* me = static_cast< const TArea* >( this );
	if ( TArea::SPATIALTESTS_IN_LOCAL_SPACE )
	{
		query.Transform( *me );
	}
	Bool ret = true;
	if ( !me->InternalLocalSpatialTest( query ) )
	{
		ret = false;
	}
	else if ( (query.m_flags & CT_NO_OBSTACLES) != CT_NO_OBSTACLES )
	{
		CObstaclesMap* obstaclesMap = m_obstaclesMap.Get();
		if ( obstaclesMap && !obstaclesMap->TSpatialQuery( query ) )
		{
			ret = false;
		}
	}

	if ( TArea::SPATIALTESTS_IN_LOCAL_SPACE )
	{
		query.CancelTransformOutput( *me );
	}
	return ret;
}

template < class TArea, class TQuery >
Bool CAreaDescription::TMultiAreaQueryInternal( const TArea& area, TQuery& query )
{
	if ( TArea::SPATIALTESTS_IN_LOCAL_SPACE )
	{
		query.Transform( area );
		query.TransformOutput( area );
	}
	Bool ret = true;
	if ( !area.InternalLocalSpatialTest( query ) )
	{
		ret = false;
	}
	else if ( (query.m_flags & CT_NO_OBSTACLES) != CT_NO_OBSTACLES )
	{
		CObstaclesMap* obstaclesMap = area.m_obstaclesMap.Get();
		if ( obstaclesMap && !obstaclesMap->TSpatialQuery( query ) )
		{
			ret = false;
		}
	}

	if ( TArea::SPATIALTESTS_IN_LOCAL_SPACE )
	{
		query.CancelTransformOutput( area );
		query.CancelTransform( area );
	}
	return ret;
}

void CAreaDescription::VPrecomputeObstacleSpawnData( CObstacleSpawnContext& context )
{

}

template< class TQuery >
Bool CAreaDescription::VLocalSpaceSpatialQuery( TQuery& query ) const
{
	if ( IsTerrainArea() )
	{
		return static_cast< const PathLib::CTerrainAreaDescription* >( this )->LocalSpaceSpatialQuery( query );
	}
	else
	{
		return static_cast< const PathLib::CNavmeshAreaDescription* >( this )->LocalSpaceSpatialQuery( query );
	}
}

template < class TQuery >
Bool CAreaDescription::TMultiAreaQuery( TQuery& query ) const
{
	Bool result = true;
	query.SetStartingArea( m_id );
	const CAreaDescription* area = this;
	while ( true )
	{
		// we do test to make special query that 
		switch( area->GetType() )
		{
		case TYPE_TERRAIN:
			result = TMultiAreaQueryInternal( *area->AsTerrainArea(), query.m_query ) && (TQuery::AUTOFAIL || result);
			break;
		case TYPE_INSTANCE_INWORLD:
			result = TMultiAreaQueryInternal( *area->AsNavmeshArea(), query.m_query ) && (TQuery::AUTOFAIL || result);
			break;
		case TYPE_INSTANCE_TRANSFORMED:
			result = TMultiAreaQueryInternal( *area->AsTransformedNavmeshArea(), query.m_query ) && (TQuery::AUTOFAIL || result);
			break;
		default:
			ASSUME( false );
		}
		if ( TQuery::AUTOFAIL && !result )
			return false;

		do 
		{
			AreaId areaId;
			if ( !query.PopArea( areaId, query.m_query.m_basePos ) )
			{
				return result;
			}
			area = m_pathlib->GetAreaDescription( areaId );
			// !area is possible when navmesh based area (that remember its neighbours) got desynchronized somehow
			if ( !area || !area->IsLoaded() )
			{
				return false;
			}
		} while ( !area );
	} 
	
	return result;
}

// instantiate function templates
template					Bool CAreaDescription::TMultiAreaQuery< CCircleQueryData::MultiArea >( CCircleQueryData::MultiArea& query ) const;
template					Bool CAreaDescription::TMultiAreaQuery< CClosestObstacleCircleQueryData::MultiArea >( CClosestObstacleCircleQueryData::MultiArea& query ) const;
template					Bool CAreaDescription::TMultiAreaQuery< CCollectCollisionPointsInCircleQueryData::MultiArea >( CCollectCollisionPointsInCircleQueryData::MultiArea& query ) const;
template					Bool CAreaDescription::TMultiAreaQuery< CCollectGeometryInCirceQueryData::MultiArea >( CCollectGeometryInCirceQueryData::MultiArea& query ) const;
template					Bool CAreaDescription::TMultiAreaQuery< CLineQueryData::MultiArea >( CLineQueryData::MultiArea& query ) const;
template					Bool CAreaDescription::TMultiAreaQuery< CWideLineQueryData::MultiArea >( CWideLineQueryData::MultiArea& query ) const;
template					Bool CAreaDescription::TMultiAreaQuery< CClosestObstacleWideLineQueryData::MultiArea >( CClosestObstacleWideLineQueryData::MultiArea& query ) const;
template					Bool CAreaDescription::TMultiAreaQuery< CClearWideLineInDirectionQueryData::MultiArea >( CClearWideLineInDirectionQueryData::MultiArea& query ) const;
template					Bool CAreaDescription::TMultiAreaQuery< CCustomTestQueryData::MultiArea >( CCustomTestQueryData::MultiArea& query ) const;



void CAreaDescription::PreLoad()
{
	StartProcessing();

	IStreamingItem::PreLoad();
}

void CAreaDescription::Load()
{
	struct F : public ResourceFunctor
	{
		F( CAreaDescription* me )
			: m_me( me ) {}
		Bool Handle( CResPtr& ptr, ENavResType resType ) override
		{
			ptr.SyncLoad( m_me );

			return true;
		}
		CAreaDescription* m_me;
	} f( this );

	IterateAreaResources( f );

	IStreamingItem::Load();
}

void CAreaDescription::PostLoad()
{
	OnPostLoad();

	// run Initialization code in resources
	struct F : public ResourceFunctor 
	{
		F( CAreaDescription* me )
			: m_me( me ) {}
		Bool Handle( CResPtr& ptr, ENavResType resType ) override
		{
			if ( ptr.IsConstructed() )
			{
				CAreaRes* res = ptr.GetRes();
				if ( res )
				{
					res->VOnPostLoad( m_me );
					return true;
				}
			}
			if ( resType == CAreaNavgraphsRes::GetResType() )
			{
				m_me->MarkDirty( DIRTY_GENERATE );
			}
			else if ( resType == CTerrainMap::GetResType() )
			{
				m_me->MarkDirty( DIRTY_SURFACE );
			}

			return true;
		}
		CAreaDescription*	m_me;
	} f( this );
	IterateAreaResources( f );

	IStreamingItem::PostLoad();
}

void CAreaDescription::PostLoadInterconnection()
{
	IStreamingItem::PostLoadInterconnection();

	// we can even access our neighbours as we have pathlib processing locked
	ConnectWithNeighbours();
}

void CAreaDescription::Attach( CStreamingManager* manager )
{
	EndProcessing();

	IStreamingItem::Attach( manager );
}
void CAreaDescription::PreUnload()
{
	StartProcessing();

	IStreamingItem::PreUnload();

	OnPreUnload();
}
void CAreaDescription::Unload()
{
	DetachFromNeighbours();

	// Free handles and unload resources
	struct F : public ResourceFunctor
	{
		CAreaDescription*			m_area;

		F( CAreaDescription* area )
			: m_area( area ) {}

		Bool	Handle( CResPtr& ptr, ENavResType resType ) override
		{
			CAreaRes* res = ptr.GetRes();
			if ( res )
			{
				res->VOnPreUnload( m_area );
			}
			ptr.Release();
			return true;
		}
	} f( this );
	IterateAreaResources( f );

	IStreamingItem::Unload();
}
void CAreaDescription::Detach( CStreamingManager* manager )
{
	IStreamingItem::Detach( manager );

	EndProcessing();
}

void CAreaDescription::OnPostLoad()
{

}
void CAreaDescription::OnPreUnload()
{

}

Bool CAreaDescription::Save( CDirectory* dir, Bool onlyChanged )
{
	struct Functor : public ResourceFunctor
	{
		Functor( CAreaDescription* area, CDirectory* dir, Bool onlyChanged )
			: m_this( area ), m_dir( dir ), m_onlyChanged( onlyChanged ), m_success( true ) {}
		Bool Handle( CResPtr& ptr, ENavResType resType ) override
		{
			// don't save navmeshes from pathlib systems (only manual creation)
			if ( ptr.IsConstructed() )
			{
				if ( m_onlyChanged )
				{
					CAreaRes* res = ptr.GetRes();
					if ( !res || !res->VHasChanged() )
					{
						return true;
					}
				}
				m_success = ptr.Save( m_this, m_dir ) && m_success;
			}

			return true;
		}
		CAreaDescription*	m_this;
		CDirectory*			m_dir;
		Bool				m_onlyChanged;
		Bool				m_success;
	} f( this, dir, onlyChanged );

	if ( dir != m_pathlib->GetCookedDataDirectory() )
	{
		m_areaFlags &= ~FLAG_IsOriginallyCooked;
	}

	IterateAreaResources( f );

	return f.m_success;
}


// Area processing and generation stuff
void CAreaDescription::MarkDirty( Uint32 flags, Float forceGenerationDelay )
{
#ifndef NO_EDITOR_PATHLIB_SUPPORT
	ASSERT( flags );
	EngineTime generationDelay;
	if ( forceGenerationDelay >= 0.f )
	{
		generationDelay = forceGenerationDelay;
	}
	else
	{
		Float delay = 10.f;
		if ( flags & DIRTY_GENERATE )
		{
			delay = 10.f;
		}
		else if ( (flags & DIRTY_CLEAR_NEIGBOURS_CONNECTION) || (flags & DIRTY_CONNECT_WITH_NEIGHBOURS) )
		{
			delay = EngineTime::ZERO;
		}
		else if ( flags & DIRTY_CALCULATE_NEIGHBOURS )
		{
			delay = 0.1f;
		}
		else if ( flags & DIRTY_TASK_SAVE )
		{
			delay = 0.f;
		}
		// some randomization
		//delay *= FRand( 0.9f, 1.f );
		generationDelay = delay;
	}
	m_isDirty |= Uint16( flags );
	EngineTime proposedDelay = generationDelay + GEngine->GetRawEngineTime();
	// TODO: More custom logic here
	if ( m_generationDelay < proposedDelay || m_generationDelay > proposedDelay + 15.f )
	{
		m_generationDelay = proposedDelay;
	}
	if ( m_pathlib && m_pathlib->GetGenerationManager() )
	{
		m_pathlib->GetGenerationManager()->MarkDirty( m_id );
	}
#endif // NO_EDITOR_PATHLIB_SUPPORT
}
#ifndef NO_EDITOR_PATHLIB_SUPPORT
Bool CAreaDescription::ProcessDirtySurface( IGenerationManagerBase::CAsyncTask** outJob )
{
	return false;
}
Bool CAreaDescription::ProcessDirtyCollisionData()
{
	return false;
}
Bool CAreaDescription::RemarkInstances()
{
	return false;
}

Bool CAreaDescription::CorrectNeighbourList()
{
	return false;
}

Bool CAreaDescription::PreGenerateSync()
{
	return true;
}

void CAreaDescription::PostGenerationSyncProcess()
{

}
void CAreaDescription::ClearNeighboursConnection()
{
	CAreaNavgraphs* res = m_graphs.Get();
	if ( !res )
	{
		return;
	}

	auto fun = [] ( CNavGraph* g )
	{
		g->DetachFromNeighbours();
		g->ClearConnectors();
	};
	res->IterateGraphs( fun );
}

Bool CAreaDescription::SyncProcessing( IGenerationManagerBase::CAsyncTask** outJob, Bool forceSynchronous )
{
	Bool processed = false;
	if ( outJob )
	{
		*outJob = NULL;
	}
	Bool canPerformHeavyTasks = (outJob != NULL) || forceSynchronous;

#ifndef NO_EDITOR

	if ( m_isDirty & DIRTY_TASK_CHECK_CONSISTENCY )
	{
		m_isDirty &= ~DIRTY_TASK_CHECK_CONSISTENCY;
		// check area consistency and modify flags
		// TODO

		
		processed = true;
	}
	if ( m_isDirty & DIRTY_TASK_RECALCULATE_OBSTACLES )
	{
		m_isDirty &= ~DIRTY_TASK_RECALCULATE_OBSTACLES;

		m_pathlib->UpdateObstacles( m_bbox );

		processed = true;
	}
	if ( (m_isDirty & DIRTY_TASK_SURFACE) && canPerformHeavyTasks )
	{
		m_isDirty &= ~(DIRTY_TASK_SURFACE | DIRTY_TASK_COLLISIONDATA);

		// check if dirty surface modified collision data
		processed = ProcessDirtySurface( outJob );
		if ( outJob && *outJob )
		{
			return true;
		}
		canPerformHeavyTasks = forceSynchronous;
	}

	if ( m_isDirty & DIRTY_TASK_REMARK_INSTANCES )
	{
		m_isDirty &= ~(DIRTY_TASK_REMARK_INSTANCES);

		RemarkInstances();

		processed = true;
	}

	if ( m_isDirty & DIRTY_TASK_COLLISIONDATA )
	{
		// check terrain collision data
		m_isDirty &= ~DIRTY_TASK_COLLISIONDATA;

		processed = ProcessDirtyCollisionData();
	}

	if ( (m_isDirty & DIRTY_TASK_CLEAR_NEIGHBOURS_CONNECTION) )
	{
		m_isDirty &= ~DIRTY_TASK_CLEAR_NEIGHBOURS_CONNECTION;
		ClearNeighboursConnection();
		
		processed = true;
	}

	if ( m_isDirty & DIRTY_TASK_GENERATE && canPerformHeavyTasks )
	{
		if ( forceSynchronous )
		{
			m_usedCategories = 0xff;
			if ( PreGenerateSync() )
			{
				GenerateSync();
				PostGenerationSyncProcess();
			}
		}
		else
		{
			if ( PreGenerateSync() )
			{
				CAreaProcessingJob* newJob = new CAreaGenerationJob( this, DIRTY_TASK_GENERATE, false, true );
				//CAreaProcessingJob* newJob = new CAreaProcessingJob( this, DIRTY_TASK_GENERATE );
				//newJob->InitThread();
				//newJob->DetachThread();
				*outJob = newJob;
				m_isDirty |= DIRTY_TASK_SAVE | DIRTY_TASK_CONNECT_WITH_NEIGHBOURS;
				canPerformHeavyTasks = false;
			}
		}
		m_isDirty &= ~DIRTY_TASK_GENERATE;
		processed = true;
	}
	
	// do only if we haven't started new generation job or we are doing synchronous test
	if ( canPerformHeavyTasks || (outJob == NULL) )
	{
		if ( m_isDirty & DIRTY_TASK_CALCULATE_NEIGHBOURS )
		{
			m_isDirty &= ~DIRTY_TASK_CALCULATE_NEIGHBOURS;
			if ( CorrectNeighbourList() )
			{
				m_isDirty |= DIRTY_TASK_SAVE;
			}
		}

		if ( m_isDirty & DIRTY_TASK_SAVE )
		{
			m_isDirty &= ~DIRTY_TASK_SAVE;

			{
				// prepare for serialization
				DetachFromNeighbours();
				// save
				CDirectory* outputDir = m_pathlib->GetSaveDirectory();
				if ( !outputDir || !Save( outputDir, true ) )
				{
					PATHLIB_ERROR( TXT("Problem while saving navigation area!\n") );
				}
				else
				{
					m_pathlib->SaveSystemConfiguration();

					if ( IsTerrainArea() )
					{
						Int32 tileX, tileY;
						m_pathlib->GetTerrainInfo().GetTileCoordsFromId( m_id, tileX, tileY );
						PATHLIB_LOG( TXT("Navigation area terrain %d, %d saved!\n"), tileX, tileY );
					}
					else
					{
						PATHLIB_LOG( TXT("Navigation area instance %04x saved!\n"), m_id & CAreaDescription::ID_MASK_INDEX );
					}

				}

				// reattach to world
				ConnectWithNeighbours();

				m_isDirty &= ~DIRTY_TASK_CONNECT_WITH_NEIGHBOURS;
			}
		}

		if ( m_isDirty & DIRTY_TASK_CONNECT_WITH_NEIGHBOURS )
		{
			m_isDirty &= ~DIRTY_TASK_CONNECT_WITH_NEIGHBOURS;

			ConnectWithNeighbours();

			processed = true;
		}
	}
#endif

	return processed;
}
Bool CAreaDescription::AsyncProcessing( CAreaGenerationJob* job )
{
	Bool processed = false;
#ifndef NO_EDITOR
	if ( job->GetGenerationFlags() & DIRTY_TASK_GENERATE )
	{
		// generation
		GenerateAsync( job, 0xff );
				
		processed = true;
	}
#endif
	return processed;
}

void CAreaDescription::DescribeProcessingTasks( String& description, Uint32 flags )
{
	String areaDesc;
	String taskDesc;
	Describe( areaDesc );
	if ( flags & DIRTY_TASK_GENERATE )
	{
		taskDesc = TXT( "generate navgraph" );
	}
	else if ( flags & DIRTY_TASK_SURFACE )
	{
		taskDesc = TXT( "apply surface modification" );
	}
	else if ( flags & DIRTY_TASK_CHECK_CONSISTENCY )
	{
		taskDesc = TXT( "consistency check" );
	}
	else if ( flags & DIRTY_TASK_CLEAR_NEIGHBOURS_CONNECTION )
	{
		taskDesc = TXT( "reset neighbours connection" );
	}
	else if ( flags & DIRTY_TASK_CONNECT_WITH_NEIGHBOURS )
	{
		taskDesc = TXT( "connect with neighbours" );
	}
	else if ( flags & DIRTY_TASK_COLLISIONDATA )
	{
		taskDesc = TXT( "apply new collision data" );
	}
	else if ( flags & DIRTY_TASK_SAVE )
	{
		taskDesc = TXT( "save navdata" );
	}
	else if ( flags & DIRTY_TASK_CALCULATE_NEIGHBOURS )
	{
		taskDesc = TXT( "recalculate connected neighbours" );
	}
	else if ( flags == 0 )
	{
		taskDesc = TXT( "no waiting tasks" );
	}
	else
	{
		taskDesc = TXT( "invalid state" );
	}
	description = areaDesc + TXT(": ") + taskDesc;
}

void CAreaDescription::PreCooking()
{
	m_isDirty = 0;

	// simulate load
	PreLoad();
	NeverUnstream( m_pathlib->GetStreamingManager() );
	PostLoad();
	PostLoadInterconnection();
	Attach( m_pathlib->GetStreamingManager() );
	
}
void CAreaDescription::PostCooking()
{
	m_isDirty = 0;
	m_areaFlags |= FLAG_IsOriginallyCooked;
}

#endif	// NO_EDITOR_PATHLIB_SUPPORT
};		// namespace PathLib
