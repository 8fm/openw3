/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "storySceneIncludes.h"
#include "../engine/behaviorGraphInstance.h"
#include "../engine/behaviorGraphStack.h"
#include "../engine/cameraDirector.h"
#include "../../common/core/gatheredResource.h"
#include "movableRepresentationPathAgent.h"

#include "storySceneDirector.h"
#include "storyScenePlayer.h"
#include "storySceneInput.h"
#include "storySceneActions.h"
#include "storySceneAnimationParams.h"
#include "sceneLog.h"
#include "storySceneSystem.h"
#include "communitySystem.h"
#include "storySceneUtils.h"
#include "storySceneNPCTeleport.h"
#include "storySceneCutsceneSection.h"
#include "storySceneCutscene.h"
#include "../engine/camera.h"
#include "../engine/cameraComponent.h"
#include "../engine/mimicComponent.h"
#include "../engine/particleSystem.h"
#include "../engine/dynamicLayer.h"
#include "../engine/particleComponent.h"
#include "../engine/spotLightComponent.h"
#include "../engine/pointLightComponent.h"
#include "gameWorld.h"
#include "actorsManager.h"
#include "../engine/cutsceneInstance.h"
#include "../engine/dimmerComponent.h"

#ifdef DEBUG_SCENES_2
#pragma optimize("",off)
#endif

#define SCENE_TELEPORT_ZONE 20.0f

#if 0
	#define TELEPORT_VERBOSE_LOG SCENE_WARN
#else
	#define TELEPORT_VERBOSE_LOG(...)
#endif

RED_DEFINE_STATIC_NAME( IsAxiied )

// CNPCTeleportPositionFilter

CNPCTeleportPositionFilter::CNPCTeleportPositionFilter()
	: m_refCount( 0 )
{
}

CNPCTeleportPositionFilter::~CNPCTeleportPositionFilter()
{
}

void CNPCTeleportPositionFilter::AddRef()
{
	m_refCount.Increment();
}

void CNPCTeleportPositionFilter::Release()
{
	if ( !m_refCount.Decrement() )
	{
		delete this;
	}
}

// BoxFilter

class BoxFilter : public CNPCTeleportPositionFilter
{
private:
	TDynArray< Box > m_excludedBoxes;

public:
	Bool AcceptPosition( const Vector3& pos ) override
	{
		for ( const Box& excludedBox : m_excludedBoxes )
		{
			if ( excludedBox.Contains2D( pos ) )
			{
				return false;
			}
		}
		return true;
	}

	Bool IsFullyContained( const Box& box ) const override
	{
		for ( const Box& excludedBox : m_excludedBoxes )
		{
			if ( excludedBox.Contains2D( box ) )
			{
				return true;
			}
		}
		return false;
	}

	void ReserveExcludedBoxes( Uint32 count )
	{
		m_excludedBoxes.Reserve( count );
	}

	void AddExcludedBox( const Box& box )
	{
		m_excludedBoxes.PushBack( box );
	}

	Bool TestBox( const Box& box ) const
	{
		for ( const Box& excludedBox : m_excludedBoxes )
		{
			if ( excludedBox.Touches2D( box ) )
			{
				return true;
			}
		}
		return false;
	}
};

// ConvexAreaFilter

class ConvexAreaFilter : public BoxFilter
{
private:
	const TDynArray< Vector >	m_includedConvex;
	const TDynArray< Vector >	m_excludedConvex;

public:
	ConvexAreaFilter( TDynArray< Vector >&& includedConvex, TDynArray< Vector >&& excludedConvex )
		: m_includedConvex( Move( includedConvex ) )
		, m_excludedConvex( Move( excludedConvex ) )
	{}

	Bool AcceptPosition( const Vector3& pos ) override
	{
		if ( !IsPointInsideConvexShape( pos, m_includedConvex ) )
		{
			return false;
		}

		if ( IsPointInsideConvexShape( pos, m_excludedConvex ) )
		{
			return false;
		}

		return BoxFilter::AcceptPosition( pos );
	}

	Bool IsFullyContained( const Box& box ) const override
	{
		if ( MathUtils::GeometryUtils::TestPolygonContainsRectangle2D( m_excludedConvex, reinterpret_cast< const Vector2& >( box.Min ), reinterpret_cast< const Vector2& >( box.Max ) ) )
		{
			return true;
		}

		return BoxFilter::IsFullyContained( box );
	}
};

// CNPCTeleportPositionRequest

CNPCTeleportPositionRequest::CNPCTeleportPositionRequest()
	: m_filter( nullptr )
{
}

CNPCTeleportPositionRequest::~CNPCTeleportPositionRequest()
{
}

void CNPCTeleportPositionRequest::Setup( CNewNPC* npc, CNPCTeleportPositionFilter::Ptr& filter, const Box& box, const Vector3& destinationPosition, Float minDistance )
{
	m_npc = npc;
	m_filter = filter;
	m_npcBox = npc->CalcBoundingBox();
	m_npcBoxExtents = m_npcBox.CalcExtents();
	m_wasInInterior = GCommonGame->IsPositionInInterior( npc->GetWorldPositionRef() );
	m_minDistance = minDistance;

	CPathLibWorld* pathLibWorld = GCommonGame->GetActiveWorld()->GetPathLibWorld();
	CMovingAgentComponent* mac = npc->GetMovingAgentComponent();
	CPathAgent* pathAgent = mac ? mac->GetPathAgent() : nullptr;

	const Float actorRadius = Max( m_npcBoxExtents.X, m_npcBoxExtents.Y );
	const PathLib::CollisionFlags collisionFlags = pathAgent ? pathAgent->GetCollisionFlags() : PathLib::CT_DEFAULT;
	const PathLib::NodeFlags nodeFlags = pathAgent ? pathAgent->GetForbiddenPathfindFlags() : PathLib::NFG_FORBIDDEN_BY_DEFAULT;

	const Vector boxSize = box.CalcSize();
	const Float maxDistance = boxSize.X + boxSize.Y;

	PathLib::CWalkableSpotQueryRequest::Setup( *pathLibWorld, box, destinationPosition, npc->GetWorldPositionRef(), actorRadius, maxDistance, FLAG_BAIL_OUT_ON_SUCCESS, collisionFlags, nodeFlags, 0xf00d );
}

Bool CNPCTeleportPositionRequest::AcceptPosition( const Vector3& pos )
{
	TELEPORT_VERBOSE_LOG( TXT( "CNPCTeleportPositionRequest: AcceptPosition %f %f Distance: %f : NPC pos: %f %f npc: %s" ), pos.X, pos.Y, m_npcBox.CalcCenter().DistanceTo2D( Vector( pos ) ), m_npcBox.CalcCenter().X, m_npcBox.CalcCenter().Y, m_npc.Get()->GetFriendlyName().AsChar() );

	const Float distance = m_npcBox.CalcCenter().DistanceTo2D( pos );

	// Min distance check

	if ( distance < m_minDistance )
	{
		return false;
	}

	// Height check

	const Float stepDistance = 4.0f;
	const Float maxAcceptableHeightDifference = stepDistance + Max( 0.0f, distance - stepDistance ) * 0.5f; // The further away, the more height difference we allow
	const Float heightDifference = Abs( pos.Z - m_npcBox.Min.Z );
	if ( heightDifference > maxAcceptableHeightDifference )
	{
		TELEPORT_VERBOSE_LOG( TXT( "CNPCTeleportPositionRequest: AcceptPosition FAIL: height difference is %f (max acceptable is %f)" ), heightDifference, maxAcceptableHeightDifference );
		return false;
	}

	// Make sure "interiorness" matches

	const Bool isInInterior = GCommonGame->IsPositionInInterior( Vector( pos ) );
	if ( isInInterior != m_wasInInterior )
	{
		TELEPORT_VERBOSE_LOG( TXT( "CNPCTeleportPositionRequest: AcceptPosition FAIL: interiorness mismatch" ) );
		return false;
	}

	// Dispatch to shared filter

	if ( !m_filter->AcceptPosition( pos ) )
	{
		TELEPORT_VERBOSE_LOG( TXT( "CNPCTeleportPositionRequest: AcceptPosition FAIL: convex collision" ) );
		return false;
	}

	TELEPORT_VERBOSE_LOG( TXT( "CNPCTeleportPositionRequest: AcceptPosition SUCCESS" ) );
	return true;
}

// CTeleportNPCsJob

Float CTeleportNPCsJob::TeleportNPCJobTimeout = 1.0f;

CTeleportNPCsJob::CTeleportNPCsJob( CStorySceneDirector* director, CNPCTeleportPositionFilter* filter, const Box& box )
	: m_director( director )
	, m_startTime( GCommonGame->GetEngineTime() )
	, m_filter( filter )
	, m_box( box )
{}

CTeleportNPCsJob::~CTeleportNPCsJob()
{
}

void CTeleportNPCsJob::AddTeleportRequest( CNewNPC* npc, Float minDistance )
{
	CPathLibWorld* pathLibWorld = GCommonGame->GetActiveWorld()->GetPathLibWorld();

	Box box = m_box;

	// If min distance is specified, then teleport into a box between minDistance and minDistance * 2

	if ( minDistance != 0.0f )
	{
		const Vector boxCenter = m_box.CalcCenter();

		box.Min = boxCenter - Vector( minDistance, minDistance, 0.0f ) * 3.0f;
		box.Max = boxCenter + Vector( minDistance, minDistance, 0.0f ) * 3.0f;
	}

	// Generate random position to start search from (to randomize results -> to avoid collisions between teleported NPCs)

	const Vector boxCenter = box.CalcCenter();
	const Float spawnDestinationBoxSize = 5.0f;

	Box spawnDestinationBox;
	spawnDestinationBox.Min = boxCenter - Vector( spawnDestinationBoxSize, spawnDestinationBoxSize, 0.0f );
	spawnDestinationBox.Max = boxCenter + Vector( spawnDestinationBoxSize, spawnDestinationBoxSize, 0.0f );

	Vector3 randomDestinationPosition;
	randomDestinationPosition.X = GEngine->GetRandomNumberGenerator().Get< Float >( spawnDestinationBox.Min.X, spawnDestinationBox.Max.X );
	randomDestinationPosition.Y = GEngine->GetRandomNumberGenerator().Get< Float >( spawnDestinationBox.Min.Y, spawnDestinationBox.Max.Y );
	randomDestinationPosition.Z = GEngine->GetRandomNumberGenerator().Get< Float >( spawnDestinationBox.Min.Z, spawnDestinationBox.Max.Z );

	// Create async request and start it

	CNPCTeleportPositionRequest* request = new CNPCTeleportPositionRequest();
	m_requests.PushBack( request );
	request->Setup( npc, m_filter, box, randomDestinationPosition, minDistance );
	request->Submit( *pathLibWorld );
}

void CTeleportNPCsJob::FinalizeSyncPart()
{
	ASSERT( IsAsyncPartDone() );

	const Float secsGoing = ( Float ) ( GCommonGame->GetEngineTime() - m_startTime );

	// Perform teleport for all NPCs

	CActionPointManager* actionPointManager = GCommonGame->GetSystem< CCommunitySystem >()->GetActionPointManager();

	Uint32 numTeleported = 0;
	Uint32 numTimedOut = 0;
	for ( CNPCTeleportPositionRequest::Ptr& request : m_requests )
	{
		CNewNPC* npc = request->GetNPC();
		if ( !npc )
		{
			SCENE_WARN( TXT( "CTeleportNPCsJob: Failed to teleport NPC away from scene area, reason: NPC was despawned in the meantime" ) );
			continue;
		}

		if ( !request->IsQueryCompleted() )
		{
			++numTimedOut;
			m_director->GetDirectorParent()->HideNonSceneActor( npc );
			SCENE_WARN( TXT( "CTeleportNPCsJob: Failed to teleport NPC '%ls' away from scene area, reason: job timed out after %f secs; NPC will be hidden" ), npc->GetFriendlyName().AsChar(), secsGoing );
			continue;
		}

		// Get the query result

		if ( !request->IsQuerySuccess() )
		{
			m_director->GetDirectorParent()->HideNonSceneActor( npc );
			SCENE_WARN( TXT( "CTeleportNPCsJob: Failed to teleport NPC '%ls' away from scene area, reason: find-walkable-spot query failed; NPC will be hidden" ), npc->GetFriendlyName().AsChar() );
			continue;
		}

		const Vector newPosition = Vector( request->GetComputedPosition() );

		// Check if NPC can be teleported out (is busy)

		const Int32 canTeleportOutOfSceneArea = npc->SignalGameplayEventReturnInt( CNAME( AI_CanTeleportOutOfSceneArea ), 1 );
		if ( !canTeleportOutOfSceneArea )
		{
			m_director->GetDirectorParent()->HideNonSceneActor( npc );
			SCENE_LOG( TXT("CTeleportNPCsJob: Failed to teleport NPC '%ls' away from scene area, reason: NPC returned 0 from SignalGameplayEventReturnInt( AI_CanTeleportOutOfSceneArea ); NPC will be hidden"), npc->GetFriendlyName().AsChar() );
			continue;
		}

		// Teleport NPC

		const Vector oldPosition = npc->GetWorldPosition();
		if ( !npc->Teleport( newPosition, npc->GetWorldRotation() ) )
		{
			m_director->GetDirectorParent()->HideNonSceneActor( npc );
			SCENE_WARN( TXT( "CTeleportNPCsJob: Failed to teleport NPC '%ls' away from scene area, reason: Teleport() function failed; NPC will be hidden" ), npc->GetFriendlyName().AsChar() );
			continue;
		}
		++numTeleported;
		SCENE_LOG( TXT( "CTeleportNPCsJob: Teleported NPC '%ls' away from scene area from (%f %f %f) to (%f %f %f) distance %f." ), npc->GetFriendlyName().AsChar(), oldPosition.X, oldPosition.Y, oldPosition.Z, newPosition.X, newPosition.Y, newPosition.Z, npc->GetWorldPositionRef().DistanceTo2D( newPosition ) );

		npc->ForceUpdateTransformNodeAndCommitChanges();
	}

	SCENE_LOG( TXT("CTeleportNPCsJob: Teleported %d NPCs (%u failed; %u timed out after %f secs)"), numTeleported, m_requests.Size() - numTeleported, numTimedOut, secsGoing );

	m_requests.ClearFast();
	m_filter = nullptr;
}

Bool CTeleportNPCsJob::IsAsyncPartDone() const
{
	if ( m_startTime + TeleportNPCJobTimeout < GCommonGame->GetEngineTime() )
	{
		return true; // Ignore unfinished requests
	}

	for ( const CNPCTeleportPositionRequest::Ptr& request : m_requests )
	{
		if ( !request->IsQueryCompleted() )
		{
			return false;
		}
	}
	return true;
}

void CTeleportNPCsJob::Cancel()
{
	m_requests.ClearFast();
	m_filter = nullptr;
}

// CStorySceneNPCTeleportHelper

CTeleportNPCsJob* CStorySceneNPCTeleportHelper::TeleportNPCsAway( CStorySceneDirector* director, const TDynArray< NPCToTeleport >& npcsToTeleportAway, const Box& box, CNPCTeleportPositionFilter* filter )
{
	CTeleportNPCsJob* job = new CTeleportNPCsJob( director, filter, box );

	CActionPointManager* actionPointManager = GCommonGame->GetSystem< CCommunitySystem >()->GetActionPointManager();

	for ( const NPCToTeleport& npcWrapper : npcsToTeleportAway )
	{
		CNewNPC* npc = npcWrapper.m_npc;

		// Make sure it is OK to teleport it

		const Int32 canTeleportOutOfSceneArea = npc->SignalGameplayEventReturnInt( CNAME( AI_CanTeleportOutOfSceneArea ), 1 );
		if ( !canTeleportOutOfSceneArea )
		{
			SCENE_LOG( TXT("CStorySceneNPCTeleportHelper: NPC '%ls' will be hidden; it won't be teleported away from scene area because NPC returned 0 from SignalGameplayEventReturnInt( AI_CanTeleportOutOfSceneArea )"), npc->GetFriendlyName().AsChar() );
			director->GetDirectorParent()->HideNonSceneActor( npc );
			continue;
		}

		// Add it

		job->AddTeleportRequest( npc, npcWrapper.m_minDistance );
	}

	SCENE_LOG( TXT("CStorySceneNPCTeleportHelper: Asynchronous NPCs teleport job created") );

	return job;
}

CTeleportNPCsJob* CStorySceneNPCTeleportHelper::BlockAreaForNonCutsceneSection( CStorySceneDirector* director, const CStorySceneSection* section )
{
	const CStorySceneDialogsetInstance* dialogsetInstance = director->GetPendingOrCurrentDialogsetInstance();
	if ( !dialogsetInstance )
	{
		return nullptr;
	}

	const EngineTransform currentScenePlacement = director->GetScenePlacement( dialogsetInstance );

	// Figure out scene's bounds (used later to clear area)

	Vector sceneCenter;
	Box sceneBox;
	TDynArray< Vector > sceneConvex;

	SStorySceneDialogsetInstanceCalcBoundDesc desc;
	desc.m_dialogSetInstance = dialogsetInstance;
	desc.m_parentTransform = currentScenePlacement;
	desc.m_safeZone = dialogsetInstance->GetSafePlacementRadius();
	desc.m_includeCameras = dialogsetInstance->AreCamerasUsedForBoundsCalculation();
	if ( !director->GetDirectorParent()->DetermineSceneBounds( desc, sceneCenter, sceneBox, sceneConvex, section ) )
	{
		return nullptr;
	}

	// Create denied area
	{
		// Determine convex again (but now in local space)

		Vector localSpaceSceneCenter;
		Box localSpaceSceneBox;
		TDynArray< Vector > localSpaceSceneConvex;

		desc.m_parentTransform.Identity();
		if ( !director->GetDirectorParent()->DetermineSceneBounds( desc, localSpaceSceneCenter, localSpaceSceneBox, localSpaceSceneConvex, section ) )
		{
			return nullptr;
		}

		// Enlarge denied area by a bit

		const Float extrusionLength = 0.25f;

		for ( Vector& point : localSpaceSceneConvex )
		{
			Vector extrusionVector = point - localSpaceSceneCenter;
			extrusionVector.Normalize3();
			extrusionVector *= extrusionLength;

			point += extrusionVector;
		}

		// Add denied area

		director->GetDirectorParent()->AddDeniedArea( localSpaceSceneConvex, currentScenePlacement );
	}

	// Get all non-scene actors within scene's bounds

	TDynArray< NPCToTeleport > npcsToTeleportAway;
	struct ActorInSceneAreaFunctor : CActorsManager::DefaultFunctor
	{
		ActorInSceneAreaFunctor( TDynArray< NPCToTeleport >& npcsToTeleportAway,
			const TDynArray< CStorySceneController::SceneActorInfo >& sceneActors,
			const TDynArray< Vector >& sceneConvex )
			: m_npcsToTeleportAway( npcsToTeleportAway )
			, m_sceneActors( sceneActors )
			, m_sceneConvex( sceneConvex )
		{}

		RED_FORCE_INLINE Bool operator()( const CActorsManagerMemberData& actorData )
		{
			if ( CNewNPC* npc = Cast< CNewNPC >( actorData.Get() ) )
			{
				// Exclude scene actors

				for ( const auto& sceneActor : m_sceneActors )
				{
					if ( sceneActor.m_actor == npc )
					{
						return true;
					}
				}

				// Test NPC's bounding circle against scene convex

				const Box box = npc->CalcBoundingBox();
				const Vector boxExtents = box.CalcExtents();

				const Vector circleCenter = box.CalcCenter();
				const Float circleRadius = Max( boxExtents.X, boxExtents.Y );
				if ( !MathUtils::GeometryUtils::TestIntersectionPolygonCircle2D( m_sceneConvex, circleCenter, circleRadius ) )
				{
					return true;
				}

				// Non-scene actor intersects with scene -> teleport it

				m_npcsToTeleportAway.PushBack( NPCToTeleport( npc ) );
			}

			return true;
		}

		TDynArray< NPCToTeleport >&									m_npcsToTeleportAway;
		const TDynArray< CStorySceneController::SceneActorInfo >&	m_sceneActors;
		const TDynArray< Vector >&									m_sceneConvex;

	} actorInSceneAreaFunctor( npcsToTeleportAway, director->GetDirectorParent()->GetSceneController()->GetMappedActors(), sceneConvex );
	GCommonGame->GetActorsManager()->TQuery( actorInSceneAreaFunctor, sceneBox, false, nullptr, 0 );

	FindAxiiedNPCs( director, npcsToTeleportAway, sceneCenter );

	FilterOutSceneActors( director, npcsToTeleportAway );

	if ( npcsToTeleportAway.Empty() )
	{
		return nullptr;
	}

	// Determine target teleport area (an extended scene convex)

	Box teleportBox;
	teleportBox.Clear();

	TDynArray< Vector > extendedSceneConvex;
	extendedSceneConvex.Reserve( sceneConvex.Size() );

	for ( auto it = sceneConvex.Begin(), end = sceneConvex.End(); it != end; ++it )
	{
		const Vector extrusionDirection = ( *it - sceneCenter ).Normalized2();
		const Vector extrudedVertex = *it + extrusionDirection * SCENE_TELEPORT_ZONE;
		extendedSceneConvex.PushBack( extrudedVertex );

		teleportBox.AddPoint( extrudedVertex );
	}

	// Prepare filter

	ConvexAreaFilter* filter = new ConvexAreaFilter( Move( extendedSceneConvex ), Move( sceneConvex ) );
	AddNearbyNPCsToFilter( filter, sceneCenter );

	// Teleport found actors out of the dialog's bounds

	return CStorySceneNPCTeleportHelper::TeleportNPCsAway( director, npcsToTeleportAway, teleportBox, filter );
}

CTeleportNPCsJob* CStorySceneNPCTeleportHelper::BlockAreaForCutsceneSection( CStorySceneDirector* director, const CStorySceneCutsceneSection* section )
{
	// Get cutscene boxes and offset

	CCutsceneTemplate* csTemplate = section->GetCsTemplate();
	if ( !csTemplate )
	{
		return nullptr;
	}

	TDynArray< Box > boxes;
	csTemplate->GetBoundingBoxes( boxes );
	if ( boxes.Empty() )
	{
		return nullptr;
	}

	Matrix offset = Matrix::IDENTITY;
	if ( !CStorySceneCutscenePlayer::GetCsPoint( section, offset, director->GetDirectorParent()->GetLayer()->GetWorld() ) )
	{
		return nullptr;
	}

	// Place denied areas

	for ( Uint32 i=0; i<boxes.Size(); ++i )
	{
		const Box& box = boxes[i];

		// Transform box to local space so the point 0,0 is inside ( required by PE )
		const Vector& boxPosition = box.CalcCenter();
		Box localBox = box;
		localBox -= boxPosition;
		Matrix boxLocalToWorld;
		boxLocalToWorld.SetIdentity();
		boxLocalToWorld.SetTranslation( boxPosition );

		// Apply offset
		boxLocalToWorld = boxLocalToWorld * offset;

		TDynArray<Vector> localPoints;
		Vector min = localBox.Min;
		Vector max = localBox.Max;

		localPoints.PushBack( Vector( min.X, min.Y, min.Z ) );
		localPoints.PushBack( Vector( max.X, min.Y, min.Z ) );
		localPoints.PushBack( Vector( max.X, max.Y, min.Z ) );
		localPoints.PushBack( Vector( min.X, max.Y, min.Z ) );

		director->GetDirectorParent()->AddDeniedArea( localPoints, boxLocalToWorld );
	}

	// Construct box enclosing all boxes

	Box cutsceneBox;
	cutsceneBox.Clear();

	BoxFilter* outsideOfCutsceneAreaPositionFilter = new BoxFilter();
	outsideOfCutsceneAreaPositionFilter->ReserveExcludedBoxes( boxes.Size() );
	for ( const Box& box : boxes )
	{
		const Box transformedBox = offset.TransformBox( box );
		outsideOfCutsceneAreaPositionFilter->AddExcludedBox( transformedBox );
		cutsceneBox.AddBox( transformedBox );
	}

	// Get all non-scene actors within scene's bounds

	TDynArray< NPCToTeleport > npcsToTeleportAway;
	struct ActorInSceneAreaFunctor : CActorsManager::DefaultFunctor
	{
		ActorInSceneAreaFunctor( TDynArray< NPCToTeleport >& npcsToTeleportAway,
			const TDynArray< CStorySceneController::SceneActorInfo >& sceneActors,
			const BoxFilter* filter )
			: m_npcsToTeleportAway( npcsToTeleportAway )
			, m_sceneActors( sceneActors )
			, m_filter( filter )
		{}

		RED_FORCE_INLINE Bool operator()( const CActorsManagerMemberData& actorData )
		{
			if ( CNewNPC* npc = Cast< CNewNPC >( actorData.Get() ) )
			{
				for ( const auto& sceneActor : m_sceneActors )
				{
					if ( sceneActor.m_actor == npc )
					{
						return true;
					}
				}

				if ( !m_filter->TestBox( npc->CalcBoundingBox() ) )
				{
					return true;
				}

				m_npcsToTeleportAway.PushBack( NPCToTeleport( npc ) );
			}
			return true;
		}

		TDynArray< NPCToTeleport >&									m_npcsToTeleportAway;
		const TDynArray< CStorySceneController::SceneActorInfo >&	m_sceneActors;
		const BoxFilter*											m_filter;

	} actorInSceneAreaFunctor( npcsToTeleportAway, director->GetDirectorParent()->GetSceneController()->GetMappedActors(), outsideOfCutsceneAreaPositionFilter );
	GCommonGame->GetActorsManager()->TQuery( actorInSceneAreaFunctor, cutsceneBox, false, nullptr, 0 );

	FindAxiiedNPCs( director, npcsToTeleportAway, cutsceneBox.CalcCenter() );

	// Filter out NPCs included in the cutscene template

	for ( Int32 i = npcsToTeleportAway.SizeInt() - 1; i >= 0; i-- )
	{
		CNewNPC* npc = npcsToTeleportAway[i].m_npc;
		const TagList& tags = npc->GetTags();
		if( ( !tags.Empty() || npc->GetVoiceTag() ) && csTemplate->HasActor( npc->GetVoiceTag(), &npc->GetTags() ) )
		{
			npcsToTeleportAway.RemoveAtFast( i );
		}
	}

	FilterOutSceneActors( director, npcsToTeleportAway );

	if ( npcsToTeleportAway.Empty() )
	{
		return nullptr;
	}

	// Prepare filter

	AddNearbyNPCsToFilter( outsideOfCutsceneAreaPositionFilter, cutsceneBox.CalcCenter() );

	// Teleport found actors out of the cutscene's boxes

	Box teleportBox = cutsceneBox;
	teleportBox.Extrude( Vector( SCENE_TELEPORT_ZONE, SCENE_TELEPORT_ZONE, 0.0f ) );

	return TeleportNPCsAway( director, npcsToTeleportAway, teleportBox, outsideOfCutsceneAreaPositionFilter );
}

void CStorySceneNPCTeleportHelper::FindAxiiedNPCs( CStorySceneDirector* director, TDynArray< NPCToTeleport >& npcsToTeleportAway, const Vector& center )
{
	const Float searchDistance = 15.0f;

	struct AxiiedNPCsAroundFunctor : CActorsManager::DefaultFunctor
	{
		AxiiedNPCsAroundFunctor( TDynArray< NPCToTeleport >& npcsToTeleportAway,
			const TDynArray< CStorySceneController::SceneActorInfo >& sceneActors,
			Float searchDistance )
			: m_npcsToTeleportAway( npcsToTeleportAway )
			, m_sceneActors( sceneActors )
			, m_searchDistance( searchDistance )
		{}

		RED_FORCE_INLINE Bool operator()( const CActorsManagerMemberData& actorData )
		{
			CNewNPC* npc = Cast< CNewNPC >( actorData.Get() );
			if ( !npc )
			{
				return true;
			}

			// Skip if scene actor

			for ( const auto& sceneActor : m_sceneActors )
			{
				if ( sceneActor.m_actor == npc )
				{
					return true;
				}
			}

			// Skip if not axiied

			Bool isAxiied = false;
			if ( !CallFunctionRet( npc, CNAME( IsAxiied ), isAxiied ) || !isAxiied )
			{
				return false;
			}

			// Add to teleported NPCs set (or update if found before)

			for ( NPCToTeleport& npcWrapper : m_npcsToTeleportAway )
			{
				if ( npcWrapper.m_npc == npc )
				{
					npcWrapper.m_minDistance = m_searchDistance;
					return true;
				}
			}
			m_npcsToTeleportAway.PushBack( NPCToTeleport( npc, m_searchDistance ) );

			return true;
		}

		TDynArray< NPCToTeleport >&									m_npcsToTeleportAway;
		const TDynArray< CStorySceneController::SceneActorInfo >&	m_sceneActors;
		Float														m_searchDistance;

	} axiiedNPCsAroundFunctor( npcsToTeleportAway, director->GetDirectorParent()->GetSceneController()->GetMappedActors(), searchDistance );

	Box box;
	box.Min = center - Vector( searchDistance, searchDistance, 0.0f );
	box.Max = center + Vector( searchDistance, searchDistance, 0.0f );
	GCommonGame->GetActorsManager()->TQuery( axiiedNPCsAroundFunctor, box, false, nullptr, 0 );
}

void CStorySceneNPCTeleportHelper::FilterOutSceneActors( CStorySceneDirector* director, TDynArray< NPCToTeleport >& npcsToTeleportAway )
{
	for ( const CStorySceneController::SceneActorInfo& sceneActorInfo : director->GetDirectorParent()->GetSceneController()->GetMappedActors() )
	{
		// Exclude actor with matching voicetag

		for ( Int32 i = npcsToTeleportAway.SizeInt() - 1; i >= 0; i-- )
		{
			CNewNPC* npc = npcsToTeleportAway[i].m_npc;

			if ( npc->GetVoiceTag() == sceneActorInfo.m_voicetag )
			{
				npcsToTeleportAway.RemoveAtFast( i );
				break;
			}
		}
	}
}

void CStorySceneNPCTeleportHelper::AddNearbyNPCsToFilter( BoxFilter* filter, const Vector& searchCenter )
{
	struct AddNearbyActorsBoxesFunctor
	{
		enum { SORT_OUTPUT = 0 };

		BoxFilter* m_filter;

		AddNearbyActorsBoxesFunctor( BoxFilter* filter )
			: m_filter( filter )
		{}

		RED_INLINE Bool operator()( const CActorsManagerMemberData& ptr )
		{
			CActor* actor = ptr.Get();
			const Box box = actor->CalcBoundingBox();
			if ( !m_filter->IsFullyContained( box ) )
			{
				m_filter->AddExcludedBox( box );
			}
			return true;
		}

	} addNearbyActorsBoxesFunctor( filter );

	Box searchBox;
	searchBox.Min = searchBox.Max = searchCenter;
	searchBox.Extrude( SCENE_TELEPORT_ZONE );

	GCommonGame->GetActorsManager()->TQuery( addNearbyActorsBoxesFunctor, searchBox, true, nullptr, 0 );
}

CTeleportNPCsJob* CStorySceneNPCTeleportHelper::BlockAreaForSection( CStorySceneDirector* director, const CStorySceneSection* section )
{
	// Log potential problems when some scene actors are missing

	for ( const CStorySceneController::SceneActorInfo& sceneActor : director->GetDirectorParent()->GetSceneController()->GetMappedActors() )
	{
		if ( !sceneActor.m_actor.Get() )
		{
			SCENE_WARN( TXT( "WARN: CStorySceneNPCTeleportHelper: Scene NPC with voicetag '%s' might get teleported away because it hasn't been mapped - should it have 'Don't match by voicetag in dialog' option checked?" ), sceneActor.m_voicetag.AsChar() );
		}
	}

	// Different behavior for regular and cutscene sections

	if ( const CStorySceneCutsceneSection* cutsceneSection = Cast< const CStorySceneCutsceneSection >( section ) )
	{
		return CStorySceneNPCTeleportHelper::BlockAreaForCutsceneSection( director, cutsceneSection );
	}
	else if ( section->UsesSetting() )
	{
		return CStorySceneNPCTeleportHelper::BlockAreaForNonCutsceneSection( director, section );
	}

	return nullptr;
}

#ifdef DEBUG_SCENES_2
#pragma optimize("",on)
#endif
