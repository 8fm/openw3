#pragma once

#include "spawnTreeInitializer.h"

class CSpawnTreeInitializerAddTag : public ISpawnTreeInitializer
#ifndef NO_EDITOR
	, public IEntityTemplatePropertyOwner
#endif
{
	DECLARE_ENGINE_CLASS( CSpawnTreeInitializerAddTag, ISpawnTreeInitializer, 0 );
protected:
	TagList				m_tag;	
	Bool				m_onlySetOnSpawnAppearance;
public:
	CSpawnTreeInitializerAddTag(){}
	
	void							OnCreatureSpawn( EntitySpawnInfo& entityInfo, CSpawnTreeInstance* instance, CBaseCreatureEntry* entry ) const override;
	String							GetBlockCaption() const override;
	String							GetEditorFriendlyName() const override;

	Bool							Accept( CActor* actor ) const override;
	EOutput							Activate( CActor* actor, CSpawnTreeInstance* instance, CBaseCreatureEntry* entry, EActivationReason reason ) const override;
	void							Deactivate( CActor* actor, CSpawnTreeInstance* instance, CBaseCreatureEntry* entry ) const override;
	Bool							CallActivateOnSpawn() const override;

	Bool							OnPropertyTypeMismatch( CName propertyName, IProperty* existingProperty, const CVariant& readValue ) override;

#ifndef NO_EDITOR
	CEntityTemplate*				Editor_GetEntityTemplate()	override;
#endif
private:
	void							funcAddTag( CScriptStackFrame& stack, void* result );
};

BEGIN_CLASS_RTTI( CSpawnTreeInitializerAddTag );
	PARENT_CLASS( ISpawnTreeInitializer );
	PROPERTY_EDIT( m_tag, TXT("Tag") );
	PROPERTY_EDIT( m_onlySetOnSpawnAppearance, TXT("Guys spawned with this initializer will have proper appearance setup, and only guys with such appearance will be accepted by this initializer in runtime.") );
	NATIVE_FUNCTION( "AddTag", funcAddTag );
END_CLASS_RTTI();
