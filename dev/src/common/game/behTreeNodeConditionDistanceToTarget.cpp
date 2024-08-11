#include "build.h"

#include "behTreeNodeConditionDistanceToTarget.h"
#include "behTreeInstance.h"
#include "../engine/tagManager.h"

BEHTREE_STANDARD_SPAWNDECORATOR_FUNCTION( CBehTreeNodeConditionDistanceToCombatTargetDefinition )
BEHTREE_STANDARD_SPAWNDECORATOR_FUNCTION( CBehTreeNodeConditionDistanceToActionTargetDefinition )
BEHTREE_STANDARD_SPAWNDECORATOR_FUNCTION( CBehTreeNodeConditionDistanceToCustomTargetDefinition )
BEHTREE_STANDARD_SPAWNDECORATOR_FUNCTION( CBehTreeNodeConditionDistanceToTaggedDefinition )
BEHTREE_STANDARD_SPAWNDECORATOR_FUNCTION( CBehTreeNodeConditionDistanceToNamedTargetDefinition )

////////////////////////////////////////////////////////////////////////
// CBehTreeNodeConditionDistanceToCombatTargetInstance
////////////////////////////////////////////////////////////////////////
CBehTreeNodeConditionDistanceToCombatTargetInstance::CBehTreeNodeConditionDistanceToCombatTargetInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: CBehTreeNodeConditionInstance( def, owner, context, parent )
	, m_checkRotation( def.m_checkRotation )
	, m_rotationTolerance( def.m_rotationTolerance )
{
	m_minDistanceSq = def.m_minDistance.GetVal( context ); 
	m_minDistanceSq *= m_minDistanceSq;
	m_maxDistanceSq = def.m_maxDistance.GetVal( context ); 
	m_maxDistanceSq *= m_maxDistanceSq;
}


Bool CBehTreeNodeConditionDistanceToCombatTargetInstance::ConditionCheck()
{
	CActor* actor = m_owner->GetActor();
	CActor* target = m_owner->GetCombatTarget().Get();
	if ( target )
	{
		// Rotation
		if( m_checkRotation )
		{
			if( !actor->IsRotatedTowards( target->GetWorldPositionRef(), m_rotationTolerance ) )
			{
				return false;
			}
		}
		// Distance
		Float distanceSq = actor->GetWorldPositionRef().DistanceSquaredTo( target->GetWorldPositionRef() );
		if( distanceSq >= m_minDistanceSq && distanceSq <= m_maxDistanceSq )
		{
			return true;
		}
	}

	return false;
}

////////////////////////////////////////////////////////////////////////
// CBehTreeNodeConditionDistanceToActionTargetInstance
////////////////////////////////////////////////////////////////////////
CBehTreeNodeConditionDistanceToActionTargetInstance::CBehTreeNodeConditionDistanceToActionTargetInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: CBehTreeNodeConditionInstance( def, owner, context, parent )
	, m_checkRotation( def.m_checkRotation )
	, m_rotationTolerance( def.m_rotationTolerance )
{
	m_minDistanceSq = def.m_minDistance.GetVal( context ); 
	m_minDistanceSq *= m_minDistanceSq;
	m_maxDistanceSq = def.m_maxDistance.GetVal( context ); 
	m_maxDistanceSq *= m_maxDistanceSq;
}


Bool CBehTreeNodeConditionDistanceToActionTargetInstance::ConditionCheck()
{
	CActor* actor = m_owner->GetActor();
	CNode* target = m_owner->GetActionTarget().Get();
	if ( target )
	{
		// Rotation
		if( m_checkRotation )
		{
			if( !actor->IsRotatedTowards( target->GetWorldPositionRef(), m_rotationTolerance ) )
			{
				return false;
			}
		}
		// Distance
		Float distanceSq = actor->GetWorldPositionRef().DistanceSquaredTo( target->GetWorldPositionRef() );
		if( distanceSq >= m_minDistanceSq && distanceSq <= m_maxDistanceSq )
		{
			return true;
		}
	}

	return false;
}
/////////////////////////////////////
// Named Target 
/////////////////////////////////////
CBehTreeNodeConditionDistanceToNamedTargetInstance::CBehTreeNodeConditionDistanceToNamedTargetInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: CBehTreeNodeConditionInstance( def, owner, context, parent )
	, m_checkRotation( def.m_checkRotation )
	, m_rotationTolerance( def.m_rotationTolerance )
	, m_targetName( def.m_targetName.GetVal( context ) )
{
	m_minDistanceSq = def.m_minDistance.GetVal( context ); 
	m_minDistanceSq *= m_minDistanceSq;
	m_maxDistanceSq = def.m_maxDistance.GetVal( context ); 
	m_maxDistanceSq *= m_maxDistanceSq;
}

String CBehTreeNodeConditionDistanceToNamedTargetDefinition::GetNodeCaption() const
{
	String baseCaption = TBaseClass::GetNodeCaption();
	return baseCaption + TXT(" ( ") + m_targetName.GetValue().AsString() + TXT(" )");
}

Bool CBehTreeNodeConditionDistanceToNamedTargetInstance::ConditionCheck()
{
	CActor* actor = m_owner->GetActor();
	CNode* target = m_owner->GetNamedTarget( m_targetName ).Get();
	if ( target )
	{
		// Rotation
		if( m_checkRotation )
		{
			if( !actor->IsRotatedTowards( target->GetWorldPositionRef(), m_rotationTolerance ) )
			{
				return false;
			}
		}
		// Distance
		Float distanceSq = actor->GetWorldPositionRef().DistanceSquaredTo( target->GetWorldPositionRef() );
		if( distanceSq >= m_minDistanceSq && distanceSq <= m_maxDistanceSq )
		{
			return true;
		}
	}

	return false;
}

// end of Name Target

////////////////////////////////////////////////////////////////////////
// CBehTreeNodeConditionDistanceToCustomTargetInstance
////////////////////////////////////////////////////////////////////////
CBehTreeNodeConditionDistanceToCustomTargetInstance::CBehTreeNodeConditionDistanceToCustomTargetInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: Super( def, owner, context, parent )
	, m_checkRotation( def.m_checkRotation )
	, m_rotationTolerance( def.m_rotationTolerance )
	, m_customTarget( owner )
{
	m_minDistanceSq = def.m_minDistance.GetVal( context ); 
	m_minDistanceSq *= m_minDistanceSq;
	m_maxDistanceSq = def.m_maxDistance.GetVal( context ); 
	m_maxDistanceSq *= m_maxDistanceSq;
}

Bool CBehTreeNodeConditionDistanceToCustomTargetInstance::ConditionCheck()
{
	CBehTreeCustomMoveData* data = m_customTarget.Get();
	CActor* actor = m_owner->GetActor();

	// Rotation
	if( m_checkRotation )
	{
		if( !actor->IsRotatedTowards( data->GetTarget(), m_rotationTolerance ) )
		{
			return false;
		}
	}
	// Distance
	Float distanceSq = actor->GetWorldPositionRef().DistanceSquaredTo( data->GetTarget() );
	return ( distanceSq >= m_minDistanceSq && distanceSq <= m_maxDistanceSq );
}


////////////////////////////////////////////////////////////////////////
// CBehTreeNodeConditionDistanceToActionTargetInstance
////////////////////////////////////////////////////////////////////////
CBehTreeNodeConditionDistanceToTaggedInstance::CBehTreeNodeConditionDistanceToTaggedInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: CBehTreeNodeConditionInstance( def, owner, context, parent )
	, m_checkRotation( def.m_checkRotation )
	, m_rotationTolerance( def.m_rotationTolerance )
	, m_tag( def.m_tag.GetVal( context ) )
	, m_allowActivationWhenNoTarget( def.m_allowActivationWhenNoTarget )
{
	m_minDistanceSq = def.m_minDistance.GetVal( context ); 
	m_minDistanceSq *= m_minDistanceSq;
	m_maxDistanceSq = def.m_maxDistance.GetVal( context ); 
	m_maxDistanceSq *= m_maxDistanceSq;
}

CNode* CBehTreeNodeConditionDistanceToTaggedInstance::GetTarget()
{
	if ( !m_tag.Empty() )
	{
		CTagManager* tagMgr = GGame->GetActiveWorld()->GetTagManager();
		CNode* target = tagMgr->GetTaggedNode( m_tag );
		return target;
	}
	return nullptr;
}

Bool CBehTreeNodeConditionDistanceToTaggedInstance::ConditionCheck()
{
	CActor* actor = m_owner->GetActor();
	CNode* target = GetTarget();
	if ( target )
	{
		// Rotation
		if( m_checkRotation )
		{
			if( !actor->IsRotatedTowards( target->GetWorldPositionRef(), m_rotationTolerance ) )
			{
				return false;
			}
		}
		// Distance
		Float distanceSq = actor->GetWorldPositionRef().DistanceSquaredTo( target->GetWorldPositionRef() );
		if( distanceSq >= m_minDistanceSq && distanceSq <= m_maxDistanceSq )
		{
			return true;
		}
	}

	return m_allowActivationWhenNoTarget;
}