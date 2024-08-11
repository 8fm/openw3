#include "build.h"
#include "aimHelpTargetGatherer.h"
#include "r6Player.h"
#include "r6InventoryItemComponent.h"
#include "../../common/game/gameplayStorage.h"
#include "aimTarget.h"
#include "../../common/engine/physicsBodyWrapper.h"

IMPLEMENT_ENGINE_CLASS( SAimHelpParams );
IMPLEMENT_ENGINE_CLASS( CAimHelpTargetsGatherer );


const Uint32	CAimHelpTargetsGatherer::AIM_MAX_ENTITIES = 1000;
const Uint32	CAimHelpTargetsGatherer::AIM_MAX_LINES_OF_SIGHT = 10;
const Float		CAimHelpTargetsGatherer::AIM_AABB_EXTRA_MARGIN = 2.0f;


//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
CAimHelpTargetsGatherer::CAimHelpTargetsGatherer() :
	m_backDistanceForAngle(5.0f)
{
}

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
void CAimHelpTargetsGatherer::AddAimingHelpParams( SAimHelpParams& aimParams )
{
	m_aimParams.m_nearDistance	= Max( m_aimParams.m_nearDistance	, aimParams.m_nearDistance );
	m_aimParams.m_nearAngle		= Max( m_aimParams.m_nearAngle		, aimParams.m_nearAngle / 180.0f * M_PI );
	m_aimParams.m_farDistance	= Max( m_aimParams.m_farDistance	, aimParams.m_farDistance );
	m_aimParams.m_farAngle		= Max( m_aimParams.m_farAngle		, aimParams.m_farAngle / 180.0f * M_PI );
}

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
void CAimHelpTargetsGatherer::PreUpdate()
{
	m_areEntitiesSorted	= false;
}

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
void CAimHelpTargetsGatherer::GatherValidEntitiesInBox(const Vector& origin, const Vector& intendedDirection)
{
	// Functor to get the gameplay entities with the target component only
	struct AimTargetFunctor : public CGameplayStorage::DefaultFunctor
	{
		TDynArray< TPointerWrapper< CAimTarget > >& m_out;

		AimTargetFunctor( TDynArray< TPointerWrapper< CAimTarget > >& out ) : m_out( out ) { }

		RED_INLINE Bool operator()( const TPointerWrapper< CGameplayEntity >& ptr )
		{
			for ( ComponentIterator< CAimTarget > it( ptr.Get() ); it; ++it )	
			{
				ComponentIterator< CAimTarget > comp( it );
				CAimTarget* target	= *comp;
				if ( target )
				{
					m_out.PushBack( target );
				}
			}
			return true;
		}
	};

	Float margin	= (m_aimParams.m_farDistance * Abs(sinf(m_aimParams.m_farAngle))) + AIM_AABB_EXTRA_MARGIN;
	Float distance	= m_aimParams.m_farDistance * 0.5f;
	Vector aabbMax(abs(intendedDirection.X) * distance + margin, abs(intendedDirection.Y) * distance + margin, abs(intendedDirection.Z) * distance + margin);

	m_gameplayEntitiesInRange.ClearFast();
	m_targetsInRange.ClearFast();

	STATIC_NODE_FILTER( IsNotPlayer, filterNotPlayer );
	static const INodeFilter* filters[] = { &filterNotPlayer };

	AimTargetFunctor func( m_gameplayEntitiesInRange );

	GCommonGame->GetGameplayStorage()->TQuery( origin + intendedDirection * distance, func, Box( -aabbMax, aabbMax ), true, NULL, 0 );
	//GCommonGame->GetGameplayStorage()->GetClosestToPoint(origin + intendedDirection * distance, m_gameplayEntitiesInRange,  Box(-aabbMax, aabbMax), AIM_MAX_ENTITIES, filters, 1);
	//GCommonGame->GetGameplayStorage()->GetAll( m_gameplayEntitiesInRange, AIM_MAX_ENTITIES, NULL, 0);
}

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
void CAimHelpTargetsGatherer::SortGatheredEntities( const Vector& origin, const Vector& intendedDirection )
{
	CGameplayEntity*		owner; 

	Uint32					numEntities;
	CGameplayEntity*		entityPointed;
	CAimTarget*				aimTarget;
	Vector					toEntityDirection;
	Vector					toEntityFromBehind;
	Float					entityDistance;
	Float					entityAngle;

	owner = ( CGameplayEntity* ) GetEntity();
	if(!owner)
	{
		return;
	}

	// Get all entities in front of the owner
	GatherValidEntitiesInBox(origin, intendedDirection);

	// No entities?
	if( m_gameplayEntitiesInRange.Empty() )
	{
		return;
	}

	// Get some data to test against
	CR6Player* player		= Cast< CR6Player >( owner );
	CEntity* equippedItem	= NULL;
	if( player != NULL && player->GetEquippedItem() != NULL )
	{
		equippedItem	= player->GetEquippedItem()->GetEntity();
	}

	numEntities	= m_gameplayEntitiesInRange.Size();
	for(Uint32 i = 0; i < numEntities; ++i)
	{
		aimTarget		= m_gameplayEntitiesInRange[i].Get();
		if( !IsValidObject( aimTarget ) )
		{
			continue;
		}

		entityPointed	= Cast< CGameplayEntity > ( aimTarget->GetEntity() );
		if( entityPointed == NULL )
		{
			continue;
		}

		// The entity can't be himself or the camera
		if( entityPointed	== owner )
		{
			continue;
		}

		// Can't be targeted entity equipped by player
		if( entityPointed == equippedItem )
		{
			continue;
		}

		// Get the position
		Vector3	position	= aimTarget->GetWorldPosition();

		// Get the vector to the aimTarget
		toEntityDirection	= position - origin;
		entityDistance		= toEntityDirection.Mag3();

		// The aimTarget needs to be close enough
		if(entityDistance > m_aimParams.m_farDistance)
		{
			continue;
		}

		// get the proper normalized direction
		toEntityDirection	/= entityDistance;

		toEntityFromBehind	= position  - (origin - intendedDirection * m_backDistanceForAngle);
		toEntityFromBehind.Normalize3();
		//entityAngle		= acosf(Vector::Dot3(toEntityDirection, intendedDirection));
		entityAngle		= acosf(Vector::Dot3(toEntityFromBehind, intendedDirection));
		// The angle we tolerate depends on the two distances
		if(entityAngle <m_aimParams.m_farAngle || (entityDistance < m_aimParams.m_nearDistance && entityAngle < m_aimParams.m_nearAngle))
		{
			m_targetsInRange.PushBack(SPotentialAimHelpTarget(entityPointed, toEntityDirection, entityDistance, entityAngle));
		}
	}
	
	// No targets, no correction
	if(m_targetsInRange.Empty())
	{
		return;
	}

	// Sort the targets by angle distance
	Sort(m_targetsInRange.Begin(), m_targetsInRange.End());

	m_areEntitiesSorted	= true;
}

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
CGameplayEntity* CAimHelpTargetsGatherer::GetClosestValidEntity( const Vector& origin, const Vector& intendedDirection )
{
	// Gather and sort only if not done this frame
	if( !m_areEntitiesSorted )
	{
		SortGatheredEntities(origin, intendedDirection);
	}

	// Check if there are any entities
	if(m_targetsInRange.Empty())
	{
		return NULL;
	}

	// Return the first one
	return m_targetsInRange[0].m_entity;
}

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
CGameplayEntity* CAimHelpTargetsGatherer::GetClosestValidEntityInParams( const Vector& origin, const Vector& intendedDirection, SAimHelpParams& aimParams )
{
	// Gather and sort only if not done this frame
	if( !m_areEntitiesSorted )
	{
		SortGatheredEntities(origin, intendedDirection);
	}

	// Check if there are any entities
	if(m_targetsInRange.Empty())
	{
		return NULL;
	}

	// Return the first one that fits in the params
	Uint32	numEntities	= m_targetsInRange.Size();
	for(Uint32 i = 0; i < numEntities; ++i)
	{
		// Discard all if the closest one is angularly too far
		if(m_targetsInRange[i].m_angle > aimParams.m_nearAngle)
		{
			break;
		}

		// Discard this if it is linearly too far
		if(m_targetsInRange[i].m_distance > aimParams.m_farDistance)
		{
			continue;
		}

		// Discard this if angularly too far
		if(m_targetsInRange[i].m_distance > aimParams.m_nearDistance)
		{
			if(m_targetsInRange[i].m_angle > aimParams.m_farAngle)
			{
				continue;
			}
		}
		else if (m_targetsInRange[i].m_angle > aimParams.m_nearAngle)
		{
			continue;
		}

		// Reached this point, we found the closest suitable entity
		return m_targetsInRange[i].m_entity;
	}

	return NULL;
}

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
CGameplayEntity* CAimHelpTargetsGatherer::GetClosestValidEntityInLineOfSight( const Vector& origin, const Vector& intendedDirection )
{
	CPhysicsWorld* physicsWorld = GGame->GetActiveWorld()->GetPhysicsWorld();
	if ( !physicsWorld )
	{
		return NULL;
	}

	// Gather and sort only if not done this frame
	if( !m_areEntitiesSorted )
	{
		SortGatheredEntities(origin, intendedDirection);
	}

	// Check if there are any entities
	if(m_targetsInRange.Empty())
	{
		return NULL;
	}

	// Return the first one with line of sight
	Vector	rayEnd		= origin + intendedDirection * m_aimParams.m_farDistance;

	Uint32 maxTargets	= m_targetsInRange.Size() < AIM_MAX_LINES_OF_SIGHT ? m_targetsInRange.Size() : AIM_MAX_LINES_OF_SIGHT;
	for( Uint32 i = 0; i < maxTargets; ++i )
	{
		// Calculate line of sight only if not calculated already
		if( m_targetsInRange[i].m_lineOfSight == AHLOS_NotSet )
		{
			SPhysicsContactInfo contactInfo;
			Bool				impacted;
			STATIC_GAME_ONLY CPhysicsEngine::CollisionMask include =	GPhysicEngine->GetCollisionTypeBit( CNAME( Character ) ) 
																	|	GPhysicEngine->GetCollisionTypeBit( CNAME( Static ) ) 
																	|	GPhysicEngine->GetCollisionTypeBit( CNAME( Terrain ) )  
																	|	GPhysicEngine->GetCollisionTypeBit( CNAME( RigidBody ) );
			STATIC_GAME_ONLY CPhysicsEngine::CollisionMask exclude =	GPhysicEngine->GetCollisionTypeBit( CNAME( Player ) );

			// Test line of sight
			impacted	= physicsWorld->RayCastWithSingleResult( origin, rayEnd, include, exclude, contactInfo ) == TRV_Hit;
			if( impacted )
			{
				Float realDistance	= contactInfo.m_distance * m_aimParams.m_farDistance; //contactInfo.m_position.DistanceTo( origin );
				STATIC_GAME_ONLY Float	distanceHack	= 1.0f;
				if( realDistance >= m_targetsInRange[i].m_distance - distanceHack )
				{
					m_targetsInRange[i].m_lineOfSight  =  AHLOS_Yes;
				}
				else
				{
					m_targetsInRange[ i ].m_lineOfSight = AHLOS_No;
					CComponent* component = nullptr;
					if ( contactInfo.m_userDataA && contactInfo.m_userDataA->GetParent( component, contactInfo.m_rigidBodyIndexA.m_actorIndex ) ) 
					{
						CComponent* component = nullptr;
						contactInfo.m_userDataA->GetParent( component, contactInfo.m_rigidBodyIndexA.m_actorIndex );
						CEntity* collidingEntity = component->GetEntity();
						if ( collidingEntity == m_targetsInRange[ i ].m_entity )
						{
							m_targetsInRange[ i ].m_lineOfSight  =  AHLOS_Yes;
						}
					}
				}
			}
			else
			{
				m_targetsInRange[i].m_lineOfSight  =  AHLOS_Yes;
			}
		}
		if( m_targetsInRange[i].m_lineOfSight == AHLOS_Yes )
		{
			return m_targetsInRange[i].m_entity;
		}
	}
	return NULL;
}

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
void CAimHelpTargetsGatherer::funcPreUpdate( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	PreUpdate();
}

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
void CAimHelpTargetsGatherer::funcAddAimingHelpParams( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SAimHelpParams, aimParams, SAimHelpParams() );
	FINISH_PARAMETERS;

	AddAimingHelpParams( aimParams );
}

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
void CAimHelpTargetsGatherer::funcGetClosestValidEntity( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, origin, Vector::ZEROS );
	GET_PARAMETER( Vector, direction, Vector::ZEROS );
	FINISH_PARAMETERS;

	RETURN_HANDLE( CGameplayEntity, GetClosestValidEntity( origin, direction ) );
}

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
void CAimHelpTargetsGatherer::funcGetClosestValidEntityInParams( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, origin, Vector::ZEROS );
	GET_PARAMETER( Vector, direction, Vector::ZEROS );
	GET_PARAMETER( SAimHelpParams , aimParams, SAimHelpParams() );
	FINISH_PARAMETERS;

	RETURN_HANDLE( CGameplayEntity, GetClosestValidEntityInParams( origin, direction, aimParams ) );
}

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
void CAimHelpTargetsGatherer::funcGetClosestValidEntityInSight( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, origin, Vector::ZEROS );
	GET_PARAMETER( Vector, direction, Vector::ZEROS );
	FINISH_PARAMETERS;

	RETURN_HANDLE( CGameplayEntity, GetClosestValidEntityInLineOfSight( origin, direction ) );
}
