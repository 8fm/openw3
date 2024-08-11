#pragma once

#include "../core/hashmap.h"

// This class stores an individual 'group' of memory budget information.
// A group consists of either entire pool budgets, or individual memory class budgets
// It also must contain a memory manager pointer, as labels and classes are per-manager
class CMemoryBudgetGroup
{
public:
	CMemoryBudgetGroup( const String& label, Red::MemoryFramework::MemoryManager* memoryManager );
	CMemoryBudgetGroup( const CMemoryBudgetGroup& other );
	CMemoryBudgetGroup( CMemoryBudgetGroup&& other );
	~CMemoryBudgetGroup();

	CMemoryBudgetGroup& operator=( const CMemoryBudgetGroup& other );
	CMemoryBudgetGroup& operator=( CMemoryBudgetGroup&& other );

	CMemoryBudgetGroup& AddPoolBudget( Red::MemoryFramework::PoolLabel pool, MemSize budget );
	CMemoryBudgetGroup& AddClassBudget( Red::MemoryFramework::MemoryClass memClass, MemSize budget );
	CMemoryBudgetGroup& AddClassGroupBudget( const AnsiChar* groupName, MemSize budget );

	Uint32 GetTotalBudgets() const	{ return m_poolBudgets.Size() + m_memClassBudgets.Size() + m_memClassGroupBudgets.Size(); }
	const String& GetLabel() const	{ return m_groupLabel; }

	typedef THashMap< Red::MemoryFramework::PoolLabel, MemSize > BudgetGroupPools;
	typedef THashMap< Red::MemoryFramework::MemoryClass, MemSize > BudgetGroupClasses;
	typedef THashMap< Uint32, MemSize > BudgetGroupMemclassGroup;

	BudgetGroupPools::iterator BeginPools() { return m_poolBudgets.Begin(); }
	BudgetGroupPools::iterator EndPools() { return m_poolBudgets.End(); }

	BudgetGroupClasses::iterator BeginClasses() { return m_memClassBudgets.Begin(); }
	BudgetGroupClasses::iterator EndClasses() { return m_memClassBudgets.End(); }

	BudgetGroupMemclassGroup::iterator BeginClassGroups() { return m_memClassGroupBudgets.Begin(); }
	BudgetGroupMemclassGroup::iterator EndClassGroups() { return m_memClassGroupBudgets.End(); }

	Red::MemoryFramework::MemoryManager* GetMemoryManager() { return m_memoryManager; }

private:
	String m_groupLabel;
	BudgetGroupPools m_poolBudgets;
	BudgetGroupClasses m_memClassBudgets;
	BudgetGroupMemclassGroup m_memClassGroupBudgets;
	Red::MemoryFramework::MemoryManager* m_memoryManager;
};

// This class is used to store memory budgets, broken down into platforms, pools, and memory classes
class CMemoryBudgetStorage
{
public:
	CMemoryBudgetStorage();
	~CMemoryBudgetStorage();

	void AddPlatform( Uint32 platformId, const String& platformName );
	void AddGroup( Uint32 platformId, const CMemoryBudgetGroup& group );

	typedef TDynArray< CMemoryBudgetGroup > BudgetGroupArray;
	typedef THashMap< Uint32, String > PlatformList;

	// Group iterators
	BudgetGroupArray::iterator BeginGroups( Uint32 platformIndex );
	BudgetGroupArray::iterator EndGroups( Uint32 platformIndex );

	// Platform iterators
	PlatformList::iterator BeginPlatforms();
	PlatformList::iterator EndPlatforms();

private:
	THashMap< Uint32, BudgetGroupArray > m_memoryBudgetGroups;		// Platform 'id' -> budget groups
	PlatformList m_platformNames;
};

typedef TSingleton< CMemoryBudgetStorage > SMemoryBudgets;		// Memory budgets singleton.
