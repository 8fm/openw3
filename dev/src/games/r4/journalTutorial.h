#pragma once

#include "../../common/game/journalBase.h"

//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------

class CJournalTutorial : public CJournalChildBase
{
	DECLARE_ENGINE_CLASS( CJournalTutorial, CJournalChildBase, 0 )

public:
	CJournalTutorial();
	virtual ~CJournalTutorial();
	virtual Bool IsParentClass( CJournalBase* other ) const;

	void funcGetDescriptionStringId( CScriptStackFrame& stack, void* result );
	void funcGetNameStringId( CScriptStackFrame& stack, void* result );
	void funcGetImagePath( CScriptStackFrame& stack, void* result );
	void funcGetVideoPath( CScriptStackFrame& stack, void* result );
	void funcGetDLCLock( CScriptStackFrame& stack, void* result );

private:
	LocalizedString m_name;
	String m_image;
	String m_video;
	LocalizedString m_description;
	CName m_dlcLock;
};

RED_DECLARE_NAME( video )

BEGIN_CLASS_RTTI( CJournalTutorial )
	PARENT_CLASS( CJournalChildBase )
	NATIVE_FUNCTION( "GetDescriptionStringId",  funcGetDescriptionStringId )
	NATIVE_FUNCTION( "GetNameStringId",  funcGetNameStringId )
	NATIVE_FUNCTION( "GetImagePath",  funcGetImagePath )
	NATIVE_FUNCTION( "GetVideoPath",  funcGetVideoPath )
	NATIVE_FUNCTION( "GetDLCLock", funcGetDLCLock )
	PROPERTY_EDIT( m_name, TXT( "Name" ) )
	PROPERTY_EDIT( m_image, TXT( "Image" ) )
	PROPERTY_EDIT( m_video, TXT( "Video" ) )
	PROPERTY_EDIT( m_description, TXT( "Description" ) )
	PROPERTY_EDIT( m_dlcLock, TXT("DLC Lock") )
	PROPERTY_EDIT_NOSERIALIZE( m_active, TXT( "In journal at beginning" ) )
END_CLASS_RTTI()
//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------

class CJournalTutorialGroup : public CJournalBase
{
	DECLARE_ENGINE_CLASS( CJournalTutorialGroup, CJournalBase, 0 )

public:
	CJournalTutorialGroup();
	virtual ~CJournalTutorialGroup();
	virtual Bool IsParentClass( CJournalBase* other ) const;

   	void funcGetNameStringId( CScriptStackFrame& stack, void* result );
	void funcGetImage( CScriptStackFrame& stack, void* result );

private:
	LocalizedString m_name;
	String m_image;
};

BEGIN_CLASS_RTTI( CJournalTutorialGroup )
	PARENT_CLASS( CJournalBase )
	PROPERTY_EDIT( m_name, TXT( "Name" ) )
	PROPERTY_EDIT( m_image, TXT( "Image" ) )

	NATIVE_FUNCTION( "GetNameStringId",  funcGetNameStringId )
	NATIVE_FUNCTION( "GetImage",  funcGetImage )
END_CLASS_RTTI()

//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------

class CJournalTutorialRoot : public CJournalBase
{
public:
	DECLARE_ENGINE_CLASS( CJournalTutorialRoot, CJournalBase, 0 )

	CJournalTutorialRoot();
	virtual ~CJournalTutorialRoot();

	virtual Bool IsParentClass( CJournalBase* other ) const;
};

BEGIN_CLASS_RTTI( CJournalTutorialRoot )
	PARENT_CLASS( CJournalBase )
END_CLASS_RTTI()
