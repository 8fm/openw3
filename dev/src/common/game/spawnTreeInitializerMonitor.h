/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once


class ISpawnTreeSpawnMonitorBaseInitializer : public ISpawnTreeInitializer
{
	DECLARE_ENGINE_ABSTRACT_CLASS( ISpawnTreeSpawnMonitorBaseInitializer, ISpawnTreeInitializer );
protected:
	enum ESpawnMonitorEvent
	{
		ESME_Killed,
		ESME_Lost,
		ESME_Spawned,
	};

	virtual void					MonitorCreature( CSpawnTreeInstance& instance, CActor* actor, CBaseCreatureEntry* entry, ESpawnMonitorEvent eventType ) const = 0;
public:
	EOutput							Activate( CActor* actor, CSpawnTreeInstance* instance, CBaseCreatureEntry* entry, EActivationReason reason ) const override;
	void							Deactivate( CActor* actor, CSpawnTreeInstance* instance, CBaseCreatureEntry* entry ) const override;

	void							OnCreatureRemoval( CSpawnTreeInstance& instance, CActor* actor, ESpawnTreeCreatureRemovalReason removalReason, CBaseCreatureEntry* entry ) const override;

	Bool							IsSpawnable() const override;
	Bool							IsSpawnableOnPartyMembers() const override;

	String							GetEditorFriendlyName() const override;
};

BEGIN_ABSTRACT_CLASS_RTTI( ISpawnTreeSpawnMonitorBaseInitializer )
	PARENT_CLASS( ISpawnTreeInitializer )
END_CLASS_RTTI()

class ISpawnTreeSpawnMonitorInitializer : public ISpawnTreeSpawnMonitorBaseInitializer
{
	DECLARE_ENGINE_CLASS( ISpawnTreeSpawnMonitorInitializer, ISpawnTreeSpawnMonitorBaseInitializer, 0 );
protected:
	mutable CSpawnTreeInstance*		m_contextInstance;
	mutable CBaseCreatureEntry*		m_contextEntry;


	void							MonitorCreature( CSpawnTreeInstance& instance, CActor* actor, CBaseCreatureEntry* entry, ESpawnMonitorEvent eventType ) const override;

public:
	void							OnFullRespawn( CSpawnTreeInstance& instance ) const override;

	Bool							IsSpawnable() const override;
	Bool							IsConflicting( const ISpawnTreeInitializer* initializer ) const override;

	String							GetEditorFriendlyName() const override;
private:
	void							funcGetNumCreaturesSpawned( CScriptStackFrame& stack, void* result );
	void							funcGetNumCreaturesToSpawn( CScriptStackFrame& stack, void* result );
	void							funcGetNumCreaturesDead( CScriptStackFrame& stack, void* result );
};

BEGIN_CLASS_RTTI( ISpawnTreeSpawnMonitorInitializer )
	PARENT_CLASS( ISpawnTreeSpawnMonitorBaseInitializer )
	NATIVE_FUNCTION( "GetNumCreaturesSpawned", funcGetNumCreaturesSpawned );
	NATIVE_FUNCTION( "GetNumCreaturesToSpawn", funcGetNumCreaturesToSpawn );
	NATIVE_FUNCTION( "GetNumCreaturesDead", funcGetNumCreaturesDead );
END_CLASS_RTTI()
