/*
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "spawnTreeInitializer.h"

class CSpawnTreeInitializerPersistent : public ISpawnTreeInitializer
{
	DECLARE_ENGINE_CLASS( CSpawnTreeInitializerPersistent, ISpawnTreeInitializer, 0 );

public:
	Bool					Accept( CActor* actor ) const override;
	void					OnCreatureSpawn( EntitySpawnInfo& entityInfo, CSpawnTreeInstance* instance, CBaseCreatureEntry* creatureEntry ) const override;
	String					GetEditorFriendlyName() const override;
};

BEGIN_CLASS_RTTI( CSpawnTreeInitializerPersistent )
	PARENT_CLASS( ISpawnTreeInitializer )
END_CLASS_RTTI()