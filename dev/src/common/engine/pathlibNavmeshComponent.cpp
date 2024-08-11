#include "build.h"
#include "pathlibNavmeshComponent.h"

#include "pathlibNavmesh.h"
#include "pathlibNavmeshLegacy.h"
#include "pathlibWorld.h"
#include "pathlibAreaDescription.h"
#include "pathlibVisualizer.h"
#include "pathlibNavmeshGeneration.h"
#include "../core/taskManager.h"
#include "../core/factory.h"
#include "../core/directory.h"
#include "../core/feedback.h"
#include "viewport.h"
#include "game.h"
#include "renderFrame.h"
#include "worldIterators.h"
#include "world.h"
#include "tickManager.h"
#include "entity.h"


IMPLEMENT_ENGINE_CLASS( CNavmeshComponent );
IMPLEMENT_ENGINE_CLASS( CNavmeshGenerationRootComponent );
IMPLEMENT_ENGINE_CLASS( CNavmeshInputAttachment );
IMPLEMENT_ENGINE_CLASS( SNavmeshParams );


void SNavmeshParams::ResetToDefaults()
{
	SNavmeshParams defaultParams;

	m_extensionLength = defaultParams.m_extensionLength;
	m_cellWidth = defaultParams.m_cellWidth;
	m_cellHeight = defaultParams.m_cellHeight;
	m_walkableSlopeAngle = defaultParams.m_walkableSlopeAngle;
	m_agentHeight = defaultParams.m_agentHeight;
	m_margin =defaultParams.m_margin;
	m_agentClimb = defaultParams.m_agentClimb;
	m_maxEdgeLen = defaultParams.m_maxEdgeLen;
	m_maxEdgeError = defaultParams.m_maxEdgeError;
	m_regionMinSize = defaultParams.m_regionMinSize;
	m_regionMergeSize = defaultParams.m_regionMergeSize;
	m_vertsPerPoly = defaultParams.m_vertsPerPoly;
	m_detailSampleDist = defaultParams.m_detailSampleDist;
	m_detailSampleMaxError = defaultParams.m_detailSampleMaxError;
	m_extraStreamingRange = defaultParams.m_extraStreamingRange;
}


CNavmeshComponent::~CNavmeshComponent()
{
	if ( m_navmesh )
	{
		delete m_navmesh;
		m_navmesh = NULL;
	}
}

//void CNavmeshComponent::OnUpdateBounds()
//{
//	//if ( !m_navmesh )
//	//{
//	//	m_boundingBox = Box( GetWorldPosition(), 0.1f );
//	//	return;
//	//}
//	//Matrix mat;
//
//	//GetLocalToWorld( mat );
//	//const Box& navbox = m_navmesh->GetBoundingBox();
//	//Vector pos = mat.TransformVector( navbox.Min );
//	//m_boundingBox = Box( pos, pos );
//	//pos = mat.TransformVector( Vector( navbox.Min.X, navbox.Max.Y, navbox.Min.Z, 0.f ) );
//	//m_boundingBox.AddPoint( pos );
//	//pos = mat.TransformVector( Vector( navbox.Max.X, navbox.Min.Y, navbox.Max.Z, 0.f ) );
//	//m_boundingBox.AddPoint( pos );
//	//pos = mat.TransformVector( navbox.Max );
//	//m_boundingBox.AddPoint( pos );
//
//	//RecompileBSP();
//}
Bool CNavmeshComponent::IsManualCreationAllowed() const
{
	return true;
}
void CNavmeshComponent::OnAttached( CWorld* world )
{
#ifndef NO_EDITOR_FRAGMENTS
	// Register in editor fragment list
	world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_NavMesh );
#endif

	TBaseClass::OnAttached( world );
}
void CNavmeshComponent::OnDetached( CWorld* world )
{
#ifndef NO_EDITOR_FRAGMENTS
	// Remove form editor fragment list
	world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, SHOW_NavMesh );
#endif

#ifndef NO_NAVMESH_GENERATION
	if ( m_generationJob )
	{
		m_generationJob->Release();
		m_generationJob = nullptr;
		world->GetTickManager()->Remove( this );
	}
#endif

	TBaseClass::OnDetached( world );
}

void CNavmeshComponent::ComputeNavmeshBasedBounds()
{
	if ( m_navmesh )
	{
		CEntity* entity = GetEntity();
		SetScale( Vector( 1.f, 1.f, 1.f, 1.f ) );
		entity->SetScale( Vector( 1.f, 1.f, 1.f, 1.f ) );
		// geather all vertexes and compute z boundings
		Float zMin = FLT_MAX;
		Float zMax = -FLT_MAX;
		TDynArray< Vector2 > pointsInput( m_navmesh->GetVertexesCount() );
		TDynArray< Vector2 > convexHull;
		for ( PathLib::CNavmesh::VertexIndex i = 0, n = m_navmesh->GetVertexesCount(); i < n; ++i )
		{
			const Vector3& v = m_navmesh->GetVertex( i );
			zMin = Min( zMin, v.Z );
			zMax = Max( zMax, v.Z );
			pointsInput[ i ] = v.AsVector2();
		}
		// setup component in 3d space
		zMax += 1.f;
		zMin -= 1.f;
		m_height = zMax - zMin;

		Vector currWorldPosition = GetWorldPosition();
		Float correctZ = zMin - currWorldPosition.Z;

		//if ( m_useAreaBasedGeneration )
		{
			const Vector& startingPos = entity->GetPosition();
			entity->SetPosition( Vector( startingPos.X, startingPos.Y, zMin ) );
			
		}
		//else
		//{
		//	SetPosition( Vector( 0.f, 0.f, zMin ) );
		//}

		// fix internal root points
		for ( Uint32 i = 0, n = m_generationRootPoints.Size(); i != n; ++i )
		{
			m_generationRootPoints[ i ].Z -= correctZ;
		}


		// compute convex hull
		MathUtils::GeometryUtils::ComputeConvexHull2D( pointsInput, convexHull );
		Uint32 convexHullSize = convexHull.Size();
		m_localPoints.Resize( convexHullSize );
		if ( IsNavmeshUsingTransformation() )
		{
			for ( Uint32 i = 0; i < convexHullSize; ++i )
			{
				const Vector2& v = convexHull[ i ];
				m_localPoints[ i ] = Vector( v.X, v.Y, 0.f );
			}
		}
		else
		{

			const Matrix& m = GetWorldToLocal();
			for ( Uint32 i = 0; i < convexHullSize; ++i )
			{
				const Vector2& v = convexHull[ i ];
				Vector2 transformed = m.TransformPoint( Vector( v ) ).AsVector2();
				m_localPoints[ i ] = Vector( transformed.X, transformed.Y, 0.f );
			}
		}
		entity->ForceUpdateTransformNodeAndCommitChanges();
		UpdateWorldPoints();
		ForceUpdateBoundsNode();
	}
}
void CNavmeshComponent::ClearNavmesh()
{
	if ( m_navmesh )
	{
		delete m_navmesh;
		m_navmesh = nullptr;
	}
}
#ifndef NO_EDITOR_FRAGMENTS
Bool CNavmeshComponent::OnPropertyMissing( CName propertyName, const CVariant& readValue )
{
	if( propertyName.AsString() == TXT("navmesh") )
	{
		const IRTTIType* type = readValue.GetRTTIType();
		if ( type->IsPointerType() && static_cast< const IRTTIPointerTypeBase* >( type )->GetPointedType() == ::CNavmesh::GetStaticClass() )
		{
			::CNavmesh* navmesh = static_cast< ::CNavmesh* >( static_cast< const IRTTIPointerTypeBase* >( type )->GetPointer( readValue.GetData() ).GetPointer() );
			if ( navmesh && navmesh->GetFile() )
			{
				const String& fileName = navmesh->GetFile()->GetFileName();
				PathLib::AreaId areaId = CPathLibWorld::GetInstanceAreaIdFromFileName( fileName );
				if ( areaId != PathLib::INVALID_AREA_ID )
				{
					m_pathlibAreaId = areaId;
				}
			}
			return true;
		}
	}

	return TBaseClass::OnPropertyMissing( propertyName, readValue );
}
void CNavmeshComponent::OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag )
{
	TBaseClass::OnGenerateEditorFragments( frame, flag );

	if ( flag == SHOW_Areas )
	{
		if ( IsUsingGenerationRootPoints() )
		{
			Matrix m = Matrix::IDENTITY;
			const Vector& translation = GetWorldPositionRef();
			for ( Uint32 i = 0, n = m_generationRootPoints.Size(); i < n; ++i )
			{
				frame->AddDebugSphere( m_generationRootPoints[ i ] + translation, 0.5f, m, Color( 0, 255, 0 ), false );
			}
		}
	}
	else if ( flag == SHOW_NavMesh )
	{
#ifndef NO_NAVMESH_GENERATION
		if ( m_generationJob )
		{
			Int32 x = frame->GetFrameOverlayInfo().m_width / 2 - 50;
			Int32 y = frame->GetFrameOverlayInfo().m_height / 2 - 25;

			const Char* taskDescription = TXT("");
			switch( m_generationJob->GetCurrentTaskDescription() )
			{
			case Recast::ENavGen_Initialize:
				taskDescription = TXT( "Initialize generation" );
				break;
			case Recast::ENavGen_MarkTriangles:
				taskDescription = TXT( "Mark triangles" );
				break;
			case Recast::ENavGen_ProcessHeightfield:
				taskDescription = TXT( "Heightfield processing" );
				break;
			case Recast::ENavGen_BuildRegions:
				taskDescription = TXT( "Build regions map" );
				break;
			case Recast::ENavGen_BuildContourSet:
				taskDescription = TXT( "Build contour set" );
				break;
			case Recast::ENavGen_BuildPolyMesh:
				taskDescription = TXT( "Build poly mesh" );
				break;
			case Recast::ENavGen_BuildPolyMeshDetail:
				taskDescription = TXT( "Build detailed mesh" );
				break;
			case Recast::ENavGen_ConsistencyProcessing:
				taskDescription = TXT( "Consistance processing" );
				break;
			case Recast::ENavGen_PrepareExport:
				taskDescription = TXT( "Prepare export" );
				break;
			case Recast::ENavGen_ExportGeometry:
				taskDescription = TXT( "Export geometry" );
				break;
			}

			
			// 

			frame->AddDebugScreenFormatedText( x, y, Color::LIGHT_CYAN, TXT("Navmesh generation: %s"), taskDescription );
		}
#endif
		if ( m_navmesh )
		{
			CEntity* entity = GetEntity();
			if ( entity )
			{
				Vector position = m_navmesh->GetBoundingBox().GetMassCenter();
				if ( IsNavmeshUsingTransformation() )
				{
					position = GetLocalToWorld().TransformPoint( position );
				}
				position.Z += 2.5f;
				//String text = String::Printf( TXT("%s file: %s"), entity->GetFriendlyName().AsChar(), m_navmesh->GetFile()->GetName().AsChar() );
				frame->AddDebugText( position, entity->GetFriendlyName(), false, Color( 0, 255, 0 ) );
			}
		}
	}
}
#endif
Bool CNavmeshComponent::Initialize( const TDynArray< Vector3 >& vertexes, const TDynArray< Uint16 >& triangleVertexes, Bool useTransformation /* NOT USED FOR NOW */, Bool save )
{
#ifndef NO_NAVMESH_GENERATION
	if ( m_navmesh )
	{
		m_navmesh->Clear();
	}
	else
	{
		m_navmesh = new PathLib::CNavmesh();
	}

	// initialize navmesh with data
	m_navmesh->InitializeMesh( vertexes, triangleVertexes );

	PostNavmeshInitialization( save );

	return true;
#else
	return false;
#endif
}


#ifndef NO_NAVMESH_GENERATION

CNavmeshComponent::InputIterator* CNavmeshComponent::AreaBasedInputGenerator()
{
	struct AreaInputIterator : public InputIterator
	{
		AreaInputIterator( CNavmeshComponent* component, CWorld* world, Bool rootPoints )
			: m_areaComponent( component )
			, m_it( world )
			, m_rootPoints( rootPoints )
			, m_world( world ) {}
		Bool Reset() override
		{
			m_it = WorldAttachedComponentsIterator( m_world );
			return true;
		}
		void Next() override
		{
			++m_it;
		}
		CNode* Get() override
		{
			for ( ; m_it; ++m_it )
			{
				CBoundedComponent* boundedComponent = Cast< CBoundedComponent >( *m_it );
				if ( boundedComponent )
				{
					CAreaComponent* areaComponent = Cast< CAreaComponent >( boundedComponent );
					if ( areaComponent )
					{
						if ( m_areaComponent->TestIntersection( areaComponent ) )
						{
							return *m_it;
						}
					}
					else
					{
						const Box& areaBBox = m_areaComponent->GetBoundingBox();
						const Box& componentBBox = boundedComponent->GetBoundingBox();

						if ( areaBBox.Touches( componentBBox ) 
							&& MathUtils::GeometryUtils::TestIntersectionPolygonRectangle2D( m_areaComponent->GetWorldPoints(), componentBBox.Min.AsVector2(), componentBBox.Max.AsVector2(), 0.1f ) )
						{
							return *m_it;
						}
					}
				}
				else if ( m_rootPoints )
				{
					CNavmeshGenerationRootComponent* generationRoot = Cast < CNavmeshGenerationRootComponent >( *m_it );
					if ( generationRoot )
					{
						if ( m_areaComponent->TestPointOverlap( generationRoot->GetWorldPosition() ) )
						{
							return *m_it;
						}
					}
				}
			}

			return NULL;

		}
		CNavmeshComponent*					m_areaComponent;
		WorldAttachedComponentsIterator		m_it;
		Bool								m_rootPoints;
		CWorld*								m_world;
	};


	CEntity* entity = GetEntity();
	if ( !entity )
	{
		return NULL;
	}

	CLayer* layer = entity->GetLayer();
	CWorld* world = layer ? layer->GetWorld() : NULL;
	if ( !world )
	{
		return NULL;
	}

	return new AreaInputIterator( this, world, IsUsingGenerationRootPoints() );
}

void CNavmeshComponent::PostNavmeshInitialization( Bool save )
{
	CWorld* world = GetEntity()->GetLayer()->GetWorld();
	CPathLibWorld* pathlib = world ? world->GetPathLibWorld() : nullptr;
	if ( !pathlib )
	{
		return;
	}

	if ( m_pathlibAreaId == PathLib::INVALID_AREA_ID )
	{
		m_pathlibAreaId = pathlib->GetFreeInstanceId();
	}

	if ( save )
	{
		String fileName;
		pathlib->GetGenericFileName( m_pathlibAreaId, fileName, PathLib::CNavmeshRes::GetFileExtension() );

		// save 'source' file
		CDirectory* sourceDir = pathlib->GetSourceDataDirectory();
		CDirectory* localDir = pathlib->GetLocalDataDirectory();

		auto funSaveDir = 
			[ this, pathlib, &fileName ] 	( CDirectory* dir ) -> Bool
		{
			String depoPath;
			dir->GetDepotPath( depoPath );
			String fileName;
			pathlib->GetGenericFileName( m_pathlibAreaId, fileName, PathLib::CNavmeshRes::GetFileExtension() );
			CDiskFile* file = dir->FindLocalFile( fileName );
			if ( !m_navmesh->Save( depoPath + fileName ) )
			{
				PATHLIB_ERROR( TXT("Problem while saving navmesh file '%ls'."), fileName.AsChar() );
				return false;
			}
			if ( !file )
			{
				file = dir->CreateNewFile( fileName );
			}
			return true;
		};
		
		if ( sourceDir )
		{
#ifndef NO_FILE_SOURCE_CONTROL_SUPPORT
			
			Bool failedCheckout = false;
			CDiskFile* file = sourceDir->FindLocalFile( fileName );
			if ( file )
			{
				file->GetStatus();
				if ( !file->IsLocal() && !file->IsCheckedOut())
				{
					if ( file->IsNotSynced() )
					{
						failedCheckout = true;
						GFeedback->ShowMsg( TXT("Source control problem"), TXT("There is new version of '%ls' waiting for you on P4! Sync please."), fileName.AsChar() );
						PATHLIB_ERROR( TXT("Source control problem! There is new version of '%ls' waiting for you on P4! Sync please."), fileName.AsChar() );
					}
					if ( !file->SilentCheckOut( true ) )
					{
						failedCheckout = true;
						GFeedback->ShowMsg( TXT("Source control problem"), TXT("Couldn't check out navmesh file '%ls'."), fileName.AsChar() );
						PATHLIB_ERROR( TXT("Source control problem! Couldn't check out navmesh file '%ls'."), fileName.AsChar() );
					}
				}
			}
			if ( !failedCheckout )
			{
#endif
			if ( !funSaveDir( sourceDir ) )
			{
				PATHLIB_ERROR( TXT("Failed to save navmesh file '%ls'!"), fileName.AsChar() );
			}
#ifndef NO_FILE_SOURCE_CONTROL_SUPPORT
			// add file to source control
			if ( !file )
			{
				file = sourceDir->FindLocalFile( fileName );
			}
			if ( file && !file->IsAdded() )
			{
				if ( !file->Add() )
				{
					PATHLIB_ERROR( TXT("Problem while adding navmesh file '%ls' to vsc system."), fileName.AsChar() );
				}
			}
			}
#endif
		}
		if ( localDir )
		{
			funSaveDir( localDir );
		}
	}

	pathlib->NotifyOfInstanceUpdated( this, true, PathLib::CAreaDescription::DIRTY_GENERATE | PathLib::CAreaDescription::DIRTY_SAVE );
	if ( !pathlib->IsTaskManagerEnabled() )
	{
		if ( m_pathlibAreaId != PathLib::INVALID_AREA_ID )
		{
			pathlib->ForceSynchronousAreaProcessing( m_pathlibAreaId );
		}
	}
}

Bool CNavmeshComponent::SetGenerationJob( Recast::CNavmeshGenerationJob* job )
{
	if ( job == m_generationJob )
	{
		return false;
	}

	Bool wasOnPreviously = false;

	if ( m_generationJob )
	{
		m_generationJob->Release();
		wasOnPreviously = true;
	}

	CLayer* layer = GetLayer();
	CWorld* world = layer ? layer->GetWorld() : nullptr;

	if ( !world )
	{
		if ( job )
		{
			job->Release();
		}
		m_generationJob = nullptr;
		return false;
	}

	m_generationJob = job;


	if ( job )
	{
		if ( !wasOnPreviously )
		{
			world->GetTickManager()->AddToGroup( this, TICK_Main );
		}

		GTaskManager->Issue( *job );
		return true;
	}
	else
	{
		world->GetTickManager()->RemoveFromGroup( this, TICK_Main );
		return false;
	}
}
void CNavmeshComponent::ClearGenerationJob()
{
	SetGenerationJob( nullptr );
}

Bool CNavmeshComponent::InputIterator::Reset()
{
	return false;
}

Bool CNavmeshComponent::GenerateNavmesh( InputIterator* input, Bool transformableNavmesh )
{
	Recast::CGenerationInputData inputData( input, this );

	Recast::CGenerator generator( inputData );
	if ( generator.Initialize( inputData ) && generator.Generate() )
	{
		Initialize( generator.m_vertsOutput, generator.m_trisOutput, transformableNavmesh, true );

		if ( m_navmesh )
		{
			m_navmesh->NoticeProblems( generator.GetProblems() );
		}
		ReportErrors();

		return true;
	}

	generator.ReportErrors();

	return false;
}

void CNavmeshComponent::GenerateNavgraph()
{
	CWorld* world = GetEntity()->GetLayer()->GetWorld();
	if ( world )
	{
		CPathLibWorld* pathlib = world->GetPathLibWorld();
		if ( pathlib )
		{
			pathlib->NotifyOfInstanceUpdated( this, false, PathLib::CAreaDescription::DIRTY_GENERATE );
		}
	}
}

Bool CNavmeshComponent::GenerateNavmeshAsync()
{
	// cannot start new job when previous one is running
	if ( m_generationJob )
	{
		return false;
	}

	InputIterator* input = AreaBasedInputGenerator();
	if ( !input )
	{
		return false;
	}

	Recast::CGenerationInputData inputData( input, this );

	// spawn job (and compute input)
	return SetGenerationJob( Recast::CNavmeshGenerationJob::CreateGenerationJob( inputData ) );
}
Bool CNavmeshComponent::GenerateNavmeshRecursiveAsync()
{
	// cannot start new job when previous one is running
	if ( m_generationJob )
	{
		return false;
	}

	InputIterator* input = AreaBasedInputGenerator();
	if ( !input )
	{
		return false;
	}

	Recast::CGenerationInputData inputData( input, this );

	// spawn job (and compute input)
	return SetGenerationJob( Recast::CNavmeshRecursiveGenerationJob::CreateGenerationJob( inputData ) );

}
void CNavmeshComponent::OnTick( float timeDelta )
{
	if ( !m_generationJob || !m_generationJob->IsFinished() )
	{
		return;
	}

	CWorld* world = GetLayer()->GetWorld();
	CPathLibWorld* pathlib = world ? world->GetPathLibWorld() : NULL;
	if ( pathlib )
	{
		if ( pathlib->IsInstanceProcessing( this ) )
		{
			return;
		}
	}
	

	if ( m_generationJob->IsFinished() && m_generationJob->IsSuccess() )
	{
		if ( m_navmesh )
		{
			delete m_navmesh;
		}
		m_navmesh = m_generationJob->StealOutputNavmesh();
		//if ( m_navmesh )
		//{
		//	m_navmesh->CopyFrom( m_generationJob->GetOutputNavmesh() );
		//}
		//else
		//{
		//	m_navmesh = m_generationJob->StealOutputNavmesh();
		//}
		
		PostNavmeshInitialization( true );

		ReportErrors();
	}

	if ( m_generationJob->IsRunningRecursiveGeneration() )
	{
		// here goes large chunk of code about recursive navmesh generation
		CAreaComponent::TAreaPoints prevPoints = m_localPoints;
		// first recompute navmesh based bounds
		ComputeNavmeshBasedBounds();
		// and check if it changed
		Bool hasNavmeshChanged = false;
		if ( m_localPoints.Size() != prevPoints.Size() )
		{
			hasNavmeshChanged = true;
		}
		else
		{
			for ( Uint32 i = 0, n = m_localPoints.Size(); i < n; ++i )
			{
				if ( (m_localPoints[ i ] - prevPoints[ i ]).SquareMag2() > (0.25f*0.25f) )
				{
					hasNavmeshChanged = true;
					break;
				}
			}
		}
		Recast::CNavmeshRecursiveGenerationJob* prevJob = static_cast< Recast::CNavmeshRecursiveGenerationJob* >( m_generationJob );
		InputIterator* inputIterator = prevJob->StealInputIterator();
		if ( inputIterator->Reset() )
		{
			Recast::CGenerationInputData inputData( inputIterator, this );
			Recast::CNavmeshGenerationJob* newJob;
			if ( hasNavmeshChanged )
			{
				// if navmesh bounds has changed we should run recursive navmesh generation process
				newJob = Recast::CNavmeshRecursiveGenerationJob::CreateGenerationJob( inputData );
			}
			else
			{
				// if navmesh bounds doesn't change - run last generation - this time with default terrain navmesh generation and connection autogeneration process
				newJob = Recast::CNavmeshGenerationJob::CreateGenerationJob( inputData );
			}

			SetGenerationJob( newJob );
			
			return;
		}
		else
		{
			delete inputIterator;
			ASSERT( false );
			ASSUME( false );
		}
		// turn off the system as usual
	}

	ClearGenerationJob();
}
void CNavmeshComponent::ReportErrors()
{
	if ( m_navmesh )
	{
		String msg;
		msg = String::Printf( TXT("Vertexes: %d / %d   Triangles: %d / %d\n"), m_navmesh->GetVertexesCount(), PathLib::CNavmesh::VERTEX_LIMIT, m_navmesh->GetTrianglesCount(), PathLib::CNavmesh::TRIANGLE_LIMIT );

		const auto& problems = m_navmesh->GetProblems();
		if ( !problems.Empty() )
		{
			for ( Uint32 i = 0, n = problems.Size(); i < n; msg += TXT("\n"), ++i )
			{
				const auto& p = problems[ i ];
				if ( p.IsLocationUnspecified() )
				{
					msg += p.m_text;
				}
				else
				{
					msg += String::Printf( TXT("[%.2f,%.2f,%.2f] %s"), p.m_location.X, p.m_location.Y, p.m_location.Z, p.m_text.AsChar() );
				}
			}
			GFeedback->ShowMsg( TXT("Navmesh generation problems!"), msg.AsChar() );
		}
		else
		{
			GFeedback->ShowMsg( TXT("Navmesh generated successfully"), msg.AsChar() );
		}
		
	}
	
}

Bool CNavmeshComponent::IsRunningRecursiveGeneration() const
{
	return m_generationJob && m_generationJob->IsRunningRecursiveGeneration();
}
void CNavmeshComponent::StopRecursiveGeneration()
{
	if ( m_generationJob )
	{
		m_generationJob->StopRecursiveGeneration();
	}
}

#ifndef NO_EDITOR

void CNavmeshComponent::EditorPreDeletion()
{
	CPathLibWorld* pathlib = GetLayer()->GetWorld()->GetPathLibWorld();
	if ( pathlib )
	{
		pathlib->NotifyOfInstanceRemoved( this );
	}
	TBaseClass::EditorPreDeletion();
}

Bool CNavmeshComponent::RemoveOnCookedBuild()
{
	return true;
}

#endif			// !NO_EDITOR

#endif			// !NO_NAVMESH_GENERATION


Vector CNavmeshComponent::GetGenerationRootWorldPosition( Uint32 i ) const
{
	return GetWorldPositionRef() + m_generationRootPoints[ i ];
}

void CNavmeshComponent::GenericFileName( PathLib::AreaId areaId, String& outFilename )
{
	ASSERT( areaId  != PathLib::INVALID_AREA_ID );
	CWorld* world = GGame->GetActiveWorld();
	if ( world )
	{
		CPathLibWorld* pathlib = world->GetPathLibWorld();
		if ( pathlib )
		{
				pathlib->GetGenericFileName( areaId, outFilename, PathLib::CNavmeshRes::GetFileExtension() );
			return;
		}
	}
	outFilename = String::Printf( TXT("instance_%04x.navmesh"), areaId & PathLib::CAreaDescription::ID_MASK_INDEX );
}

Bool CNavmeshInputAttachment::Init( CNode* parent, CNode* child, const AttachmentSpawnInfo* info )
{
	// Initialize base attachment
	if ( !TBaseClass::Init( parent, child, info ) )
	{
		return false;
	}
	return true;
}

const CGUID& CNavmeshComponent::GetGUID4PathLib() const								
{ 
	return GetEntity()->GetGUID(); 
}

