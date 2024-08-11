#include "build.h"

#include "behTreeInstance.h"
#include "behTreeNodeConditionLineofSight.h"


BEHTREE_STANDARD_SPAWNDECORATOR_FUNCTION( CBehTreeNodeConditionLineofSightDefinition )
BEHTREE_STANDARD_SPAWNDECORATOR_FUNCTION( CBehTreeNodeConditionLineofSightToNamedTargetDefinition )

////////////////////////////////////////////////////////////////////////
// CBehTreeNodeConditionInstance
////////////////////////////////////////////////////////////////////////
CBehTreeNodeConditionLineofSightInstance::CBehTreeNodeConditionLineofSightInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: CBehTreeNodeConditionInstance( def, owner, context, parent )
	, m_useCombatTarget( def.m_useCombatTarget.GetVal( context ) )
{}

Bool CBehTreeNodeConditionLineofSightInstance::ConditionCheck()
{
	CActor* actor = m_owner->GetActor();
	CNode* target = m_useCombatTarget ? m_owner->GetCombatTarget().Get() : m_owner->GetActionTarget().Get();

	if ( target )
	{
		Vector targetPos = target->GetWorldPositionRef();		
		CActor* tartegAsActor = Cast< CActor >( target );
		if( tartegAsActor && tartegAsActor->GetHeadBone() != -1 )
		{
			targetPos = tartegAsActor->GetHeadPosition();
		}

		return GGame->GetActiveWorld()->TestLineOfSight( actor->GetHeadPosition(), targetPos , nullptr );
	}

	return false;
}


String CBehTreeNodeConditionLineofSightToNamedTargetDefinition::GetNodeCaption() const
{
	String baseCaption = TBaseClass::GetNodeCaption();
	return baseCaption + TXT(" ( ") + m_targetName.GetValue().AsString() + TXT(" )");
}

Bool CBehTreeNodeConditionLineofSightToNamedTargetInstance::ConditionCheck()
{
	CActor* actor = m_owner->GetActor();
	CNode* target = m_owner->GetNamedTarget( m_targetName ).Get();

	if ( target )
	{
		Vector targetPos = target->GetWorldPositionRef();		
		CActor* tartegAsActor = Cast< CActor >( target );
		if( tartegAsActor && tartegAsActor->GetHeadBone() != -1 )
		{
			targetPos = tartegAsActor->GetHeadPosition();
		}

		return GGame->GetActiveWorld()->TestLineOfSight( actor->GetHeadPosition(), targetPos, nullptr );
	}

	return false;
}