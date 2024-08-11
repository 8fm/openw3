
#pragma once

#include "../../common/game/journalBase.h"

//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------

class CJournalStoryBookPageDescription : public CJournalContainerEntry
{
	DECLARE_ENGINE_CLASS( CJournalStoryBookPageDescription, CJournalContainerEntry, 0 )

public:
	CJournalStoryBookPageDescription();
	virtual ~CJournalStoryBookPageDescription();

	virtual Bool IsParentClass( CJournalBase* other ) const;

	RED_INLINE const String& GetVideoFilename() const { return m_videoFilename; }
	RED_INLINE String GetDescription() const { return m_description.GetString(); }
	RED_INLINE Bool IsFinal() const { return m_isFinal; }

	void funcGetVideoFilename( CScriptStackFrame& stack, void* result );
	void funcGetDescriptionStringId( CScriptStackFrame& stack, void* result );

#ifndef NO_EDITOR_RESOURCE_SAVE
	virtual void OnPreSave();
#endif

private:
	String m_videoFilename;
	LocalizedString m_description;
	Bool m_isFinal;
};

BEGIN_CLASS_RTTI( CJournalStoryBookPageDescription )
	PARENT_CLASS( CJournalContainerEntry )
	NATIVE_FUNCTION( "GetVideoFilename",  funcGetVideoFilename )
	NATIVE_FUNCTION( "GetDescriptionStringId",  funcGetDescriptionStringId )
	PROPERTY_EDIT( m_videoFilename, TXT( "Video filename" ) )
	PROPERTY_EDIT( m_description, TXT( "Description" ) )
	PROPERTY_EDIT( m_isFinal, TXT( "Final" ) )
	PROPERTY_EDIT_NOSERIALIZE( m_active, TXT( "In journal at beginning" ) )
END_CLASS_RTTI()

//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------

class CJournalStoryBookPage : public CJournalContainer
{
	DECLARE_ENGINE_CLASS( CJournalStoryBookPage, CJournalContainer, 0 )

public:
	CJournalStoryBookPage();
	virtual ~CJournalStoryBookPage();
	virtual void DefaultValues();
	virtual Bool IsParentClass( CJournalBase* other ) const;

	RED_INLINE String GetTitle() const { return m_title.GetString(); }
	RED_INLINE Uint32 GetWorld() const { return m_world; }

	void funcGetTitleStringId( CScriptStackFrame& stack, void* result );

private:
	LocalizedString m_title;
	Uint32 m_world;
};

BEGIN_CLASS_RTTI( CJournalStoryBookPage )
	PARENT_CLASS( CJournalContainer )
	NATIVE_FUNCTION( "GetTitleStringId",  funcGetTitleStringId )
	PROPERTY_EDIT( m_title, TXT( "Title" ) )
	PROPERTY_CUSTOM_EDIT( m_world, TXT( "World" ), TXT("WorldSelection_Quest") )
	PROPERTY_EDIT_NOSERIALIZE( m_active, TXT( "In journal at beginning" ) )
END_CLASS_RTTI()

//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------

class CJournalStoryBookChapter : public CJournalBase
{
	DECLARE_ENGINE_CLASS( CJournalStoryBookChapter, CJournalBase, 0 )

public:
	CJournalStoryBookChapter();
	virtual ~CJournalStoryBookChapter();
	virtual Bool IsParentClass( CJournalBase* other ) const;
	RED_INLINE String GetTitle() const { m_title.Load(); return m_title.GetString(); } 

private:
   	void funcGetTitleStringId( CScriptStackFrame& stack, void* result );
	void funcGetImage( CScriptStackFrame& stack, void* result );

private:
	LocalizedString m_title;
	String m_image;
};

BEGIN_CLASS_RTTI( CJournalStoryBookChapter )
	PARENT_CLASS( CJournalBase )
	PROPERTY_EDIT( m_title, TXT( "Chapter Heading" ) )
	PROPERTY_EDIT( m_image, TXT( "Image" ) )
	PROPERTY_EDIT_NOSERIALIZE( m_active, TXT( "In journal at beginning" ) )

	NATIVE_FUNCTION( "GetTitleStringId",  funcGetTitleStringId )
	NATIVE_FUNCTION( "GetImage",  funcGetImage )
END_CLASS_RTTI()

//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------

class CJournalStoryBookRoot : public CJournalBase
{
public:
	DECLARE_ENGINE_CLASS( CJournalStoryBookRoot, CJournalBase, 0 )

	CJournalStoryBookRoot();
	virtual ~CJournalStoryBookRoot();

	virtual Bool IsParentClass( CJournalBase* other ) const;
};

BEGIN_CLASS_RTTI( CJournalStoryBookRoot )
	PARENT_CLASS( CJournalBase )
END_CLASS_RTTI()
