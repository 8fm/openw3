/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "../../common/game/behTreeMetanodeOnSpawn.h"


class CBehTreeMetanodeSetupCombatReachability : public IBehTreeMetanodeOnSpawnDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeMetanodeSetupCombatReachability, IBehTreeMetanodeOnSpawnDefinition, IBehTreeNodeInstance, SetupCombatTargeting );
protected:
	CBehTreeValFloat				m_reachabilityTolerance;

	void							RunOnSpawn( CBehTreeSpawnContext& context, CBehTreeInstance* owner ) const override;
public:
	CBehTreeMetanodeSetupCombatReachability()
		: m_reachabilityTolerance( 4.f )												{}
	
};


BEGIN_CLASS_RTTI( CBehTreeMetanodeSetupCombatReachability );
	PARENT_CLASS( IBehTreeMetanodeOnSpawnDefinition );
	PROPERTY_EDIT( m_reachabilityTolerance, TXT("Distance tolerance at which we can consider ourselves threat to the enemy") );
END_CLASS_RTTI();