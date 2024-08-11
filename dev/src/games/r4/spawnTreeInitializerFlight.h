/*
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "../../common/game/spawnTreeInitializerAI.h"

class CSpawnTreeInitializerIdleFlightAI : public ISpawnTreeInitializerAI
{
	DECLARE_ENGINE_CLASS( CSpawnTreeInitializerIdleFlightAI, ISpawnTreeInitializerAI, 0 );

public:
	CName			GetDynamicNodeEventName() const override;

	String			GetEditorFriendlyName() const override;
};

BEGIN_CLASS_RTTI( CSpawnTreeInitializerIdleFlightAI );
	PARENT_CLASS( ISpawnTreeInitializerAI );
END_CLASS_RTTI();