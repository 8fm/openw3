/**
* Copyright © 2011 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "persistentEntity.h"

/// Base class of all saveable entities
class CEffectEntity : public CPeristentEntity
{
	DECLARE_ENGINE_CLASS( CEffectEntity, CPeristentEntity, 0 );

private:
	TDynArray< CName >		m_effects;

public:
	CEffectEntity() {}

	// All components of entity has been attached
	virtual void OnAttachFinished( CWorld* world );

	//! Called when we need to store gameplay state of this entity
	virtual void OnSaveGameplayState( IGameSaver* saver );

	//! Called when we need to restore gameplay state of this entity
	virtual void OnLoadGameplayState( IGameLoader* loader );

	//! Should save?
	virtual Bool CheckShouldSave() const { return true; }
};

BEGIN_CLASS_RTTI( CEffectEntity );
PARENT_CLASS( CPeristentEntity );
END_CLASS_RTTI();