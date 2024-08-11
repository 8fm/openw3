#pragma once

#include "../../common/game/journalBase.h"

//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------

class CJournalGlossaryDescription : public CJournalContainerEntry
{
	DECLARE_ENGINE_CLASS( CJournalGlossaryDescription, CJournalContainerEntry, 0 )

public:
	CJournalGlossaryDescription();
	virtual ~CJournalGlossaryDescription();
	virtual Bool IsParentClass( CJournalBase* other ) const;
	void funcGetDescriptionStringId( CScriptStackFrame& stack, void* result );
private:
	LocalizedString m_description;
};

BEGIN_CLASS_RTTI( CJournalGlossaryDescription )
	PARENT_CLASS( CJournalContainerEntry )
	NATIVE_FUNCTION( "GetDescriptionStringId",  funcGetDescriptionStringId )
	PROPERTY_EDIT( m_description, TXT( "Description" ) )
	PROPERTY_EDIT_NOSERIALIZE( m_active, TXT( "In journal at beginning" ) )
END_CLASS_RTTI()

//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------

class CJournalGlossary : public CJournalContainer
{
	DECLARE_ENGINE_CLASS( CJournalGlossary, CJournalContainer, 0 )

public:
	CJournalGlossary();
	virtual ~CJournalGlossary();
	virtual Bool IsParentClass( CJournalBase* other ) const;
	void funcGetTitleStringId( CScriptStackFrame& stack, void* result );
	void funcGetImagePath( CScriptStackFrame& stack, void* result );
private:
	LocalizedString m_title;
	String m_image;
};

BEGIN_CLASS_RTTI( CJournalGlossary )
	PARENT_CLASS( CJournalContainer )
	NATIVE_FUNCTION( "GetTitleStringId",  funcGetTitleStringId )
	NATIVE_FUNCTION( "GetImagePath",  funcGetImagePath )
	PROPERTY_EDIT( m_title, TXT( "Title" ) )
	PROPERTY_EDIT( m_image, TXT( "Image" ) )
	PROPERTY_EDIT_NOSERIALIZE( m_active, TXT( "In journal at beginning" ) )
END_CLASS_RTTI()

//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------

class CJournalGlossaryGroup : public CJournalBase
{
	DECLARE_ENGINE_CLASS( CJournalGlossaryGroup, CJournalBase, 0 )

public:
	CJournalGlossaryGroup();
	virtual ~CJournalGlossaryGroup();
	virtual Bool IsParentClass( CJournalBase* other ) const;

private:
};

BEGIN_CLASS_RTTI( CJournalGlossaryGroup )
	PARENT_CLASS( CJournalBase )
END_CLASS_RTTI()

//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------

class CJournalGlossaryRoot : public CJournalBase
{
public:
	DECLARE_ENGINE_CLASS( CJournalGlossaryRoot, CJournalBase, 0 )

	CJournalGlossaryRoot();
	virtual ~CJournalGlossaryRoot();

	virtual Bool IsParentClass( CJournalBase* other ) const;
};

BEGIN_CLASS_RTTI( CJournalGlossaryRoot )
	PARENT_CLASS( CJournalBase )
END_CLASS_RTTI()
