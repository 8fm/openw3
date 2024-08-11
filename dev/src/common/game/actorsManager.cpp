#include "build.h"
#include "actorsManager.h"

#include "../engine/pathlibWorld.h"
#include "../engine/physicsCharacterWrapper.h"

#include "actorsManagerFunctors.h"
#include "encounter.h"
#include "strayActorManager.h"
#include "movingPhysicalAgentComponent.h"
#include "movableRepresentationPhysicalCharacter.h"

#if defined( USE_UMBRA )
#include "../engine/umbraScene.h"
#endif

using namespace ActorsManagerFunctors;


void CActorsManager::SActorLODConfig_Internal::CacheSqrDistances()
{
	m_deadZoneStartSqr = Red::Math::MSqr( m_distance );
	m_deadZoneEndSqr = Red::Math::MSqr( m_distance + m_deadZone );
}

CActorsManager::CActorsManager( const SGameplayConfig* config )
	: m_forcedLOD( nullptr )
	, m_referencePositionValid( false )
	, m_world( nullptr )
	, m_pathLibWorld( nullptr )
	, m_physicsWorld( nullptr )
{
#ifdef QUAD_TREE_THREAD_SAFETY_ENABLED
	EnableThreadValidation();
#endif
	OnReloadedConfig( config );
}

void CActorsManager::OnReloadedConfig( const SGameplayConfig* config )
{
	if ( !m_lods.Empty() )
	{
		// Reset LOD for all actors

		const SActorLODConfig* highestLOD = &m_lods[ 0 ];

		for ( auto it = m_nodeToEntry.Begin(), end = m_nodeToEntry.End(); it != end; ++it )
		{
			CActor* actor = it->m_first;

			SActorLODState& lodState = actor->m_LOD;
			lodState.m_timeInvisible = 0.0f;
			lodState.m_desiredLOD = highestLOD;

			SActorLODInstanceInfo instanceInfo;
			instanceInfo.m_hasCollisionDataAround = true;
			instanceInfo.m_hasNavigationDataAround = true;

			actor->UpdateLODLevel( &instanceInfo );
		}
	}

	// Set up LODs based on new config

	m_invisibilityTimeThreshold = config->m_LOD.m_actorInvisibilityTimeThreshold;

	m_lods.Resize( config->m_LOD.m_actorLODs.Size() );
	for ( Uint32 i = 0; i < config->m_LOD.m_actorLODs.Size(); ++i )
	{
		*( ( SActorLODConfig* ) &m_lods[ i ]) = config->m_LOD.m_actorLODs[ i ];
	}

	for ( Uint32 i = 0; i < m_lods.Size(); ++i )
	{
		m_lods[ i ].CacheSqrDistances();
	}

	m_forcedLOD = nullptr;

	// Apply new LODs

	UpdateLODs( 0.0f );
}

CActorsManager::~CActorsManager()
{
}

void CActorsManager::OnGameStart( CWorld* world )
{
	m_world = world;
	m_pathLibWorld = world->GetPathLibWorld();
	world->GetPhysicsWorld( m_physicsWorld );
}

void CActorsManager::UpdateLODs( Float deltaTime )
{
	PC_SCOPE_PIX( UpdateActorLODs );

	if ( !m_referencePositionValid )
	{
		return;
	}

	for ( auto it = m_nodeToEntry.Begin(), end = m_nodeToEntry.End(); it != end; ++it )
	{
		UpdateLODForActor( it->m_first, deltaTime );
	}
}

void CActorsManager::UpdateLODForActor( CActor* actor, Float deltaTime )
{
#if 0 // This disables actor LOD management
	return;
#endif

	SActorLODInstanceInfo instanceInfo;
	SActorLODState& lod = actor->m_LOD;
	const Vector& pos = actor->GetWorldPositionRef();

	// Update visibility

#if defined( USE_UMBRA ) && !defined( RED_FINAL_BUILD )

	Bool isVisible = true; // Report as "visible" if we have no umbra data
	if ( const CUmbraScene* umbraScene = m_world->GetUmbraScene() )
	{
		if ( umbraScene->HasData() )
		{
			isVisible = actor->WasVisibleLastFrame();
		}
	}

#else

	const Bool isVisible = actor->WasVisibleLastFrame();

#endif

	if ( isVisible )
	{
		lod.m_timeInvisible = 0.0f;
	}
	else
	{
		lod.m_timeInvisible += deltaTime;
	}

	// Determine collision data presence

#if 0 // Shouldn't this be enough?
	instanceInfo.m_hasCollisionDataAround = m_physicsWorld->IsTerrainPresentAt( pos.AsVector2() );
#else
#ifdef USE_PHYSX
	instanceInfo.m_hasCollisionDataAround = true;
	if ( CMovingPhysicalAgentComponent* physComponent = Cast< CMovingPhysicalAgentComponent >( actor->GetMovingAgentComponent() ) )
	{
		PC_SCOPE( UpdateActorLODs_HasCollisionAround )

		if ( const CMRPhysicalCharacter* physCharacter = physComponent->GetPhysicalCharacter() )
		{
			if ( CPhysicsCharacterWrapper* physWrapper = physCharacter->GetCharacterController() )
			{
#ifdef USE_PHYSX
				instanceInfo.m_hasCollisionDataAround = physWrapper->CanMove( pos );
#endif
			}
		}
	}
#endif // USE_PHYSX
#endif

	// Determine navigation data presence

	const Box& bbox = m_pathLibWorld->GetStreamedWorldBox();
	instanceInfo.m_hasNavigationDataAround = bbox.Contains2D( pos.AsVector3() );

	// Determine desired LOD

	const SActorLODConfig_Internal* desiredLOD = nullptr;

	if ( !m_forcedLOD )
	{
		const SActorLODConfig_Internal* highestLOD = &m_lods[ 0 ];

		const SActorLODConfig_Internal* currentLOD = static_cast< const SActorLODConfig_Internal* >( lod.m_desiredLOD );
		const SActorLODConfig_Internal* prevLOD = ( currentLOD && currentLOD != highestLOD ) ? ( currentLOD - 1 ) : nullptr;

		// Check if LOD changed at all

		const Float distanceSqr = instanceInfo.m_distanceSqr = pos.DistanceSquaredTo( m_referencePosition );
		if ( ( !prevLOD || prevLOD->m_deadZoneStartSqr <= distanceSqr ) &&
			 ( currentLOD && distanceSqr < currentLOD->m_deadZoneEndSqr ) )
		{
			desiredLOD = currentLOD;
		}

		// LOD changed - determine new one

		else
		{
			const Uint32 numLODsToCheck = m_lods.Size() - 1;
			for ( Uint32 i = 0; i < numLODsToCheck; ++i )
			{
				if ( distanceSqr <= m_lods[ i ].m_deadZoneStartSqr )
				{
					desiredLOD = &m_lods[ i ];
					break;
				}
			}
			if ( !desiredLOD )
			{
				desiredLOD = &m_lods.Back();
			}

			// Take dead zone into account

			if ( desiredLOD + 1 == currentLOD && desiredLOD->m_deadZoneEndSqr < distanceSqr )
			{
				desiredLOD = currentLOD;
			}
		}
	}
	else
	{
		desiredLOD = m_forcedLOD;
	}

	// Update LOD

	lod.m_desiredLOD = desiredLOD;
	actor->UpdateLODLevel( &instanceInfo );
}

void CActorsManager::EnableForcedLOD( Bool enable, Uint32 index )
{
	m_forcedLOD = enable ? &m_lods[ index ] : nullptr;
	UpdateLODs( 0.0f );
}

void funcCPPForceActorLOD( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool, enable, false );
	GET_PARAMETER( Uint32, LODIndex, 0 );
	FINISH_PARAMETERS;

	GCommonGame->GetActorsManager()->EnableForcedLOD( enable, LODIndex );
}

void RegisterActorManagerScriptFunctions()
{	
	NATIVE_GLOBAL_FUNCTION( "CPPForceActorLOD", funcCPPForceActorLOD );
}

Bool CActorsManager::TestLocation( const Vector& pos, Float radius, CActor* ignoreActor )
{
	struct TestFunctor : Red::System::NonCopyable
	{
		enum { SORT_OUTPUT = false };

		TestFunctor( const Vector2& pos, Float radius, CActor* ignoreActor )
			: m_pos( pos )
			, m_radius( radius )
			, m_ignoreActor( ignoreActor )
			, m_hasHit( false ) {}

		RED_INLINE Bool operator()( const CActorsManagerMemberData& member )
		{
			CActor* actor = member.Get();
			if ( actor == m_ignoreActor )
			{
				return true;
			}
			const Vector& actorPos = actor->GetWorldPositionRef();
			Float radius = m_radius + actor->GetRadius();
			if ( (actorPos.AsVector2() - m_pos.AsVector2()).SquareMag() <= radius*radius )
			{
				m_hasHit = true;
				return false;
			}
			return true;
		}
		Vector2			m_pos;
		Float			m_radius;
		CActor*			m_ignoreActor;
		Bool			m_hasHit;
	} functor( pos.AsVector2(), radius, ignoreActor );

	Float testRadius = radius + MAX_AGENT_RADIUS;

	/// TODO: Max Height?
	Box bbox( Vector( -testRadius, -testRadius, -2.f ), Vector( testRadius, testRadius, 2.f ) );

	TQuery( pos, functor, bbox, true, NULL, 0 );

	return !functor.m_hasHit;
}

Bool CActorsManager::TestLine( const Vector& posFrom, const Vector& posTo, Float radius, CActor* ignoreActor, Bool ignoreGhostCharacters )
{
	Float testRadius = radius + MAX_AGENT_RADIUS;

	Box bbox( Box::RESET_STATE );
	bbox.AddPoint( Vector( 0, 0, 0, 0 ) );
	bbox.AddPoint( posTo - posFrom );
	bbox.Min.AsVector3() += Vector3( -testRadius, -testRadius, -2.f );
	bbox.Max.AsVector3() += Vector3( testRadius, testRadius, 2.f );

	LineTestIgnoreActorFunctor functor( posFrom.AsVector2(), posTo.AsVector2(), radius, ignoreActor, ignoreGhostCharacters );
	TQuery( posFrom, functor, bbox, true, NULL, 0 );
	return !functor.m_hasHit;
}
Bool CActorsManager::TestLine( const Vector& posFrom, const Vector& posTo, Float radius, CActor** ignoreActor, Uint32 ignoredActorsCount, Bool ignoreGhostCharacters )
{
	Float testRadius = radius + MAX_AGENT_RADIUS;

	Box bbox( Box::RESET_STATE );
	bbox.AddPoint( Vector( 0, 0, 0, 0 ) );
	bbox.AddPoint( posTo - posFrom );
	bbox.Min.AsVector3() += Vector3( -testRadius, -testRadius, -2.f );
	bbox.Max.AsVector3() += Vector3( testRadius, testRadius, 2.f );

	LineTestIgnoreMultipleFunctor functor( posFrom.AsVector2(), posTo.AsVector2(), radius, ignoreActor, ignoredActorsCount, ignoreGhostCharacters );
	TQuery( posFrom, functor, bbox, true, NULL, 0 );
	return !functor.m_hasHit;

}

Int32 CActorsManager::CollectActorsAtLine( const Vector& posFrom, const Vector& posTo, Float radius, CActor** outputArray, Int32 maxElems )
{
	Float testRadius = radius + MAX_AGENT_RADIUS;

	Box bbox( Box::RESET_STATE );
	bbox.AddPoint( Vector( 0, 0, 0, 0 ) );
	bbox.AddPoint( posTo - posFrom );
	bbox.Min.AsVector3() += Vector3( -testRadius, -testRadius, -2.f );
	bbox.Max.AsVector3() += Vector3( testRadius, testRadius, 2.f );

	CollectActorsOnLineFunctor functor( outputArray, maxElems, posFrom, posTo, radius );

	TQuery( posFrom, functor, bbox, true, NULL, 0 );

	return functor.m_nextElem;
}

void CActorsManager::GetClosestToNode( const CNode& node, TDynArray< TPointerWrapper< CActor > >& output, const Box& aabb, Uint32 maxElems, const INodeFilter** filters, const Uint32 numFilters )
{
	if ( node.IsA< CActor >() )
	{
		GetClosestToEntity( static_cast<const CActor&>( node ), output, aabb, maxElems, filters, numFilters );
	}
	else
	{
		GetClosestToPoint( node.GetWorldPosition(), output, aabb, maxElems, filters, numFilters );
	}
}
void CActorsManager::GetClosestToEntity( const CActor& actor, TDynArray< TPointerWrapper< CActor > >& output, const Box& aabb, Uint32 maxElems, const INodeFilter** filters, const Uint32 numFilters )
{
	PC_SCOPE( ActorsStorage );
	CollectActorsFunctor functor( output, maxElems );
	TQuery( actor, functor, aabb, true, filters, numFilters );
}
void CActorsManager::GetClosestToPoint( const Vector& position, TDynArray< TPointerWrapper< CActor > >& output, const Box& aabb, Uint32 maxElems, const INodeFilter** filters, const Uint32 numFilters )
{
	PC_SCOPE( ActorsStorage );
	CollectActorsFunctor functor( output, maxElems );
	TQuery( position, functor, aabb, true, filters, numFilters );
}
void CActorsManager::GetAll( TDynArray< TPointerWrapper< CActor > >& output, Uint32 maxElems, const INodeFilter** filters, const Uint32 numFilters )
{
	PC_SCOPE( ActorsStorage );
	CollectActorsFunctor functor( output, maxElems );
	TQuery( functor, filters, numFilters );
}

void CActorsManager::Add( CActor* actor )
{
	CActorsManagerMemberData member;
	member.m_actor = actor;
	CQuadTreeStorage::Add( member );

	UpdateLODForActor( actor, 0.0f );
}

void CActorsManager::Remove( CActor* actor )
{
	CActorsManagerMemberData member;
	member.m_actor = actor;
	CQuadTreeStorage::Remove( member );
}

void CActorsManager::Update( Float deltaTime )
{
	UpdateLODs( deltaTime );
}