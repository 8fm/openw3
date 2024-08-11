/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"

#include "explorationFinder.h"
#include "commonGame.h"
#include "gameWorld.h"
#include "expManager.h"
#include "foundExplorationComponent.h"
#include "../core/gatheredResource.h"
#include "../core/loadingJobManager.h"
#include "../engine/mesh.h"
#include "../../common/core/depot.h"
#include "../physics/physicsWorldUtils.h"
#include "../physics/physicsWorld.h"
#include "../engine/viewport.h"
#include "../engine/layerInfo.h"
#include "../engine/staticMeshComponent.h"
#include "../engine/renderFrame.h"
#include "../engine/worldIterators.h"
#include "../engine/jobSpawnEntity.h"

//-----

CGatheredResource resExplorationFinderTemplate( TXT("engine\\templates\\editor\\exploration_finder.w2ent"), 0 );

//-----

CExplorationFinder::CExplorationFinder()
: m_active( false )
, m_visible( false )
, m_job( nullptr )
, m_world( nullptr )
{

}

CExplorationFinder::~CExplorationFinder()
{

}

void CExplorationFinder::BindToWorld( CWorld* world )
{
}

void CExplorationFinder::Shutdown()
{
	FindExplorations( false );
}	

void CExplorationFinder::FindExplorations( Bool find, CLayer* onlyOnLayer )
{
	if ( m_active == find )
	{
		return;
	}
	if ( find )
	{
		Hide( false );
		m_removedValidMarkers = 0;
		m_addedInvalidMarkers = 0;
		m_totalInvalidMarkers = 0;
		m_ignoredMarkers = 0;
		m_active = true;
		m_world = GGame ? GGame->GetActiveWorld() : nullptr;
		m_onlyOnLayer = onlyOnLayer;
		m_job = new CAsyncTask(this);
		m_job->GetReady();
		m_job->InitThread();
		m_job->DetachThread();
	}
	else
	{
		Hide( true );
		m_active = false;
		if ( m_job )
		{
			m_job->TryToBreak();
		}
	}
}

void CExplorationFinder::GenerateEditorFragments( CRenderFrame* frame )
{
	const Int32 x = frame->GetFrameOverlayInfo().m_width - 350;
	const Int32 x2 = x + 150;
	if ( m_active || m_visible )
	{
		Int32 y = 75;
		const Int32 yStep = 15;
		if ( m_active )
		{
			if ( m_job )
			{
				frame->AddDebugScreenFormatedText( x, y, Color::LIGHT_GREEN, TXT("Exploration finder: %.6f%% done..."), Clamp(m_job->GetPercentageDone(), 0.0f, 1.0f) * 100.0f );
			}
			else
			{
				frame->AddDebugScreenFormatedText( x, y, Color::LIGHT_BLUE, TXT("Exploration finder: done") );
			}
		}
		y += yStep / 2;
		y += yStep;
		frame->AddDebugScreenFormatedText( x,  y, Color::YELLOW, TXT("  marker stats") );
		y += yStep;
		frame->AddDebugScreenFormatedText( x,  y, Color::LIGHT_BLUE, TXT("Removed valid:") );
		frame->AddDebugScreenFormatedText( x2, y, Color::LIGHT_BLUE, TXT("%i"), m_removedValidMarkers );
		y += yStep;
		frame->AddDebugScreenFormatedText( x,  y, Color::LIGHT_RED, TXT("Added invalid:") );
		frame->AddDebugScreenFormatedText( x2, y, Color::LIGHT_RED, TXT("%i"), m_addedInvalidMarkers );
		y += yStep;
		frame->AddDebugScreenFormatedText( x,  y, Color::RED, TXT("Total invalid:") );
		frame->AddDebugScreenFormatedText( x2, y, Color::RED, TXT("%i"), m_totalInvalidMarkers );
		y += yStep;
		frame->AddDebugScreenFormatedText( x,  y, Color::GRAY, TXT("Ignored:") );
		frame->AddDebugScreenFormatedText( x2, y, Color::GRAY, TXT("%i"), m_ignoredMarkers );
	}
}

Bool CExplorationFinder::IsFoundExploration( const CEntity * entity, Bool * outIgnored )
{
	const TDynArray< CComponent* >& components = entity->GetComponents();
	{
		for ( auto iComponent = components.Begin(); iComponent != components.End(); ++ iComponent )
		{
			if ( const CFoundExplorationComponent * fec = Cast< CFoundExplorationComponent >( *iComponent ) )
			{
				if ( outIgnored )
				{
					*outIgnored = fec->ShouldBeIgnored();
				}
				return true;
			}
		}
	}
	return false;
}

void CExplorationFinder::ToggleIgnoredTo( const CEntity * entity, Bool ignored )
{
	const TDynArray< CComponent* >& components = entity->GetComponents();
	{
		for ( auto iComponent = components.Begin(); iComponent != components.End(); ++ iComponent )
		{
			if ( CFoundExplorationComponent * fec = Cast< CFoundExplorationComponent >( *iComponent ) )
			{
				if ( ignored )
				{
					++ SExplorationFinder::GetInstance().m_ignoredMarkers;
					-- SExplorationFinder::GetInstance().m_totalInvalidMarkers;
				}
				else
				{
					-- SExplorationFinder::GetInstance().m_ignoredMarkers;
					++ SExplorationFinder::GetInstance().m_totalInvalidMarkers;
				}
				fec->ToggleIgnored( ignored );
			}
		}
	}
}

CExplorationFinder::CAsyncTask::CAsyncTask( CExplorationFinder* finder, const AnsiChar* threadName )
:	Red::Threads::CThread( threadName )
,	m_finder( finder )
,	m_percentageDone( 0.0f )
,	m_tryToBreak( false )
{
	m_markerResource = resExplorationFinderTemplate.LoadAndGet< CEntityTemplate >();
	m_markerResourceTemplate = resExplorationFinderTemplate.LoadAndGet< CEntityTemplate >();
}

CExplorationFinder::CAsyncTask::~CAsyncTask()
{
	if ( m_finder &&
		 m_finder->m_job == this )
	{
		m_finder->m_job = nullptr;
	}
}

void CExplorationFinder::CAsyncTask::GetReady()
{
	m_markersAt.ClearFast();
	m_spawnEntityJobs.ClearFast();
	Vector createTempEntityAt = Vector::ZEROS;
	for ( WorldAttachedLayerIterator iLayer( m_finder->m_world ); iLayer; ++ iLayer )
	{
		CLayer* layer = *iLayer;
		if ( m_finder->m_onlyOnLayer && m_finder->m_onlyOnLayer != layer )
		{
			// just get stats
			for ( Int32 entityIdx = 0; entityIdx < layer->GetEntities().SizeInt(); ++ entityIdx )
			{
				CEntity * entity = layer->GetEntities()[entityIdx];
				Bool shouldBeIgnored = false;
				if ( IsFoundExploration( entity, &shouldBeIgnored ) )
				{
					Vector location;
					EulerAngles orientation;
					if ( shouldBeIgnored )
					{
						++ m_finder->m_ignoredMarkers;
					}
					else
					{
						++ m_finder->m_totalInvalidMarkers;
					}
				}
			}
			continue;
		}
		if ( ! layer->GetLayerInfo() )
		{
			continue;
		}
		layer->GetLayerInfo()->RequestLoad();
		while (! layer->GetLayerInfo()->IsLoaded() || layer->GetLayerInfo()->IsLoading())
		{
			// wait for all to be loaded
			Red::Threads::SleepOnCurrentThread( 100 );
		}
		if ( layer->GetEntities().SizeInt() )
		{
			layer->MarkModified();
		}
		for ( Int32 entityIdx = 0; entityIdx < layer->GetEntities().SizeInt(); ++ entityIdx )
		{
			CEntity * entity = layer->GetEntities()[entityIdx];
			createTempEntityAt = entity->GetWorldPosition();
			const TDynArray< CComponent* >& components = entity->GetComponents();
			{
				for ( auto iComponent = components.Begin(); iComponent != components.End(); ++ iComponent )
				{
					if ( CStaticMeshComponent * sm = Cast< CStaticMeshComponent >( *iComponent ) )
					{
						entity->ForceUpdateTransformNodeAndCommitChanges();
						entity->ForceUpdateBoundsNode();
					}
				}
			}
		}
	}
}

void CExplorationFinder::CAsyncTask::ThreadFunc()
{
	// wait a little bit
	Red::Threads::SleepOnCurrentThread( 100 );
	if ( m_finder->m_world )
	{
		Int32 layerCount = 0;
		for ( WorldAttachedLayerIterator iLayer( m_finder->m_world ); iLayer; ++ iLayer )
		{
			if ( m_finder->m_onlyOnLayer && m_finder->m_onlyOnLayer != *iLayer )
			{
				continue;
			}
			++ layerCount;
		}
		Float percentageStep = 1.0f / (Float)(layerCount);
		Int32 layerIdx = 0;
		for ( WorldAttachedLayerIterator iLayer( m_finder->m_world ); iLayer; ++ iLayer )
		{
			if ( m_finder->m_onlyOnLayer && m_finder->m_onlyOnLayer != *iLayer )
			{
				continue;
			}
			m_percentageDone = percentageStep * (Float)(layerIdx);
			RunFor( *iLayer, percentageStep );
			if ( m_tryToBreak )
			{
				break;
			}
			++ layerIdx;
		}
	}

	delete this;
}

void CExplorationFinder::CAsyncTask::RunFor( CLayer* layer, Float percentageWholeStep )
{
	m_finder->m_world->GetPhysicsWorld( m_params.m_physicsWorld );

	Float percentageStart = m_percentageDone;
	Float percentageStep = percentageWholeStep / (Float)(layer->GetEntities().Size());
	
	// remove previous exploration finders
	for ( Int32 entityIdx = 0; entityIdx < layer->GetEntities().SizeInt(); ++ entityIdx )
	{
		CEntity * entity = layer->GetEntities()[entityIdx];
		Bool shouldBeIgnored = false;
		if ( IsFoundExploration( entity, &shouldBeIgnored ) )
		{
			Vector location;
			EulerAngles orientation;
			if ( ! CheckIfNeededAt( entity->GetPosition(), location, orientation ) )
			{
				entity->Destroy();
				-- entityIdx;
				++ m_finder->m_removedValidMarkers;
			}
			else if ( shouldBeIgnored )
			{
				++ m_finder->m_ignoredMarkers;
			}
			else
			{
				++ m_finder->m_totalInvalidMarkers;
			}
		}
	}

	for ( Int32 entityIdx = 0; entityIdx < layer->GetEntities().SizeInt(); ++ entityIdx )
	{
		// check edges of entity
		const CEntity * entity = layer->GetEntities()[entityIdx];

		Bool hasCollidableStaticMesh = false;

		const TDynArray< CComponent* >& components = entity->GetComponents();
		{
			for ( auto iComponent = components.Begin(); iComponent != components.End(); ++ iComponent )
			{
				if ( const CStaticMeshComponent * sm = Cast< CStaticMeshComponent >( *iComponent ) )
				{
					if ( auto mesh = sm->GetMeshNow() )
					{
						if ( mesh->HasCollision() )
						{
							hasCollidableStaticMesh = true;
							break;
						}
					}
				}
			}
		}

		if ( hasCollidableStaticMesh )
		{
			// get bounding box of entity
			Box bbox = entity->CalcBoundingBox();

			m_percentageDone = percentageStart + percentageStep * (Float)(entityIdx-1);
			Scan( layer, bbox, percentageStep );
		}

		// link all spawned entities so far
		while (LinkSpawnedEntity()) {}

		m_percentageDone = percentageStart + percentageStep * (Float)(entityIdx);
		if ( m_tryToBreak )
		{
			break;
		}
	}


	// link created entities or wait for them to create all
	while ( m_spawnEntityJobs.Size() )
	{
		if ( ! LinkSpawnedEntity() )
		{
			Red::Threads::SleepOnCurrentThread( 10 );
		}
	}

}

Bool CExplorationFinder::CAsyncTask::LinkSpawnedEntity()
{
	if ( m_spawnEntityJobs.Size() == 0 )
	{
		// nothing to link
		return false;
	}
	CJobSpawnEntity* job = m_spawnEntityJobs[0];
	if ( job->HasEnded() )
	{
		job->LinkEntities();
		job->Release();
		m_spawnEntityJobs.RemoveAt(0);
		// we have linked, get another one!
		return true;
	}
	else
	{
		// we didn't link, maybe we should wait a little?
		return false;
	}
}

void CExplorationFinder::CAsyncTask::Scan( CLayer* layer, Box const & bbox, Float percentageWholeStep )
{
	Float percentageStart = m_percentageDone;

	//CPhysicsEngine::CollisionMask include = GPhysicEngine->GetCollisionTypeBit( CNAME( Static ) ) | GPhysicEngine->GetCollisionTypeBit( CNAME( Terrain ) );
	//CPhysicsEngine::CollisionMask exclude = GPhysicEngine->GetCollisionTypeBit( CNAME( Debris ) );

	Float zWholeStep = percentageWholeStep; 
	Float zStep = zWholeStep / Max( 0.01f, bbox.Max.Z - bbox.Min.Z );
	Float xStep = m_params.m_stepZ * zStep / Max( 0.01f, bbox.Max.X - bbox.Min.X );
	Float yStep = m_params.m_stepXY * xStep / Max( 0.01f, bbox.Max.Y - bbox.Min.Y );
	for ( Float z = bbox.Min.Z; z <= bbox.Max.Z; z = z < bbox.Max.Z? Min( bbox.Max.Z, z + m_params.m_stepZ ) : z + m_params.m_stepZ )
	{
		for ( Float x = bbox.Min.X; x <= bbox.Max.X; x = x < bbox.Max.X? Min( bbox.Max.X, x + m_params.m_stepXY ) : x + m_params.m_stepXY )
		{
			for ( Float y = bbox.Min.Y; y <= bbox.Max.Y; y = y < bbox.Max.Y ? Min( bbox.Max.Y , y + m_params.m_stepXY ) : y + m_params.m_stepXY )
			{
				Vector refWS = Vector( x, y, z );

				// check if too close
				if ( CheckIfTooClose( layer, refWS, m_params.m_minSeparation, m_params.m_minHeightSeparation ) )
				{
					continue;
				}

				Vector location;
				EulerAngles orientation;
				if ( CheckIfNeededAt( refWS, location, orientation ) )
				{
					if ( CheckIfTooClose( layer, location, m_params.m_minSeparation, m_params.m_minHeightSeparation ) )
					{
						continue;
					}
					if ( m_markerResource )
					{
						// Spawn entity
						EntitySpawnInfo sinfo;
						sinfo.m_spawnPosition = location;
						sinfo.m_spawnRotation = orientation;
						sinfo.m_resource = m_markerResource;
						sinfo.m_template = m_markerResourceTemplate;
						sinfo.m_detachTemplate = true;
						// spawn entity on other thread - async
						CJobSpawnEntity* job = new CJobSpawnEntity( layer, Move( sinfo ) );
						SJobManager::GetInstance().Issue( job );
						m_spawnEntityJobs.PushBack( job );
						m_markersAt.PushBack(location);
						++ m_finder->m_addedInvalidMarkers;
						++ m_finder->m_totalInvalidMarkers;
					}
				}

				m_percentageDone = percentageStart + ( x - bbox.Min.X ) * xStep
												   + ( y - bbox.Min.Y ) * yStep
												   + ( z - bbox.Min.Z ) * zStep;
				if ( m_tryToBreak )
				{
					return;
				}
			}
		}

		while (LinkSpawnedEntity()) {}
	}
}

Bool CExplorationFinder::CAsyncTask::CheckIfNeededAt( Vector const & refWS, Vector & outLocation, EulerAngles & outOrientation )
{
	Vector startWS = refWS + Vector( 0.0f, 0.0f, m_params.m_height * 0.5f );
	Vector endWS = refWS + Vector( 0.0f, 0.0f, -m_params.m_height * 0.5f );
	SPhysicsContactInfo hitContactInfo;
	Bool needed = false;
	if( m_params.m_physicsWorld->RayCastWithSingleResult( startWS, endWS, m_params.m_include, m_params.m_exclude, hitContactInfo ) == TRV_Hit )
	{
		Vector startWS = refWS + Vector( 0.0f, 0.0f, m_params.m_height );
		Vector endWS = Vector( refWS.X, refWS.Y, hitContactInfo.m_position.Z - 1.0f );
		SPhysicsContactInfo normalCheckContactInfo;
		if( m_params.m_physicsWorld->RayCastWithSingleResult( startWS, endWS, m_params.m_include, m_params.m_exclude, normalCheckContactInfo ) == TRV_Hit )
		{
			if ( normalCheckContactInfo.m_normal.Z > 0.95f ) // should be flat
			{
				// check if it isn't on the edge
				Float offset = m_params.m_radius * 0.7f;
				SPhysicsContactInfo tempContactInfo[4];
				if( (m_params.m_physicsWorld->RayCastWithSingleResult( startWS + Vector( offset,0.0f,0.0f), endWS + Vector( offset,0.0f,0.0f), m_params.m_include, m_params.m_exclude, tempContactInfo[0] ) == TRV_Hit) &&
					(m_params.m_physicsWorld->RayCastWithSingleResult( startWS + Vector(-offset,0.0f,0.0f), endWS + Vector(-offset,0.0f,0.0f), m_params.m_include, m_params.m_exclude, tempContactInfo[1] ) == TRV_Hit) &&
					(m_params.m_physicsWorld->RayCastWithSingleResult( startWS + Vector(0.0f, offset,0.0f), endWS + Vector(0.0f, offset,0.0f), m_params.m_include, m_params.m_exclude, tempContactInfo[2] ) == TRV_Hit) &&
					(m_params.m_physicsWorld->RayCastWithSingleResult( startWS + Vector(0.0f,-offset,0.0f), endWS + Vector(0.0f,-offset,0.0f), m_params.m_include, m_params.m_exclude, tempContactInfo[3] ) == TRV_Hit ))
				{
					const Float maxDiffZ = 0.1f;
					if ( Abs( tempContactInfo[0].m_position.Z - hitContactInfo.m_position.Z ) < maxDiffZ &&
						 Abs( tempContactInfo[1].m_position.Z - hitContactInfo.m_position.Z ) < maxDiffZ &&
						 Abs( tempContactInfo[2].m_position.Z - hitContactInfo.m_position.Z ) < maxDiffZ &&
						 Abs( tempContactInfo[3].m_position.Z - hitContactInfo.m_position.Z ) < maxDiffZ )
					{
						Vector startWS = hitContactInfo.m_position + Vector( 0.0f, 0.0f, m_params.m_radius + 0.05f);
						Vector endWS = hitContactInfo.m_position + Vector( 0.0f, 0.0f, m_params.m_height - m_params.m_radius );
						SPhysicsContactInfo clearContactInfo;
						// check if there is enough place
						if( ( m_params.m_physicsWorld->SweepTestWithSingleResult( startWS, endWS, m_params.m_radius, m_params.m_include, m_params.m_exclude, clearContactInfo ) != TRV_Hit ))
						{
							// found place that hits something and then goes down
							Int32 testOffsets = 8;
							Float angleStep = 360.0f / Float(testOffsets);
							Bool foundDrop = false;
							Float dropAngle = 0.0f;
							// find from which side can we climb
							for ( Float dropOffsetDist = m_params.m_dropOffsetDistFar; dropOffsetDist >= m_params.m_dropOffsetDistClose; dropOffsetDist -= (m_params.m_dropOffsetDistFar - m_params.m_dropOffsetDistClose) / Float(3) )
							{
								Float angle = 0.0f;
								for ( Int32 offIdx = 0; offIdx < testOffsets; ++ offIdx, angle += angleStep )
								{
									Vector withOffset = hitContactInfo.m_position + EulerAngles(0.0f, 0.0f, angle).TransformVector( Vector( 0.0f, dropOffsetDist, 0.0f ) );
									Vector withOffsetCloser = hitContactInfo.m_position + EulerAngles(0.0f, 0.0f, angle).TransformVector( Vector( 0.0f, dropOffsetDist, 0.0f ) ) * 0.5f;
									Vector startWS = withOffset + Vector( 0.0f, 0.0f, m_params.m_height );
									Vector endWS = withOffset + Vector( 0.0f, 0.0f, -m_params.m_dropDepth );
									Vector startCloserWS = withOffsetCloser + Vector( 0.0f, 0.0f, m_params.m_height );
									Vector endCloserWS = withOffsetCloser + Vector( 0.0f, 0.0f, -m_params.m_dropDepth );
									// check if there isn't anything on the way
									if( m_params.m_physicsWorld->SweepTestAnyResult( hitContactInfo.m_position + Vector(0.0f, 0.0f, 0.3f), withOffset + Vector(0.0f, 0.0f, 0.3f), 0.2f, m_params.m_include, m_params.m_exclude ) != TRV_Hit )
									{
										// check if there isn't anything preventing us from dropping (check two distances to eliminate stairs
										if( ( m_params.m_physicsWorld->SweepTestAnyResult( startWS, endWS, 0.1f, m_params.m_include, m_params.m_exclude ) != TRV_Hit ) &&
											( m_params.m_physicsWorld->SweepTestAnyResult( startCloserWS, endCloserWS, 0.1f, m_params.m_include, m_params.m_exclude ) != TRV_Hit ))
										{
											// check sweep test with radius adjustment
											startWS.Z -= m_params.m_radius;
											endWS.Z += m_params.m_radius;
											if( ( m_params.m_physicsWorld->SweepTestAnyResult( startWS, endWS, m_params.m_radius, m_params.m_include, m_params.m_exclude ) != TRV_Hit ))
											{
												foundDrop = true;
												dropAngle = angle;
												break;
											}
										}
									}
								}
								if ( foundDrop )
								{
									break;
								}
							}
							if ( foundDrop )
							{
								outLocation = hitContactInfo.m_position;
								outOrientation = EulerAngles( 0.0f, 0.0f, dropAngle );
								needed = true;
							}
						}
					}
				}
			}
		}
	}
	if ( needed )
	{
		//static Bool checkExps = false;
		//if ( checkExps )
		if ( GCommonGame && GCommonGame->GetActiveWorld() )
		{
			CGameWorld* world = GCommonGame->GetActiveWorld();
			if ( world && world->GetExpManager() )
			{
				IExplorationList explorations;
				world->GetExpManager()->FindNN( outLocation, nullptr, explorations);
				for ( auto iExploration = explorations.Begin(); iExploration != explorations.End(); ++ iExploration )
				{
					const IExploration* exploration = *iExploration;
					if ( exploration->GetId() != ET_Ladder )
					{
						Vector pointA, pointB;
						exploration->GetEdgeWS(pointA, pointB);
					
						if ( outLocation.DistanceToEdge2D( pointA, pointB ) < m_params.m_minExpSeparation &&
							Abs( pointA.Z - outLocation.Z ) < m_params.m_minExpHeightSeparation )
						{
							needed = false;
							break;
						}
					}
				}
			}
		}
	}
	return needed;
}

Bool CExplorationFinder::CAsyncTask::CheckIfTooClose( CLayer* layer, Vector const & refWS, Float minSeparation, Float minHeightSeparation )
{
	Bool tooCloseToOtherMarkers = false;
	for ( auto iLoc = m_markersAt.Begin(); iLoc != m_markersAt.End(); ++ iLoc )
	{
		Vector ewp = *iLoc;
		if ( ( refWS - ewp ).SquareMag2() < minSeparation * minSeparation &&
			Abs( refWS.Z - ewp.Z ) < minHeightSeparation )
		{
			tooCloseToOtherMarkers = true;
			break;
		}
	}
	if ( ! tooCloseToOtherMarkers )
	{
		for ( auto iEntity = layer->GetEntities().Begin(); iEntity != layer->GetEntities().End(); ++ iEntity )
		{
			const CEntity * entity = *iEntity;
			if ( IsFoundExploration( entity ) )
			{
				/*
				Vector ewp = entity->GetPosition();
				Float dist = ( refWS - ewp ).Mag2();
				Float diffZ = refWS.Z - entity->GetPosition().Z;
				*/
				if ( ( refWS - entity->GetPosition() ).SquareMag2() < minSeparation * minSeparation &&
					Abs( refWS.Z - entity->GetPosition().Z ) < minHeightSeparation )
				{
					tooCloseToOtherMarkers = true;
					break;
				}
			}
		}
	}
	return tooCloseToOtherMarkers;
}

CExplorationFinder::SFinderParams::SFinderParams()
{
	m_radius = 0.4f;
	m_height = 1.8f;
	
	m_stepXY = 0.2f;
	m_stepZ = m_height * 0.5f;
	m_dropOffsetDistClose = 0.7f; // distance to drop (to check if we will climb to this point)
	m_dropOffsetDistFar = 1.7f; // distance to drop (to check if we will climb to this point)
	m_dropDepth = 0.7f; // how deep we should drop to check if we need to climb
	
	m_minSeparation = 5.0f; // no need to get more often
	m_minHeightSeparation = m_height * 0.3f;

	//test
	//m_minSeparation = 0.03f;
	//m_minHeightSeparation = m_height * 0.03f;

	// to exploration
	m_minExpSeparation = 2.0f;
	m_minExpHeightSeparation = 0.3f;

	m_physicsWorld = nullptr;

	m_include = GPhysicEngine->GetCollisionTypeBit( CNAME( Static ) ) | GPhysicEngine->GetCollisionTypeBit( CNAME( Terrain ) );
	m_exclude = GPhysicEngine->GetCollisionTypeBit( CNAME( Debris ) );
}

CEntity* CExplorationFinder::FindPrev( CWorld* world, CEntity* startAt, Bool includeIgnored )
{
	CEntity* prev = nullptr;
	for ( WorldAttachedLayerIterator iLayer( world ); iLayer; ++ iLayer )
	{
		CLayer* layer = *iLayer;
		for ( auto iEntity = layer->GetEntities().Begin(); iEntity != layer->GetEntities().End(); ++ iEntity )
		{
			CEntity* entity = *iEntity;
			Bool shouldBeIgnored = false;
			if ( ! IsFoundExploration( entity, &shouldBeIgnored ) )
			{
				continue;
			}
			if ( shouldBeIgnored && ! includeIgnored )
			{
				continue;
			}
			if ( entity == startAt )
			{
				if ( prev )
				{
					return prev;
				}
			}
			prev = entity;
		}
	}
	return prev;
}

CEntity* CExplorationFinder::FindNext( CWorld* world, CEntity* startAt, Bool includeIgnored )
{
	CEntity* first = nullptr;
	Bool encounteredStarting = false;
	for ( WorldAttachedLayerIterator iLayer( world ); iLayer; ++ iLayer )
	{
		CLayer* layer = *iLayer;
		for ( auto iEntity = layer->GetEntities().Begin(); iEntity != layer->GetEntities().End(); ++ iEntity )
		{
			CEntity* entity = *iEntity;
			Bool shouldBeIgnored = false;
			if ( ! IsFoundExploration( entity, &shouldBeIgnored ) )
			{
				continue;
			}
			if ( shouldBeIgnored && ! includeIgnored )
			{
				continue;
			}
			if ( encounteredStarting )
			{
				return entity;
			}
			if ( ! first )
			{
				first = entity;
			}
			if ( entity == startAt )
			{
				encounteredStarting = true;
			}
		}
	}
	return first;
}

void CExplorationFinder::RemoveAll()
{
	m_world = GGame ? GGame->GetActiveWorld() : nullptr;
	if ( m_world && ! m_job )
	{
		for ( WorldAttachedLayerIterator iLayer( m_world ); iLayer; ++ iLayer )
		{
			CLayer* layer = *iLayer;
			for ( Int32 entityIdx = 0; entityIdx < layer->GetEntities().SizeInt(); ++ entityIdx )
			{
				CEntity * entity = layer->GetEntities()[entityIdx];
				if ( IsFoundExploration( entity ) )
				{
					entity->Destroy();
					-- entityIdx;
				}
			}
		}
		UpdateCounters();
	}
}

void CExplorationFinder::UpdateCounters()
{
	m_removedValidMarkers = 0;
	m_addedInvalidMarkers = 0;
	m_totalInvalidMarkers = 0;
	m_ignoredMarkers = 0;

	m_world = GGame ? GGame->GetActiveWorld() : nullptr;
	if ( m_world )
	{
		for ( WorldAttachedLayerIterator iLayer( m_world ); iLayer; ++ iLayer )
		{
			CLayer* layer = *iLayer;
			for ( Int32 entityIdx = 0; entityIdx < layer->GetEntities().SizeInt(); ++ entityIdx )
			{
				CEntity * entity = layer->GetEntities()[entityIdx];
				Bool shouldBeIgnored = false;
				if ( IsFoundExploration( entity, &shouldBeIgnored ) )
				{
					if ( shouldBeIgnored )
					{
						++ m_ignoredMarkers;
					}
					else
					{
						++ m_totalInvalidMarkers;
					}
				}
			}
		}
	}
}

void CExplorationFinder::Hide(Bool hide)
{
	m_visible = ! hide;
	if ( m_visible && ! m_active )
	{
		UpdateCounters();
	}
}
