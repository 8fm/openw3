/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behTreeNodeAtomicFlyAround.h"

#include "../../common/core/mathUtils.h"

#include "../../common/engine/areaComponent.h"
#include "../../common/engine/globalWater.h"


BEHTREE_STANDARD_SPAWNNODE_FUNCTION( CBehTreeNodeAtomicFlyAroundDefinition )
BEHTREE_STANDARD_SPAWNNODE_FUNCTION( CBehTreeNodeAtomicFlyAroundPositionDefinition )

///////////////////////////////////////////////////////////////////////////////
// IBehTreeNodeAtomicFlyAroundBaseInstance
///////////////////////////////////////////////////////////////////////////////
IBehTreeNodeAtomicFlyAroundBaseInstance::IBehTreeNodeAtomicFlyAroundBaseInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: Super( def, owner, context, parent )
	, m_distance( def.m_distance.GetVal( context ) )
	, m_distanceMax( def.m_distanceMax.GetVal( context ) )
	, m_height( def.m_height.GetVal( context ) )
	, m_heightMax( def.m_heightMax.GetVal( context ) )
	, m_randomizationDelay( def.m_randomizationDelay.GetVal( context ) )
	, m_pickTargetDistance( def.m_pickTargetDistance.GetVal( context ) )
	, m_guardAreaPtr()
{
	m_distanceMax = Max( m_distanceMax, m_distance );
	m_heightMax = Max( m_heightMax, m_height );
	if ( def.m_stayInGuardArea )
	{
		m_guardAreaPtr = CBehTreeGuardAreaDataPtr( owner );
	}
}


Bool IBehTreeNodeAtomicFlyAroundBaseInstance::Activate()
{
	Vector targetPos;
	if ( !GetFlyAroundPosition( targetPos ) )
	{
		return false;
	}
	// compute starting fly-around direction
	CActor* actor = m_owner->GetActor();
	Vector2 myPos = actor->GetWorldPositionRef().AsVector2();
	Vector2 myHeading = actor->GetWorldForward().AsVector2();

	Vector2 diff = myPos.AsVector2() - targetPos.AsVector2();
	Float crossZ = myHeading.CrossZ( diff );
	m_isGoingLeft = crossZ < 0.f;

	m_directionLockTimeout = m_owner->GetLocalTime() + 1.f;
	m_nextRandomizationTimeout = -1.f;
	

	return Super::Activate();
}


void IBehTreeNodeAtomicFlyAroundBaseInstance::Update()
{
	Vector targetPos;
	if ( !GetFlyAroundPosition( targetPos ) )
	{
		Complete( BTTO_FAILED );
		return;
	}

	CBehTreeFlightData* data = m_flightData.Get();

	// randomize desired distances
	Float time = m_owner->GetLocalTime();
	if ( time >= m_nextRandomizationTimeout )
	{
		auto& generator = GEngine->GetRandomNumberGenerator();
		m_nextRandomizationTimeout = time + m_randomizationDelay * generator.Get< Float >( 0.5f, 1.5f );
		m_desiredDistance = generator.Get< Float >( m_distance, m_distanceMax );
		m_desiredHeight = generator.Get< Float >( m_height, m_heightMax );
	}

	CActor* actor = m_owner->GetActor();
	// compute desired position
	Vector myPos = actor->GetWorldPositionRef();

	Vector2 diff = myPos.AsVector2() - targetPos.AsVector2();

	// a little calculation. We want to have position if DESIRED_POS_DISTANCE ahead on circle
	// so we want to convert this distance to an angle at which to rotate
	// So qutation is like 'angle/ 2 * PI = d / 2 * PI * r'
	// So we end up with 'angle = d / r' where angle is in radians
	const Float HALF_PI = 1.5707963267948966192313216916398f;
	Float angleRad = Min( m_pickTargetDistance / m_desiredDistance, HALF_PI );

	diff.Normalize();
	diff *= m_desiredDistance;

	if ( m_directionLockTimeout < time )
	{
		if ( data->GetPathDistanceFromDesiredTarget() > 6.f )
		{
			m_isGoingLeft = !m_isGoingLeft;
			m_directionLockTimeout = time + 4.f;

		}
		Vector2 myHeading = actor->GetWorldForward().AsVector2();
		Float crossZ = myHeading.CrossZ( diff );
		Bool goLeft = crossZ < 0.f;
		if( goLeft != m_isGoingLeft )
		{
			m_isGoingLeft = goLeft;
			m_directionLockTimeout = time + 2.5f;
		}
	}

	Vector desiredPosistion;
	
	// finally desired position quotation
	desiredPosistion.AsVector2() = targetPos.AsVector2() + MathUtils::GeometryUtils::Rotate2D( diff, m_isGoingLeft ? angleRad : -angleRad );
	desiredPosistion.Z = targetPos.Z + m_desiredHeight;

	// ok thats cool. Whats about guard area?
	CBehTreeGuardAreaData* guardAreaData = m_guardAreaPtr.Get();

	if ( guardAreaData )
	{
		CAreaComponent* guardArea = guardAreaData->GetGuardArea();
		if ( guardArea )
		{
			if ( !guardArea->TestPointOverlap( desiredPosistion ) )
			{
				desiredPosistion = guardArea->GetClosestPoint( desiredPosistion );
			}
		}
	}

	
	data->SetDesiredPosition( desiredPosistion );

	Super::Update();
}

CAreaComponent* IBehTreeNodeAtomicFlyAroundBaseInstance::GetAreaEncompassingMovement()
{
	CBehTreeGuardAreaData* guardAreaData = m_guardAreaPtr.Get();

	if ( guardAreaData )
	{
		return guardAreaData->GetGuardArea();
	}
	return nullptr;
}


///////////////////////////////////////////////////////////////////////////////
// CBehTreeNodeAtomicFlyAroundInstance
///////////////////////////////////////////////////////////////////////////////

Bool CBehTreeNodeAtomicFlyAroundInstance::GetFlyAroundPosition( Vector& outPosition )
{
	CNode* target = m_useCombatTarget ? static_cast< CNode* >( m_owner->GetCombatTarget().Get() ) : m_owner->GetActionTarget().Get();
	if ( !target )
	{
		return false;
	}

	outPosition = target->GetWorldPositionRef();

	// Clamp to 'above water'
	CGlobalWater* water = GGame->GetActiveWorld()->GetGlobalWater();
	if ( water )
	{
		Float waterLevel = water->GetWaterLevelBasic( outPosition.X, outPosition.Y );
		if ( outPosition.Z < waterLevel )
		{
			outPosition.Z = waterLevel;
		}
	}
	return true;
}

///////////////////////////////////////////////////////////////////////////////
// CBehTreeNodeAtomicFlyToCustomTargetInstance
///////////////////////////////////////////////////////////////////////////////
CBehTreeNodeAtomicFlyAroundPositionInstance::CBehTreeNodeAtomicFlyAroundPositionInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: Super( def, owner, context, parent )
	, m_customTargetPtr( owner )
{

}

Bool CBehTreeNodeAtomicFlyAroundPositionInstance::GetFlyAroundPosition( Vector& outPosition )
{
	outPosition = m_customTargetPtr->GetTarget();
	return true;
}


