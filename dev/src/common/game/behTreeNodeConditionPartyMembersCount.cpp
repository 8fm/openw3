/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behTreeNodeConditionPartyMembersCount.h"

#include "behTreeInstance.h"
#include "encounter.h"

BEHTREE_STANDARD_SPAWNDECORATOR_FUNCTION( CBehTreeNodeConditionPartyMembersCountDefinition )


String CBehTreeNodeConditionPartyMembersCountDefinition::GetNodeCaption() const
{
	String count =
		m_count.m_varName.Empty()
		? String::Printf( TXT("%d"), m_count.m_value )
		: m_count.m_varName.AsString();
	return String::Printf(
		TXT("PartyMembersCount is %s than %s")
		, m_invertAvailability ? TXT("<=") : TXT(">=")
		, count.AsChar() );
}

////////////////////////////////////////////////////////////////////////
// CBehTreeNodeConditionPartyMembersCountInstance
////////////////////////////////////////////////////////////////////////

CBehTreeNodeConditionPartyMembersCountInstance::CBehTreeNodeConditionPartyMembersCountInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: Super( def, owner, context, parent )
	, m_partyMemberName( def.m_partyMemberName.GetVal( context ) )
	, m_count( def.m_count.GetVal( context ) )
{
	m_encounter = context.GetVal< CEncounter* >( CNAME( encounter ), NULL );
}

Bool CBehTreeNodeConditionPartyMembersCountInstance::ConditionCheck()
{
	Uint32 partySize = 0;
	CEncounter* encounter = m_encounter.Get();
	if ( encounter )
	{
		CActor* actor = m_owner->GetActor();
		CEncounterCreaturePool& pool = encounter->GetCreaturePool();

		if ( m_partyMemberName.Empty() )
		{
			partySize = pool.GetCreaturePartySize( actor );
		}
		else
		{
			CEncounterCreaturePool::PartyIterator partyIterator;
			if ( encounter->GetCreaturePool().GetPartyIterator( m_owner->GetActor(), partyIterator ) )
			{
				while ( partyIterator )
				{
					if ( m_partyMemberName == partyIterator.Name() )
					{
						++partySize;
					}
					partyIterator.Next();
				}
			}
		}
	}

	

	return partySize >= m_count;
}