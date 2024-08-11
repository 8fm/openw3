#pragma once

#include "../../common/game/journalBase.h"


//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------

class CJournalCreatureVitalSpotEntry : public CJournalContainerEntry
{
	DECLARE_ENGINE_CLASS( CJournalCreatureVitalSpotEntry, CJournalContainerEntry, 0 )

public:

	virtual Bool IsParentClass( CJournalBase* other ) const;
	void funcGetDescriptionStringId( CScriptStackFrame& stack, void* result );
	void funcGetTitleStringId( CScriptStackFrame& stack, void* result );
	void funcGetCreatureEntry( CScriptStackFrame& stack, void* result );
private:

	LocalizedString m_title;
	LocalizedString m_description;
};

BEGIN_CLASS_RTTI( CJournalCreatureVitalSpotEntry )
	PARENT_CLASS( CJournalContainerEntry )	
	PROPERTY_EDIT( m_title, TXT( "Title" ) )
	PROPERTY_EDIT( m_description, TXT( "Description" ) )
	PROPERTY_EDIT_NOSERIALIZE( m_active, TXT( "In journal at beginning" ) )

	NATIVE_FUNCTION( "GetTitleStringId", funcGetTitleStringId )
	NATIVE_FUNCTION( "GetDescriptionStringId", funcGetDescriptionStringId )
	NATIVE_FUNCTION( "GetCreatureEntry", funcGetCreatureEntry )
END_CLASS_RTTI()


//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------

class CJournalCreatureVitalSpotGroup : public CJournalContainer
{
	DECLARE_ENGINE_CLASS( CJournalCreatureVitalSpotGroup, CJournalContainer, 0 )

public:
	virtual Bool IsParentClass( CJournalBase* other ) const;

protected:
	virtual void DefaultValues();

private:
};

BEGIN_CLASS_RTTI( CJournalCreatureVitalSpotGroup )
	PARENT_CLASS( CJournalContainer )
END_CLASS_RTTI()


//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------

class CJournalCreatureGameplayHint : public CJournalContainerEntry
{
	DECLARE_ENGINE_CLASS( CJournalCreatureGameplayHint, CJournalContainerEntry, 0 )

public:

	CJournalCreatureGameplayHint();
	virtual ~CJournalCreatureGameplayHint();

	virtual Bool IsParentClass( CJournalBase* other ) const;

private:

	LocalizedString m_description;
};

BEGIN_CLASS_RTTI( CJournalCreatureGameplayHint )
	PARENT_CLASS( CJournalContainerEntry )
	PROPERTY_EDIT( m_description, TXT( "Description" ) )
END_CLASS_RTTI()

//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------

class CJournalCreatureHuntingClue : public CJournalContainerEntry
{
	DECLARE_ENGINE_CLASS( CJournalCreatureHuntingClue, CJournalContainerEntry, 0 )

public:

	CJournalCreatureHuntingClue();
	virtual ~CJournalCreatureHuntingClue();

	// Row number 
	const CName& GetClueCategoryName() const { return m_category; }
	const CEnum* GetClueCategory() const { return SRTTI::GetInstance().FindEnum( m_category ); }
	Int32 GetClueIndex() const { return m_clue; }

	virtual Bool IsParentClass( CJournalBase* other ) const;

private:
	CName m_category;
	Int32 m_clue;
};

BEGIN_CLASS_RTTI( CJournalCreatureHuntingClue )
	PARENT_CLASS( CJournalContainerEntry )
	PROPERTY_CUSTOM_EDIT( m_category, TXT( "Category" ), TXT( "HuntingClueCategory" ) )
	PROPERTY_CUSTOM_EDIT( m_clue, TXT( "Clue" ), TXT( "HuntingClueItem" ) )
	PROPERTY_EDIT_NOSERIALIZE( m_active, TXT( "In journal at beginning" ) )
END_CLASS_RTTI()

//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------

class CJournalCreatureDescriptionEntry : public CJournalContainerEntry
{
	DECLARE_ENGINE_CLASS( CJournalCreatureDescriptionEntry, CJournalContainerEntry, 0 )

public:

	CJournalCreatureDescriptionEntry();
	virtual ~CJournalCreatureDescriptionEntry();

	virtual Bool IsParentClass( CJournalBase* other ) const;
	void funcGetDescriptionStringId( CScriptStackFrame& stack, void* result );
private:

	LocalizedString m_description;
};

BEGIN_CLASS_RTTI( CJournalCreatureDescriptionEntry )
	PARENT_CLASS( CJournalContainerEntry )
	NATIVE_FUNCTION( "GetDescriptionStringId",		funcGetDescriptionStringId )
	PROPERTY_EDIT( m_description, TXT( "Description" ) )
	PROPERTY_EDIT_NOSERIALIZE( m_active, TXT( "In journal at beginning" ) )
END_CLASS_RTTI()

//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------

class CJournalCreatureGameplayHintGroup : public CJournalContainer
{
	DECLARE_ENGINE_CLASS( CJournalCreatureGameplayHintGroup, CJournalContainer, 0 )

public:
	CJournalCreatureGameplayHintGroup();
	virtual ~CJournalCreatureGameplayHintGroup();

	virtual Bool IsParentClass( CJournalBase* other ) const;

protected:
	virtual void DefaultValues();

private:
};

BEGIN_CLASS_RTTI( CJournalCreatureGameplayHintGroup )
	PARENT_CLASS( CJournalContainer )
END_CLASS_RTTI()

//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------

class CJournalCreatureHuntingClueGroup : public CJournalContainer
{
	DECLARE_ENGINE_CLASS( CJournalCreatureHuntingClueGroup, CJournalContainer, 0 )

public:

	CJournalCreatureHuntingClueGroup();
	virtual ~CJournalCreatureHuntingClueGroup();

	virtual Bool IsParentClass( CJournalBase* other ) const;

protected:
	virtual void DefaultValues();

private:
};

BEGIN_CLASS_RTTI( CJournalCreatureHuntingClueGroup )
	PARENT_CLASS( CJournalContainer )
END_CLASS_RTTI()

//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------

class CJournalCreatureDescriptionGroup : public CJournalContainer
{
	DECLARE_ENGINE_CLASS( CJournalCreatureDescriptionGroup, CJournalContainer, 0 )

public:
	CJournalCreatureDescriptionGroup();
	virtual ~CJournalCreatureDescriptionGroup();

	virtual Bool IsParentClass( CJournalBase* other ) const;

protected:
	virtual void DefaultValues();

private:
};

BEGIN_CLASS_RTTI( CJournalCreatureDescriptionGroup )
	PARENT_CLASS( CJournalContainer )
END_CLASS_RTTI()

//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------

class CJournalCreature : public CJournalContainer
{
	DECLARE_ENGINE_CLASS( CJournalCreature, CJournalContainer, 0 )

public:

	CJournalCreature();
	virtual ~CJournalCreature();

	virtual Bool IsParentClass( CJournalBase* other ) const;

private:
	void funcGetNameStringId( CScriptStackFrame& stack, void* result );
	void funcGetImage( CScriptStackFrame& stack, void* result );
	void funcGetEntityTemplateFilename( CScriptStackFrame& stack, void* result );
	void funcGetItemsUsedAgainstCreature( CScriptStackFrame& stack, void* result );

private:
	LocalizedString m_name;
	String m_image;
	TSoftHandle< CEntityTemplate >		m_entityTemplate;
	TDynArray< CName > m_itemsUsedAgainstCreature;
};

BEGIN_CLASS_RTTI( CJournalCreature )
	PARENT_CLASS( CJournalContainer )
	PROPERTY_EDIT( m_name, TXT( "Name" ) )
	PROPERTY_EDIT( m_image, TXT( "Image" ) )
	PROPERTY_EDIT( m_entityTemplate, TXT( "Entity template" ) )
	PROPERTY_CUSTOM_EDIT( m_itemsUsedAgainstCreature, TXT( "Items used against" ), TXT("ChooseItem") )
	PROPERTY_EDIT_NOSERIALIZE( m_active, TXT( "In journal at beginning" ) )

	NATIVE_FUNCTION( "GetNameStringId",	funcGetNameStringId )
	NATIVE_FUNCTION( "GetImage",		funcGetImage )
	NATIVE_FUNCTION( "GetEntityTemplateFilename", funcGetEntityTemplateFilename )
	NATIVE_FUNCTION( "GetItemsUsedAgainstCreature", funcGetItemsUsedAgainstCreature )

END_CLASS_RTTI()

//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------

class CJournalCreatureGroup : public CJournalBase
{
	DECLARE_ENGINE_CLASS( CJournalCreatureGroup, CJournalBase, 0 )

public:
	CJournalCreatureGroup();
	virtual ~CJournalCreatureGroup();
	virtual Bool IsParentClass( CJournalBase* other ) const;

private:
	void funcGetNameStringId( CScriptStackFrame& stack, void* result );
	void funcGetImage( CScriptStackFrame& stack, void* result );

private:
	LocalizedString m_name;
	String m_image;
};

BEGIN_CLASS_RTTI( CJournalCreatureGroup )
	PARENT_CLASS( CJournalBase )
	PROPERTY_EDIT( m_name, TXT( "Name" ) )
	PROPERTY_EDIT( m_image, TXT( "Image" ) )

	NATIVE_FUNCTION( "GetNameStringId",	funcGetNameStringId )
	NATIVE_FUNCTION( "GetImage",		funcGetImage )
END_CLASS_RTTI()

//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------

class CJournalCreatureVirtualGroup : public CJournalLink
{
	DECLARE_ENGINE_CLASS( CJournalCreatureVirtualGroup, CJournalLink, 0 )

public:
	CJournalCreatureVirtualGroup();
	virtual ~CJournalCreatureVirtualGroup();
	virtual Bool IsParentClass( CJournalBase* other ) const;
};

BEGIN_CLASS_RTTI( CJournalCreatureVirtualGroup )
	PARENT_CLASS( CJournalLink )
END_CLASS_RTTI()

//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------

class CJournalCreatureRoot : public CJournalBase
{
public:
	DECLARE_ENGINE_CLASS( CJournalCreatureRoot, CJournalBase, 0 )

	CJournalCreatureRoot();
	virtual ~CJournalCreatureRoot();

	virtual Bool IsParentClass( CJournalBase* other ) const;
};

BEGIN_CLASS_RTTI( CJournalCreatureRoot )
	PARENT_CLASS( CJournalBase )
END_CLASS_RTTI();
