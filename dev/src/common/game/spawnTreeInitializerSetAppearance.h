#pragma once

#include "spawnTreeInitializer.h"

class CSpawnTreeInitializerSetAppearance : public ISpawnTreeInitializer
#ifndef NO_EDITOR
	, public IEntityTemplatePropertyOwner
#endif
{
	DECLARE_ENGINE_CLASS( CSpawnTreeInitializerSetAppearance, ISpawnTreeInitializer, 0 );
protected:
	CName				m_appearanceName;
	Bool				m_onlySetOnSpawnAppearance;

public:
	CSpawnTreeInitializerSetAppearance()
		: m_onlySetOnSpawnAppearance( false )													{}

	Bool							Accept( CActor* actor ) const override;
	void							OnCreatureSpawn( EntitySpawnInfo& entityInfo, CSpawnTreeInstance* instance, CBaseCreatureEntry* entry ) const override;
	EOutput							Activate( CActor* actor, CSpawnTreeInstance* instance, CBaseCreatureEntry* entry, EActivationReason reason ) const override;
	void							Deactivate( CActor* actor, CSpawnTreeInstance* instance, CBaseCreatureEntry* entry ) const override;
	String							GetBlockCaption() const override;
	String							GetEditorFriendlyName() const override;

	Bool							CallActivateOnRestore() const;
	Bool							CallActivateOnSpawn() const;
	Bool							CallActivateOnPoolSpawn() const;
	Bool							CallActivateWhenStealing() const;

#ifndef NO_EDITOR
	// IEntityTemplatePropertyOwner interface
	CEntityTemplate*				Editor_GetEntityTemplate()	override;
#endif

};

BEGIN_CLASS_RTTI( CSpawnTreeInitializerSetAppearance );
	PARENT_CLASS( ISpawnTreeInitializer );
	PROPERTY_CUSTOM_EDIT( m_appearanceName, TXT("Name of the appearance to be applied"), TXT("EntityAppearanceSelect") );
	PROPERTY_EDIT( m_onlySetOnSpawnAppearance, TXT("Guys spawned with this initializer will have proper appearance setup, and only guys with such appearance will be accepted by this initializer in runtime.") );
END_CLASS_RTTI();
