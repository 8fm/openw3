#include "build.h"
#include "journalItems.h"

//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------

IMPLEMENT_ENGINE_CLASS( CJournalItemComponent );

CJournalItemComponent::CJournalItemComponent()
{

}

CJournalItemComponent::~CJournalItemComponent()
{

}

Bool CJournalItemComponent::IsParentClass( CJournalBase* other ) const
{
	return other->IsA< CJournalItem >();
}

//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------

IMPLEMENT_ENGINE_CLASS( CJournalItem );

CJournalItem::CJournalItem()
{

}

CJournalItem::~CJournalItem()
{

}

Bool CJournalItem::IsParentClass( CJournalBase* other ) const
{
	return other->IsA< CJournalItemSubGroup >();
}

//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------

IMPLEMENT_ENGINE_CLASS( CJournalItemSubGroup );

CJournalItemSubGroup::CJournalItemSubGroup()
{

}

CJournalItemSubGroup::~CJournalItemSubGroup()
{

}

Bool CJournalItemSubGroup::IsParentClass( CJournalBase* other ) const
{
	return other->IsA< CJournalItemGroup >();
}

//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------

IMPLEMENT_ENGINE_CLASS( CJournalItemGroup );

CJournalItemGroup::CJournalItemGroup()
{

}

CJournalItemGroup::~CJournalItemGroup()
{

}

Bool CJournalItemGroup::IsParentClass( CJournalBase* other ) const
{
	return other->IsA< CJournalItemRoot >();
}

//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------

IMPLEMENT_ENGINE_CLASS( CJournalItemRoot );

CJournalItemRoot::CJournalItemRoot()
{

}

CJournalItemRoot::~CJournalItemRoot()
{

}

Bool CJournalItemRoot::IsParentClass( CJournalBase* other ) const
{
	return false;
}
