#include "build.h"
#include "behTreeCustomMoveData.h"

#include "behTreeInstance.h"

IMPLEMENT_ENGINE_CLASS( CBehTreeCustomMoveData );

////////////////////////////////////////////////////////////////////////
// CBehTreCustomMoveData::CInitializer
////////////////////////////////////////////////////////////////////////
void CBehTreeCustomMoveData::CInitializer::InitializeItem( CAIStorageItem& item ) const
{
}
IRTTIType* CBehTreeCustomMoveData::CInitializer::GetItemType() const
{
	return CBehTreeCustomMoveData::GetStaticClass();
}
CName CBehTreeCustomMoveData::CInitializer::GetItemName() const
{
	return DefaultStorageName();
}

////////////////////////////////////////////////////////////////////////
// CBehTreeCustomMoveDataPtr
////////////////////////////////////////////////////////////////////////
CBehTreeCustomMoveDataPtr::CBehTreeCustomMoveDataPtr( CAIStorage* storage )
	: Super( CBehTreeCustomMoveData::CInitializer::CInitializer(), storage )
{

}

