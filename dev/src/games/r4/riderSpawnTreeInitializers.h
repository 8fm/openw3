#pragma once

#include "../../common/game/spawnTreeInitializerAI.h"
#include "../../common/game/spawnTreeInitializerStartingBehavior.h"

class CSpawnTreeInitializerRiderIdleAI : public ISpawnTreeInitializerAI
{
	DECLARE_ENGINE_CLASS( CSpawnTreeInitializerRiderIdleAI, ISpawnTreeInitializerAI, 0 );

public:
	CName			GetDynamicNodeEventName() const override;

	String			GetEditorFriendlyName() const override;
};

BEGIN_CLASS_RTTI( CSpawnTreeInitializerRiderIdleAI );
	PARENT_CLASS( ISpawnTreeInitializerAI );
END_CLASS_RTTI();

class CSpawnTreeInitializerRiderStartingBehavior : public CSpawnTreeInitializerBaseStartingBehavior
{
	DECLARE_ENGINE_CLASS( CSpawnTreeInitializerRiderStartingBehavior, CSpawnTreeInitializerBaseStartingBehavior, 0 );

protected:
	
public:
	CSpawnTreeInitializerRiderStartingBehavior()								 {}

	String			GetEditorFriendlyName() const override;
	CName			GetDynamicNodeEventName() const;
};

BEGIN_CLASS_RTTI( CSpawnTreeInitializerRiderStartingBehavior )
	PARENT_CLASS( CSpawnTreeInitializerBaseStartingBehavior )
END_CLASS_RTTI()