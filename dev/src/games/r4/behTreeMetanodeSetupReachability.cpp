/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behTreeMetanodeSetupReachability.h"

#include "r4BehTreeInstance.h"


void CBehTreeMetanodeSetupCombatReachability::RunOnSpawn( CBehTreeSpawnContext& context, CBehTreeInstance* owner ) const
{
	CR4BehTreeInstance* ai = CR4BehTreeInstance::Get( owner );

	ai->SetToleranceDistanceForCombat( m_reachabilityTolerance.GetVal( context ) );
}

