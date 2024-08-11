#pragma once

#include "../../common/game/journalBase.h"

//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------

class CJournalItemComponent : public CJournalContainerEntry
{
	DECLARE_ENGINE_CLASS( CJournalItemComponent, CJournalContainerEntry, 0 )

public:
	CJournalItemComponent();
	virtual ~CJournalItemComponent();
	virtual Bool IsParentClass( CJournalBase* other ) const;

private:
	String m_image;
	CName m_item;
};

BEGIN_CLASS_RTTI( CJournalItemComponent )
	PARENT_CLASS( CJournalContainerEntry )
	PROPERTY_EDIT( m_image, TXT( "Image" ) )
	PROPERTY_CUSTOM_EDIT( m_item, TXT( "Ingredient" ), TXT( "ChooseItem" ) )
END_CLASS_RTTI()

//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------

class CJournalItem : public CJournalContainer
{
	DECLARE_ENGINE_CLASS( CJournalItem, CJournalContainer, 0 )

public:
	CJournalItem();
	virtual ~CJournalItem();
	virtual Bool IsParentClass( CJournalBase* other ) const;

private:
	CName m_item;
	String m_image;
	LocalizedString m_description;
};

BEGIN_CLASS_RTTI( CJournalItem )
	PARENT_CLASS( CJournalContainer )
	PROPERTY_CUSTOM_EDIT( m_item, TXT( "Item" ), TXT( "ChooseItem" ) )
	PROPERTY_EDIT( m_image, TXT( "Image" ) )
	PROPERTY_EDIT( m_description, TXT( "Description" ) )
END_CLASS_RTTI()

//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------

class CJournalItemSubGroup : public CJournalChildBase
{
	DECLARE_ENGINE_CLASS( CJournalItemSubGroup, CJournalChildBase, 0 )

public:
	CJournalItemSubGroup();
	virtual ~CJournalItemSubGroup();
	virtual Bool IsParentClass( CJournalBase* other ) const;

private:
	LocalizedString m_name;
};

BEGIN_CLASS_RTTI( CJournalItemSubGroup )
	PARENT_CLASS( CJournalChildBase )
	PROPERTY_EDIT( m_name, TXT( "Name" ) )
END_CLASS_RTTI()

//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------

class CJournalItemGroup : public CJournalBase
{
	DECLARE_ENGINE_CLASS( CJournalItemGroup, CJournalBase, 0 )

public:
	CJournalItemGroup();
	virtual ~CJournalItemGroup();
	virtual Bool IsParentClass( CJournalBase* other ) const;

private:
	LocalizedString m_name;
};

BEGIN_CLASS_RTTI( CJournalItemGroup )
	PARENT_CLASS( CJournalBase )
	PROPERTY_EDIT( m_name, TXT( "Name" ) )
END_CLASS_RTTI()

//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------

class CJournalItemRoot : public CJournalBase
{
public:
	DECLARE_ENGINE_CLASS( CJournalItemRoot, CJournalBase, 0 )

	CJournalItemRoot();
	virtual ~CJournalItemRoot();

	virtual Bool IsParentClass( CJournalBase* other ) const;
};

BEGIN_CLASS_RTTI( CJournalItemRoot )
	PARENT_CLASS( CJournalBase )
END_CLASS_RTTI()
