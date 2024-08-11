#include "build.h"

#include "memoryBudgetStorage.h"

CMemoryBudgetGroup::CMemoryBudgetGroup( const String& label, Red::MemoryFramework::MemoryManager* memoryManager )
	: m_groupLabel( label )
	, m_memoryManager( memoryManager )
{
}

CMemoryBudgetGroup::CMemoryBudgetGroup( const CMemoryBudgetGroup& other )
	: m_groupLabel( other.m_groupLabel )
	, m_poolBudgets( other.m_poolBudgets )
	, m_memClassBudgets( other.m_memClassBudgets )
	, m_memClassGroupBudgets( other.m_memClassGroupBudgets )
	, m_memoryManager( other.m_memoryManager )
{
}

CMemoryBudgetGroup::CMemoryBudgetGroup( CMemoryBudgetGroup&& other )
	: m_groupLabel( std::move( other.m_groupLabel ) )
	, m_poolBudgets( std::move( other.m_poolBudgets ) )
	, m_memClassBudgets( std::move( other.m_memClassBudgets ) )
	, m_memClassGroupBudgets( std::move( other.m_memClassGroupBudgets ) )
	, m_memoryManager( std::move( other.m_memoryManager ) )
{

}

CMemoryBudgetGroup& CMemoryBudgetGroup::operator=( const CMemoryBudgetGroup& other )
{
	m_groupLabel = other.m_groupLabel;
	m_poolBudgets = other.m_poolBudgets;
	m_memClassBudgets = other.m_memClassBudgets;
	m_memClassGroupBudgets = other.m_memClassGroupBudgets;
	m_memoryManager = other.m_memoryManager;
	return *this;
}

CMemoryBudgetGroup& CMemoryBudgetGroup::operator=( CMemoryBudgetGroup&& other )
{
	m_groupLabel = std::move( other.m_groupLabel );
	m_poolBudgets = std::move( other.m_poolBudgets );
	m_memClassBudgets = std::move( other.m_memClassBudgets );
	m_memClassGroupBudgets = std::move( other.m_memClassGroupBudgets );
	m_memoryManager = std::move( other.m_memoryManager );
	return *this;
}

CMemoryBudgetGroup::~CMemoryBudgetGroup()
{

}

CMemoryBudgetGroup& CMemoryBudgetGroup::AddPoolBudget( Red::MemoryFramework::PoolLabel pool, MemSize budget )
{
	m_poolBudgets.Insert( pool, budget );

	return *this;
}

CMemoryBudgetGroup& CMemoryBudgetGroup::AddClassBudget( Red::MemoryFramework::MemoryClass memClass, MemSize budget )
{
	m_memClassBudgets.Insert( memClass, budget );

	return *this;
}

CMemoryBudgetGroup& CMemoryBudgetGroup::AddClassGroupBudget( const AnsiChar* groupName, MemSize budget )
{
	Red::MemoryFramework::AllocationMetricsCollector& metrics = m_memoryManager->GetMetricsCollector();
	Uint32 groupsInManager = metrics.GetMemoryClassGroupCount();
	for( Uint32 g = 0; g < groupsInManager; ++g )
	{
		const AnsiChar* name = metrics.GetMemoryClassGroupName( g );
		if( Red::System::StringCompare( groupName, name ) == 0 )
		{
			m_memClassGroupBudgets.Insert( g, budget );
			break;
		}
	}

	return *this;
}


CMemoryBudgetStorage::CMemoryBudgetStorage()
{

}

CMemoryBudgetStorage::~CMemoryBudgetStorage()
{

}

void CMemoryBudgetStorage::AddGroup( Uint32 platformId, const CMemoryBudgetGroup& group )
{
	if( !m_memoryBudgetGroups.KeyExist( platformId ) )
	{
		m_memoryBudgetGroups.Insert( platformId, BudgetGroupArray() );
	}

	BudgetGroupArray* groupArray = m_memoryBudgetGroups.FindPtr( platformId );
	RED_FATAL_ASSERT( groupArray, "Couldn't get platform entry" );

	groupArray->PushBack( group );
}

// Group iterators
CMemoryBudgetStorage::BudgetGroupArray::iterator CMemoryBudgetStorage::BeginGroups( Uint32 platformIndex )
{
	BudgetGroupArray* groupArray = m_memoryBudgetGroups.FindPtr( platformIndex );
	return groupArray ? groupArray->Begin() : BudgetGroupArray::iterator();
}

CMemoryBudgetStorage::BudgetGroupArray::iterator CMemoryBudgetStorage::EndGroups( Uint32 platformIndex )
{
	BudgetGroupArray* groupArray = m_memoryBudgetGroups.FindPtr( platformIndex );
	return groupArray ? groupArray->End() : BudgetGroupArray::iterator();
}

void CMemoryBudgetStorage::AddPlatform( Uint32 platformId, const String& platformName )
{
	RED_ASSERT( !m_platformNames.KeyExist( platformId ), TXT( "Platform index exists" ) );
	m_platformNames.Insert( platformId, platformName );
}

CMemoryBudgetStorage::PlatformList::iterator CMemoryBudgetStorage::BeginPlatforms()
{
	return m_platformNames.Begin();
}

CMemoryBudgetStorage::PlatformList::iterator CMemoryBudgetStorage::EndPlatforms()
{
	return m_platformNames.End();
}