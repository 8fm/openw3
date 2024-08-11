#pragma once

#include "../../common/game/journalBase.h"

class CJournalPlaceDescription : public CJournalContainerEntry
{
	DECLARE_ENGINE_CLASS( CJournalPlaceDescription, CJournalContainerEntry, 0 )

public:
	CJournalPlaceDescription();
	virtual ~CJournalPlaceDescription();
	virtual Bool IsParentClass( CJournalBase* other ) const;
	void funcGetDescriptionStringId( CScriptStackFrame& stack, void* result );
private:
	LocalizedString m_description;
};

BEGIN_CLASS_RTTI( CJournalPlaceDescription )
	PARENT_CLASS( CJournalContainerEntry )
	PROPERTY_EDIT( m_description, TXT( "Description" ) )
	PROPERTY_EDIT_NOSERIALIZE( m_active, TXT( "In journal at beginning" ) )
	NATIVE_FUNCTION( "GetDescriptionStringId",  funcGetDescriptionStringId )
END_CLASS_RTTI()

//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------

class CJournalPlace : public CJournalContainer
{
	DECLARE_ENGINE_CLASS( CJournalPlace, CJournalContainer, 0 )

public:
	CJournalPlace();
	virtual ~CJournalPlace();
	virtual Bool IsParentClass( CJournalBase* other ) const;

private:
	void funcGetNameStringId( CScriptStackFrame& stack, void* result );
	void funcGetImage( CScriptStackFrame& stack, void* result );

private:
	LocalizedString m_name;
	String m_image;
};

BEGIN_CLASS_RTTI( CJournalPlace )
	PARENT_CLASS( CJournalContainer )
	PROPERTY_EDIT( m_name, TXT( "Name" ) )
	PROPERTY_EDIT( m_image, TXT( "Image" ) )
	PROPERTY_EDIT_NOSERIALIZE( m_active, TXT( "In journal at beginning" ) )
	NATIVE_FUNCTION( "GetNameStringId",  funcGetNameStringId )
	NATIVE_FUNCTION( "GetImage",  funcGetImage )
END_CLASS_RTTI()

//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------

class CJournalPlaceGroup : public CJournalBase
{
	DECLARE_ENGINE_CLASS( CJournalPlaceGroup, CJournalBase, 0 )

public:
	CJournalPlaceGroup();
	virtual ~CJournalPlaceGroup();
	virtual Bool IsParentClass( CJournalBase* other ) const;

private:
	void funcGetNameStringId( CScriptStackFrame& stack, void* result );
	void funcGetImage( CScriptStackFrame& stack, void* result );

private:
	LocalizedString m_name;
	String m_image;
};

BEGIN_CLASS_RTTI( CJournalPlaceGroup )
	PARENT_CLASS( CJournalBase )
	PROPERTY_EDIT( m_name, TXT( "Name" ) )
	PROPERTY_EDIT( m_image, TXT( "Image" ) )
	NATIVE_FUNCTION( "GetNameStringId",	funcGetNameStringId )
	NATIVE_FUNCTION( "GetImage",  funcGetImage )
END_CLASS_RTTI()

//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------

class CJournalPlaceRoot : public CJournalBase
{
public:
	DECLARE_ENGINE_CLASS( CJournalPlaceRoot, CJournalBase, 0 )

	CJournalPlaceRoot();
	virtual ~CJournalPlaceRoot();

	virtual Bool IsParentClass( CJournalBase* other ) const;
};

BEGIN_CLASS_RTTI( CJournalPlaceRoot )
	PARENT_CLASS( CJournalBase )
END_CLASS_RTTI()
