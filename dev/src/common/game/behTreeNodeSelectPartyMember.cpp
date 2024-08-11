/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behTreeNodeSelectPartyMember.h"

#include "behTreeInstance.h"
#include "encounter.h"
#include "encounterCreaturePool.h"


////////////////////////////////////////////////////////////////////////
// CBehTreeNodeSelectPartyMemberDefinition
////////////////////////////////////////////////////////////////////////

IBehTreeNodeDecoratorInstance* CBehTreeNodeSelectPartyMemberDefinition::SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const
{
	return new Instance( *this, owner, context, parent );
}


////////////////////////////////////////////////////////////////////////
// CBehTreeNodeSelectPartyMemberInstance
////////////////////////////////////////////////////////////////////////
CBehTreeNodeSelectPartyMemberInstance::CBehTreeNodeSelectPartyMemberInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: Super( def, owner, context, parent )
	, m_partyMemberName( def.m_partyMemberName.GetVal( context ) )
{
	m_encounter = context.GetVal< CEncounter* >( CNAME( encounter ), NULL );
}

Bool CBehTreeNodeSelectPartyMemberInstance::Activate()
{
	CEncounter* encounter = m_encounter.Get();
	if ( !encounter )
	{
		DebugNotifyActivationFail();
		return false;
	}

	CActor* partyMember = encounter->GetCreaturePool().GetCreaturePartyMember( m_owner->GetActor(), m_partyMemberName );
	if ( !partyMember )
	{
		DebugNotifyActivationFail();
		return false;
	}

	// set target BEFORE children activation
	m_owner->SetActionTarget( partyMember );
	if ( !Super::Activate() )
	{
		m_owner->SetActionTarget( NULL );
		DebugNotifyActivationFail();
		return false;
	}

	return true;
}
void CBehTreeNodeSelectPartyMemberInstance::Deactivate()
{
	Super::Deactivate();
	// clear target AFTER children deactivation
	m_owner->SetActionTarget( NULL );
}

