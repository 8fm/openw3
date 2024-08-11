/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behTreeNodeAtomicFlyTo.h"

#include "../../common/engine/globalWater.h"
#include "../../common/engine/renderFrame.h"

BEHTREE_STANDARD_SPAWNNODE_FUNCTION( CBehTreeNodeAtomicFlyToDefinition )
BEHTREE_STANDARD_SPAWNNODE_FUNCTION( CBehTreeNodeAtomicFlyToPositionDefinition )


///////////////////////////////////////////////////////////////////////////////
// IBehTreeNodeAtomicFlyToBaseInstance
///////////////////////////////////////////////////////////////////////////////
Bool IBehTreeNodeAtomicFlyToBaseInstance::Activate()
{
	if ( !Super::Activate() )
	{
		return false;
	}

	m_flightData->SetForceReachTarget( m_forceReachDestination );
	return true;
}

void IBehTreeNodeAtomicFlyToBaseInstance::Update()
{
	// compute target
	Vector targetPos;
	if ( !GetFlyToPosition( targetPos ) )
	{
		Complete( BTTO_FAILED );
		return;
	}

	CActor* me = m_owner->GetActor();

	Vector myPos = me->GetWorldPositionRef();

	Vector2 targetDiff = myPos.AsVector2() - targetPos.AsVector2();
	Float targetDist2D = targetDiff.Normalize();

	// compute destination
	Vector desiredPosition = targetPos;
	// apply distance offset
	desiredPosition.AsVector2() += targetDiff * m_distanceOffset;
	// apply height offset
	desiredPosition.Z += m_heightOffset;
	
	CBehTreeFlightData* data = m_flightData.Get();
	data->SetDesiredPosition( desiredPosition );

	Super::Update();

	// success test
	Float distanceDiff = targetDist2D;
	if ( !m_checkDistanceWithoutOffsets )
	{
		distanceDiff = Abs( distanceDiff - m_distanceOffset );
	}
	if ( distanceDiff <= m_min2DDistance )
	{
		Bool heightOk = m_skipHeightCheck;
		if ( !heightOk )
		{
			Float heightDiff = myPos.Z - targetPos.Z;
			if ( !m_checkDistanceWithoutOffsets )
			{
				heightDiff -= m_heightOffset;
			}

			if ( m_useAbsoluteHeightDifference )
			{
				heightDiff = Abs( heightDiff );
			}

			if ( m_minHeight >= 0.f )
			{
				heightOk = heightDiff < m_minHeight;
			}
			else
			{
				heightOk = heightDiff >= m_minHeight;
			}
		}

		if ( heightOk )
		{
			Complete( BTTO_SUCCESS );
			return;
		}
	}
}

void IBehTreeNodeAtomicFlyToBaseInstance::OnGenerateDebugFragments( CRenderFrame* frame )
{
	Vector targetPos;
	if ( GetFlyToPosition( targetPos ) )
	{
		frame->AddDebugSphere( targetPos, 1.f, Matrix::IDENTITY, Color::GREEN );
	}


	Super::OnGenerateDebugFragments( frame );
}

///////////////////////////////////////////////////////////////////////////////
// CBehTreeNodeAtomicFlyToInstance
///////////////////////////////////////////////////////////////////////////////
Bool CBehTreeNodeAtomicFlyToInstance::GetFlyToPosition( Vector& outPosition )
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
CBehTreeNodeAtomicFlyToCustomTargetInstance::CBehTreeNodeAtomicFlyToCustomTargetInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: Super( def, owner, context, parent )
	, m_customTargetPtr( owner )
{

}

Bool CBehTreeNodeAtomicFlyToCustomTargetInstance::GetFlyToPosition( Vector& outPosition )
{
	outPosition = m_customTargetPtr->GetTarget();
	return true;
}

