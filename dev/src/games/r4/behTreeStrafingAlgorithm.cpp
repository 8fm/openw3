#include "build.h"
#include "behTreeStrafingAlgorithm.h"

#include "../../common/game/behTreeInstance.h"

////////////////////////////////////////////////////////////////////////////
// CBehTreeStrafingAlgorithmDefinition
////////////////////////////////////////////////////////////////////////////
IMPLEMENT_ENGINE_CLASS( CBehTreeStrafingAlgorithmDefinition );
IMPLEMENT_ENGINE_CLASS( CBehTreeStrafingAlgorithmListDefinition );
IMPLEMENT_ENGINE_CLASS( CBehTreeStrafingAlgorithmFastSurroundDefinition );
IMPLEMENT_ENGINE_CLASS( CBehTreeStrafingAlgorithmNeverBackDownDefinition );

////////////////////////////////////////////////////////////////////////////
// CBehTreeStrafingAlgorithmDefinition::SpawnInstance
////////////////////////////////////////////////////////////////////////////
CBehTreeStrafingAlgorithmInstance* CBehTreeStrafingAlgorithmDefinition::SpawnInstance( CBehTreeNodeStrafingInstance* node, CBehTreeSpawnContext& context ) const
{
	ASSERT( false );
	ASSUME( false );
	return nullptr;
}

CBehTreeStrafingAlgorithmInstance* CBehTreeStrafingAlgorithmListDefinition::SpawnInstance( CBehTreeNodeStrafingInstance* node, CBehTreeSpawnContext& context ) const
{
	return new Instance( this, node, context );
}

CBehTreeStrafingAlgorithmInstance* CBehTreeStrafingAlgorithmFastSurroundDefinition::SpawnInstance( CBehTreeNodeStrafingInstance* node, CBehTreeSpawnContext& context ) const
{
	return new Instance( this, node, context );
}

CBehTreeStrafingAlgorithmInstance* CBehTreeStrafingAlgorithmNeverBackDownDefinition::SpawnInstance( CBehTreeNodeStrafingInstance* node, CBehTreeSpawnContext& context ) const
{
	return new Instance( this, node, context );
}



////////////////////////////////////////////////////////////////////////////
// CBehTreeStrafingAlgorithmInstance
////////////////////////////////////////////////////////////////////////////

void CBehTreeStrafingAlgorithmInstance::SetMoveType( EMoveType moveType )
{
	CActor* actor = m_node->GetOwner()->GetActor();
	CMovingAgentComponent* mac = actor->GetMovingAgentComponent();
	if ( mac )
	{
		mac->SetMoveType( moveType );
		m_node->m_useCustomSpeed = true;
		m_node->m_customSpeed = mac->GetSpeedForMoveType( moveType, 0.0f );
	}
}

void CBehTreeStrafingAlgorithmInstance::SetDefaultMoveType()
{
	CActor* actor = m_node->GetOwner()->GetActor();
	CMovingAgentComponent* mac = actor->GetMovingAgentComponent();
	if ( mac )
	{
		mac->SetMoveType( m_node->GetDefaultMoveType() );
		m_node->m_useCustomSpeed = false;
	}
}

void CBehTreeStrafingAlgorithmInstance::Activate()
{

}
void CBehTreeStrafingAlgorithmInstance::Deactivate()
{

}
void CBehTreeStrafingAlgorithmInstance::UpdateHeading( Vector2& inoutHeading )
{

}



////////////////////////////////////////////////////////////////////////////
// CBehTreeStrafingAlgorithmListInstance
////////////////////////////////////////////////////////////////////////////

CBehTreeStrafingAlgorithmListInstance::CBehTreeStrafingAlgorithmListInstance( const Definition* def, CBehTreeNodeStrafingInstance* node, CBehTreeSpawnContext& context )
	: Super( def, node, context )
{
	m_list.Reserve( def->m_list.Size() );
	for( auto it = def->m_list.Begin(), end = def->m_list.End(); it != end; ++it )
	{
		CBehTreeStrafingAlgorithmInstance* instance = (*it)->SpawnInstance( node, context );
		if ( instance )
		{
			m_list.PushBack( instance );
		}
	}
}

CBehTreeStrafingAlgorithmListInstance::~CBehTreeStrafingAlgorithmListInstance()
{
	for( auto it = m_list.Begin(), end = m_list.End(); it != end; ++it )
	{
		delete *it;
	}
}

void CBehTreeStrafingAlgorithmListInstance::Activate()
{
	for( auto it = m_list.Begin(), end = m_list.End(); it != end; ++it )
	{
		(*it)->Activate();
	}
}
void CBehTreeStrafingAlgorithmListInstance::Deactivate()
{
	for( auto it = m_list.Begin(), end = m_list.End(); it != end; ++it )
	{
		(*it)->Deactivate();
	}
}
void CBehTreeStrafingAlgorithmListInstance::UpdateHeading( Vector2& inoutHeading )
{
	for( auto it = m_list.Begin(), end = m_list.End(); it != end; ++it )
	{
		(*it)->UpdateHeading( inoutHeading );
	}
}


////////////////////////////////////////////////////////////////////////////
// CBehTreeStrafingAlgorithmFastSurroundInstance
////////////////////////////////////////////////////////////////////////////
	
CBehTreeStrafingAlgorithmFastSurroundInstance::CBehTreeStrafingAlgorithmFastSurroundInstance( const Definition* def, CBehTreeNodeStrafingInstance* node, CBehTreeSpawnContext& context )
	: Super( def, node, context )
	, m_usageDelay( def->m_usageDelay.GetVal( context ) )
	, m_verticalHeadingLimitToBreak( def->m_verticalHeadingLimitToBreak.GetVal( context ) )
	, m_surroundMoveType( def->m_surroundMoveType.GetVal( context ) )
	, m_isFastSurrounding( false )
	, m_nextActivationDelay( -1.f )
{
	m_distanceToActivateSq = def->m_distanceToActivate.GetVal( context );
	m_distanceToActivateSq *= m_distanceToActivateSq;

	m_distanceToBreakSq = def->m_distanceToBreak.GetVal( context );
	m_distanceToBreakSq *= m_distanceToBreakSq;

	m_speedMinToActivateSq = def->m_speedMinToActivate.GetVal( context );
	m_speedMinToActivateSq *= m_speedMinToActivateSq;

	m_speedMinLimitToBreakSq = def->m_speedMinLimitToBreak.GetVal( context );
	m_speedMinLimitToBreakSq *= m_speedMinLimitToBreakSq;
}

void CBehTreeStrafingAlgorithmFastSurroundInstance::BreakFastSurround()
{
	CBehTreeInstance* instance = m_node->GetOwner();
	CActor* actor = instance->GetActor();

	m_isFastSurrounding = false;
	m_nextActivationDelay = instance->GetLocalTime() + m_usageDelay * ( 0.5f + GEngine->GetRandomNumberGenerator().Get< Float >() );

	LockOrientation( true );
	actor->SignalGameplayEvent( CNAME( AI_Strafing_FastSurround ),  0 );
	SetDefaultMoveType();
}
void CBehTreeStrafingAlgorithmFastSurroundInstance::StartFastSurround()
{
	CBehTreeInstance* instance = m_node->GetOwner();
	CActor* actor = instance->GetActor();

	m_isFastSurrounding = true;
	m_isFastSurroundingSince = instance->GetLocalTime();

	LockOrientation( false );
	actor->SignalGameplayEvent( CNAME( AI_Strafing_FastSurround ),  1 );
	SetMoveType( m_surroundMoveType );
	m_hasLowVelocity = false;
}

void CBehTreeStrafingAlgorithmFastSurroundInstance::Activate()
{
	m_isFastSurrounding = false;
}

void CBehTreeStrafingAlgorithmFastSurroundInstance::UpdateHeading( Vector2& inoutHeading )
{
	if( m_isFastSurrounding )
	{
		CBehTreeInstance* instance = m_node->GetOwner();

		// velocity test (that takes into calculation all other strafind processes
		CActor* actor = instance->GetActor();
		const Vector& velocity = actor->GetMovingAgentComponent()->GetVelocity();
		Float localTime = instance->GetLocalTime();
		if ( velocity.AsVector2().SquareMag() < m_speedMinLimitToBreakSq )
		{
			if ( !m_hasLowVelocity )
			{
				m_hasLowVelocity = true;
				m_lowVelocityBreakDelay = localTime + 0.25f;
			}
		}
		else
		{
			m_hasLowVelocity = false;
		}

		// Fast surrounding is on. Check if we shouldn't break it.
		if (
			inoutHeading.Y >= m_verticalHeadingLimitToBreak &&
			inoutHeading.SquareMag() >= m_speedMinLimitToBreakSq &&
			(!m_hasLowVelocity || m_lowVelocityBreakDelay > instance->GetLocalTime()) &&
			inoutHeading.SquareMag() > (0.5f*0.5f) &&
			((m_isFastSurroundingSince + 1.f) < localTime || GetCurrentWorldHeading().SquareMag() > (0.5f*0.5f)) )
		{

			Float angularDistance = GetCurrentDesiredAngleDistance();
			Float targetDistance = GetCurrentTargetDistance();
			Float desiredDistance = GetDesiredRange();

			Float distance2DSq = (targetDistance*targetDistance) + (desiredDistance*desiredDistance) - 2 * targetDistance * desiredDistance * ::cosf( DEG2RAD( angularDistance ) );
			if ( distance2DSq > m_distanceToBreakSq )
			{
				return;
			}
		}
		BreakFastSurround();
	}
	else
	{
		CBehTreeInstance* owner = m_node->GetOwner();
		if ( m_nextActivationDelay < owner->GetLocalTime() && inoutHeading.Y > m_verticalHeadingLimitToBreak )
		{
			Float angularDistance = GetCurrentDesiredAngleDistance();
			Float targetDistance = GetCurrentTargetDistance();
			Float desiredDistance = GetDesiredRange();

			// calculate 2d desired position distance (cosinus theorem)
			Float distance2DSq = (targetDistance*targetDistance) + (desiredDistance*desiredDistance) - 2 * targetDistance * desiredDistance * ::cosf( DEG2RAD( angularDistance ) );
			if ( distance2DSq > m_distanceToActivateSq )
			{
				CActor* actor = owner->GetActor();
				const Vector& velocity = actor->GetMovingAgentComponent()->GetVelocity();
				if ( velocity.AsVector2().SquareMag() >= m_speedMinToActivateSq )
				{
					StartFastSurround();
				}
			}
		}
	}
}

////////////////////////////////////////////////////////////////////////////
// CBehTreeStrafingAlgorithmNeverBackDownInstance
////////////////////////////////////////////////////////////////////////////
void CBehTreeStrafingAlgorithmNeverBackDownInstance::UpdateHeading( Vector2& inoutHeading )
{
	if ( inoutHeading.Y < 0.f )
	{
		inoutHeading.Y = 0.f;
	}
}