/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behTreeNodeSelectFormationLeader.h"

#include "behTreeInstance.h"
#include "encounter.h"
#include "encounterCreaturePool.h"
#include "formation.h"

BEHTREE_STANDARD_SPAWNDECORATOR_FUNCTION( CBehTreeNodeSelectFormationLeaderDefinition )

////////////////////////////////////////////////////////////////////////
// CBehTreeNodeSelectFormationLeaderInstance
////////////////////////////////////////////////////////////////////////
CBehTreeNodeSelectFormationLeaderInstance::CBehTreeNodeSelectFormationLeaderInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: Super( def, owner, context, parent )
	, m_runtimeData( owner )
	, m_formation( def.m_formation.GetVal( context ) )
{
	if ( m_formation )
	{
		m_owner->AddDependentObject( m_formation );
	}
}
void CBehTreeNodeSelectFormationLeaderInstance::OnDestruction()
{
	if ( m_formation )
	{
		m_owner->RemoveDependentObject( m_formation );
	}

	Super::OnDestruction();
}


Bool CBehTreeNodeSelectFormationLeaderInstance::TestCondition()
{
	CAIFormationData* formationData = m_runtimeData.Get();
	if ( m_isActive )
	{
		CFormationLeaderData* leaderData = formationData->LeaderData();
		if ( leaderData && leaderData->GetLeader() )
		{
			return true;
		}
	}
	CEncounter* encounter = m_encounter.Get();
	if ( !encounter || !m_formation )
	{
		return false;
	}
	CEncounterCreaturePool::PartyIterator partyIterator;
	if ( !encounter->GetCreaturePool().GetPartyIterator( m_owner->GetActor(), partyIterator ) )
	{
		// no party no fun
		return false;
	}

	CActor* actor = m_owner->GetActor();

	while ( partyIterator )
	{
		if ( m_partyMemberName.Empty() ||  partyIterator.Name() == m_partyMemberName )
		{
			// filter out self
			if ( partyIterator.Actor() != actor )
			{
				// check if guy is a leader
				if ( formationData->Test( m_formation, partyIterator.Actor() ) )
				{
					return true;
				}
			}
			
		}
		partyIterator.Next();
	}
	return false;
}
Bool CBehTreeNodeSelectFormationLeaderInstance::Activate()
{
	CEncounter* encounter = m_encounter.Get();
	if ( !encounter || !m_formation )
	{
		DebugNotifyActivationFail();
		return false;
	}

	CEncounterCreaturePool::PartyIterator partyIterator;
	if ( !encounter->GetCreaturePool().GetPartyIterator( m_owner->GetActor(), partyIterator ) )
	{
		// no party no fun
		DebugNotifyActivationFail();
		return false;
	}

	CActor* actor = m_owner->GetActor();
	CAIFormationData* formationData = m_runtimeData.Get();
	
	while ( partyIterator )
	{
		if ( m_partyMemberName.Empty() || partyIterator.Name() == m_partyMemberName )
		{
			// filter out self
			if ( partyIterator.Actor() != actor )
			{
				// check if guy is a leader
				if ( formationData->Setup( m_formation, partyIterator.Actor(), actor, false ) )
				{
					break;
				}
			}
		}
		partyIterator.Next();
	}

	if ( !formationData->IsSetup() )
	{
		DebugNotifyActivationFail();
		return false;
	}

	m_owner->SetActionTarget( formationData->LeaderData()->GetLeader() );

	// NOTICE: ommit superclass Activate() implementation
	return IBehTreeNodeDecoratorInstance::Activate();
}
Bool CBehTreeNodeSelectFormationLeaderInstance::IsAvailable()
{
	Bool toRet = TestCondition();
	if( !toRet )
	{
		DebugNotifyAvailableFail();
	}

	return toRet;
}

Int32 CBehTreeNodeSelectFormationLeaderInstance::Evaluate()
{
	if ( TestCondition() )
	{
		if( m_priority <= 0 )
		{
			DebugNotifyAvailableFail();
		}

		return m_priority;
	}

	DebugNotifyAvailableFail();
	return -1;
}