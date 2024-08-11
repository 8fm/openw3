/*
* Copyright © 2016 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behTreeNodeConditionHasAbility.h"

#include "../../common/game/characterStats.h"

BEHTREE_STANDARD_SPAWNDECORATOR_FUNCTION( CBehTreeNodeConditionHasAbilityDefinition )

String CBehTreeNodeConditionHasAbilityDefinition::GetNodeCaption() const
{
	String baseCaption = String::Printf( TXT("HasAbility %s"), m_abilityName.AsChar() );
	return DecorateCaption( baseCaption );
}

Bool CBehTreeNodeConditionHasAbilityInstance::ConditionCheck()
{
	CCharacterStats* characterStats = m_owner->GetActor()->GetCharacterStats();
	if ( !characterStats )
	{
		return false;
	}
	return characterStats->HasAbility( m_abilityName );

}