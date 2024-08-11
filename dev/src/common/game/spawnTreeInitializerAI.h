#pragma once

#include "spawnTreeInitializer.h"


class ISpawnTreeInitializerAI : public ISpawnTreeInitializer
{
	DECLARE_ENGINE_ABSTRACT_CLASS( ISpawnTreeInitializerAI, ISpawnTreeInitializer );

protected:
	mutable THandle< IAIParameters >	m_lazyAI;											// lazy cached ai property value
	mutable THandle< IAITree >			m_lazyTree;											// lazy cached tree from property valuer
	mutable Bool						m_cached;
	CName								m_dynamicTreeParameterName;

	void CacheStuff() const;

public:
	ISpawnTreeInitializerAI()
		: m_cached( false )															{}
	~ISpawnTreeInitializerAI();

	EOutput					Activate( CActor* actor, CSpawnTreeInstance* instance, CBaseCreatureEntry* entry, EActivationReason reason ) const override;
	void					Deactivate( CActor* actor, CSpawnTreeInstance* instance, CBaseCreatureEntry* entry ) const override;
	void					OnCreatureSpawn( EntitySpawnInfo& entityInfo, CSpawnTreeInstance* instance, CBaseCreatureEntry* entry ) const override;

	Bool					CallActivateOnRestore() const override;
	Bool					CallActivateOnSpawn() const override;
	Bool					CallActivateOnPoolSpawn() const override;
	Bool					CallActivateWhenStealing() const override;

	Bool					IsConflicting( const ISpawnTreeInitializer* initializer ) const override;

	virtual CName			GetDynamicNodeEventName() const = 0;

	String					GetEditorFriendlyName() const override;
	String					GetBitmapName() const override;

#ifndef NO_EDITOR
	void					OnCreatedInEditor() override;
#endif
};

BEGIN_ABSTRACT_CLASS_RTTI( ISpawnTreeInitializerAI );
	PARENT_CLASS( ISpawnTreeInitializer );
	PROPERTY( m_dynamicTreeParameterName );													// set up on script level
END_CLASS_RTTI();
