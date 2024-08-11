/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "spawnTreeInitializerAI.h"

class CSpawnTreeInitializerBaseStartingBehavior : public ISpawnTreeInitializerAI
{
	DECLARE_ENGINE_ABSTRACT_CLASS( CSpawnTreeInitializerBaseStartingBehavior, ISpawnTreeInitializerAI );

protected:
	Bool					m_runBehaviorOnSpawn;
	Bool					m_runBehaviorOnActivation;
	Bool					m_runBehaviorOnLoading;
	ETopLevelAIPriorities	m_actionPriority;

public:
	CSpawnTreeInitializerBaseStartingBehavior()
		: m_runBehaviorOnSpawn( true )
		, m_runBehaviorOnActivation( false )
		, m_runBehaviorOnLoading( false )
		, m_actionPriority( AIP_AboveCombat )										 {}

	Bool			CallActivateOnRestore() const override;
	Bool			CallActivateOnSpawn() const override;
	Bool			CallActivateOnPoolSpawn() const override;
	Bool			CallActivateWhenStealing() const override;

	EOutput			Activate( CActor* actor, CSpawnTreeInstance* instance, CBaseCreatureEntry* entry, EActivationReason reason ) const override;
	void			OnCreatureSpawn( EntitySpawnInfo& entityInfo, CSpawnTreeInstance* instance, CBaseCreatureEntry* entry ) const override;
};

BEGIN_ABSTRACT_CLASS_RTTI( CSpawnTreeInitializerBaseStartingBehavior )
	PARENT_CLASS( ISpawnTreeInitializerAI )
	PROPERTY_EDIT( m_runBehaviorOnSpawn, TXT("Run this behavior when creatures is spawned") )
	PROPERTY_EDIT( m_runBehaviorOnActivation, TXT("Run this behavior when creatures activates this entry") )
	PROPERTY_EDIT( m_runBehaviorOnLoading, TXT("Run this behavior when creatures are being loaded from the save") )
	PROPERTY_EDIT( m_actionPriority, TXT("Forced behavior priority") )
END_CLASS_RTTI()

class CSpawnTreeInitializerStartingBehavior : public CSpawnTreeInitializerBaseStartingBehavior
{
	DECLARE_ENGINE_CLASS( CSpawnTreeInitializerStartingBehavior, CSpawnTreeInitializerBaseStartingBehavior, 0 );

protected:

public:
	CSpawnTreeInitializerStartingBehavior()								 {}

	String			GetEditorFriendlyName() const override;

	CName GetDynamicNodeEventName() const;
};

BEGIN_CLASS_RTTI( CSpawnTreeInitializerStartingBehavior )
	PARENT_CLASS( CSpawnTreeInitializerBaseStartingBehavior )
END_CLASS_RTTI()
