#include "build.h"
#include "behTreeCounterData.h"

IMPLEMENT_ENGINE_CLASS( CBehTreeCounterData );

//////////////////////////////////////////////////////////////////////
// CBehTreeCounterDataPtr
//////////////////////////////////////////////////////////////////////
CBehTreeCounterDataPtr::CBehTreeCounterDataPtr( CAIStorage* storage, CName name )
	: Super( CBehTreeCounterData::CInitializer::CInitializer( name ), storage )
{

}

//////////////////////////////////////////////////////////////////////
// CBehTreeCounterData::CInitializer
//////////////////////////////////////////////////////////////////////
void CBehTreeCounterData::CInitializer::InitializeItem( CAIStorageItem& item ) const
{
}

IRTTIType* CBehTreeCounterData::CInitializer::GetItemType() const
{
	return CBehTreeCounterData::GetStaticClass();
}