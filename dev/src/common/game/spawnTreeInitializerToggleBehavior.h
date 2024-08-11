/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "spawnTreeInitializer.h"


class ISpawnTreeInitializerToggleBehavior : public ISpawnTreeInitializer
{
	DECLARE_ENGINE_CLASS( ISpawnTreeInitializerToggleBehavior, ISpawnTreeInitializer, 0 );
protected:
	CName					m_behaviorSwitchName;
	Bool					m_enableBehavior;

public:
	ISpawnTreeInitializerToggleBehavior()
		: m_enableBehavior( false )															{}

	EOutput					Activate( CActor* actor, CSpawnTreeInstance* instance, CBaseCreatureEntry* entry, EActivationReason reason ) const override;
	void					Deactivate( CActor* actor, CSpawnTreeInstance* instance, CBaseCreatureEntry* entry ) const override;

	String					GetEditorFriendlyName() const override;	

	Bool					CallActivateOnRestore() const override;
	Bool					CallActivateOnSpawn() const override;
	Bool					CallActivateOnPoolSpawn() const override;
	Bool					CallActivateWhenStealing() const override;

	Bool					IsSpawnable() const override;
};

BEGIN_CLASS_RTTI( ISpawnTreeInitializerToggleBehavior )
	PARENT_CLASS( ISpawnTreeInitializer )
	PROPERTY( m_behaviorSwitchName )
	PROPERTY_EDIT( m_enableBehavior, TXT("Enable/disable") )
END_CLASS_RTTI()