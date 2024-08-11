/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behTreeMetanodeGuardSetup.h"

#include "aiSpawnTreeParameters.h"
#include "behTreeGuardAreaData.h"
#include "behTreeInstance.h"

void CBehTreeMetanodeSetupGuardDefinition::RunOnSpawn( CBehTreeSpawnContext& context, CBehTreeInstance* owner ) const
{
	CBehTreeGuardAreaData* data = CBehTreeGuardAreaData::Find( owner );
	if ( !data )
	{
		return;
	}

	Float pursuitRange = 15.f;
	CAreaComponent* guardArea = CGuardAreaParameters::GetDefaultGuardArea( context );
	CAreaComponent* pursuitArea = nullptr;

	if ( guardArea )
	{
		pursuitArea = CGuardAreaParameters::GetDefaultPursuitArea( context );
		CGuardAreaParameters::GetDefaultPursuitRange( context, pursuitRange );
	}

	data->SetupBaseState( guardArea, pursuitArea, pursuitRange );
}