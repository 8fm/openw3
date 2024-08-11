/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/
#pragma once

class CEntityTemplate;

class IEntityTemplateModifier
{
public:
	virtual void EntityTemplateOnPostLoad( CEntityTemplate* entityTemplate ) = 0;
	virtual const CName& EntityTemplateOnGetAppearance( const CEntityTemplate* entityTemplate, const CName& appearanceName ) const { return CName::NONE; };
	virtual void EntityTemplateOnFinalize( CEntityTemplate* entityTemplate ) = 0;
};

///////////////////////////////////////////////////////////

/// Entity template modifier
class CEntityTemplateModifierManager
{
private:
	THashMap< Uint32, TDynArray< IEntityTemplateModifier* > > m_modifiers;

	TDynArray< CEntityTemplate* > m_pendingEntityTemplates;											// List of entity templates waiting to be modified.
	TDynArray< CEntityTemplate* > m_modifiedEntityTemplates;										// List of entity templates that have been modified.

	THashMap< const CEntityTemplate*, TDynArray< Uint32 > > m_modificationCauses;					// Key - entity template that have been modified.
																									// Value - list of path hashes of entity templates that caused key entity template to be modified (each entity
																									// template from this list has some modifier registered - see m_modifiers). For non-cooked entity templates this
																									// list will only contain path hash of key entity template. For cooked entity templates the list may or may not
																									// contain path hash of key entity template, it may also contain path hashes of entity templates included by key
																									// entity template.

	Bool m_active;																					// True - modifier manager is active, entity templates are processed immediately.
																									// False - modifier manager is inactive, entity templates are added to pending list.

	friend class CEntityTemplate;

public:
	CEntityTemplateModifierManager();
	~CEntityTemplateModifierManager();

	Bool RegisteModifier( const String& entityTemplateName, IEntityTemplateModifier* entityTemplateModifier );
	Bool UnregisteModifier( const String& entityTemplateName, IEntityTemplateModifier* entityTemplateModifier );
	Bool IsModifierRegistered( const String& entityTemplatePath ) const;

	void Activate();
	void Deactivate();
	Bool IsActive() const;

private:
	void EntityTemplateOnPostLoad( CEntityTemplate* entityTemplate );
	void EntityTemplateOnFinalize( CEntityTemplate* entityTemplate );
	const CName& EntityTemplateOnGetAppearance( const CEntityTemplate* entityTemplate, const CName& appearanceName ) const;

	Bool AddModification( CEntityTemplate* entityTemplate );
	void RemoveModification( CEntityTemplate* entityTemplate );

	static void GetEntityTemplateIncludesNames( const CEntityTemplate* entityTemplate, TDynArray< Uint32 >& entityTemplatesNameHashes );
};

typedef TSingleton< CEntityTemplateModifierManager, TDefaultLifetime, TCreateUsingNew > SEntityTemplateModifierManager;
