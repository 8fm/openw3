#pragma once

#include "../../common/game/journalBase.h"

//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------

enum EJournalVisibilityAction
{
	JVA_Nothing,
	JVA_Show,
	JVA_Hide,
};

BEGIN_ENUM_RTTI( EJournalVisibilityAction );
	ENUM_OPTION( JVA_Nothing );
	ENUM_OPTION( JVA_Show );
	ENUM_OPTION( JVA_Hide );
END_ENUM_RTTI();

class CJournalCharacterDescription : public CJournalContainerEntry
{
	DECLARE_ENGINE_CLASS( CJournalCharacterDescription, CJournalContainerEntry, 0 )

public:
	CJournalCharacterDescription();
	virtual ~CJournalCharacterDescription();
	virtual Bool IsParentClass( CJournalBase* other ) const;

	RED_INLINE EJournalVisibilityAction GetAction() { return m_action; }

	void funcGetDescriptionStringId( CScriptStackFrame& stack, void* result );
private:
	LocalizedString m_description;
	EJournalVisibilityAction m_action;

};

BEGIN_CLASS_RTTI( CJournalCharacterDescription )
	PARENT_CLASS( CJournalContainerEntry )
	NATIVE_FUNCTION( "GetDescriptionStringId",  funcGetDescriptionStringId )
	PROPERTY_EDIT( m_description, TXT( "Description" ) )
	PROPERTY_EDIT( m_action, TXT( "Entity template action" ) )
	PROPERTY_EDIT_NOSERIALIZE( m_active, TXT( "In journal at beginning" ) )
END_CLASS_RTTI()

//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------

enum ECharacterImportance
{
	CI_Main = 0,
	CI_Side,

	CI_Max
};

BEGIN_ENUM_RTTI( ECharacterImportance )
	ENUM_OPTION_DESC( TXT( "Main Character" ), CI_Main )
	ENUM_OPTION_DESC( TXT( "Side Character" ), CI_Side )
END_ENUM_RTTI();

class CJournalCharacter : public CJournalContainer
{
	DECLARE_ENGINE_CLASS( CJournalCharacter, CJournalContainer, 0 )

public:
	CJournalCharacter();
	virtual ~CJournalCharacter();
	virtual Bool IsParentClass( CJournalBase* other ) const;

	void funcGetNameStringId( CScriptStackFrame& stack, void* result );
	void funcGetImagePath( CScriptStackFrame& stack, void* result );
	void funcGetCharacterImportance( CScriptStackFrame& stack, void* result );
	void funcGetEntityTemplateFilename( CScriptStackFrame& stack, void* result );

private:
	LocalizedString m_name;
	String m_image;
	ECharacterImportance m_importance;
	TSoftHandle< CEntityTemplate > m_entityTemplate;
};

BEGIN_CLASS_RTTI( CJournalCharacter )
	PARENT_CLASS( CJournalContainer )
	PROPERTY_EDIT( m_name, TXT( "Name" ) )
	PROPERTY_EDIT( m_image, TXT( "Image" ) )
	PROPERTY_EDIT( m_importance, TXT( "Importance" ) )
	PROPERTY_EDIT( m_entityTemplate, TXT( "Entity template" ) )
	PROPERTY_EDIT_NOSERIALIZE( m_active, TXT( "In journal at beginning" ) )
	NATIVE_FUNCTION( "GetNameStringId",  funcGetNameStringId )
	NATIVE_FUNCTION( "GetImagePath",  funcGetImagePath )
	NATIVE_FUNCTION( "GetCharacterImportance",  funcGetCharacterImportance )
	NATIVE_FUNCTION( "GetEntityTemplateFilename",  funcGetEntityTemplateFilename )
END_CLASS_RTTI()

//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------

class CJournalCharacterGroup : public CJournalBase
{
	DECLARE_ENGINE_CLASS( CJournalCharacterGroup, CJournalBase, 0 )

public:
	CJournalCharacterGroup();
	virtual ~CJournalCharacterGroup();
	virtual Bool IsParentClass( CJournalBase* other ) const;
};

BEGIN_CLASS_RTTI( CJournalCharacterGroup )
	PARENT_CLASS( CJournalBase )
END_CLASS_RTTI()

//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------

class CJournalCharacterRoot : public CJournalBase
{
public:
	DECLARE_ENGINE_CLASS( CJournalCharacterRoot, CJournalBase, 0 )

	CJournalCharacterRoot();
	virtual ~CJournalCharacterRoot();

	virtual Bool IsParentClass( CJournalBase* other ) const;
};

BEGIN_CLASS_RTTI( CJournalCharacterRoot )
	PARENT_CLASS( CJournalBase )
END_CLASS_RTTI()
