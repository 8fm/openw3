/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
namespace Red { namespace MemoryFramework { 

/////////////////////////////////////////////////////////////////
// CTor
//
RED_INLINE AllocatorInfo::AllocatorInfo()
{
}

/////////////////////////////////////////////////////////////////
// DTor
//
RED_INLINE AllocatorInfo::~AllocatorInfo()
{
}

/////////////////////////////////////////////////////////////////
// GetBudget
//
RED_INLINE Red::System::MemSize AllocatorInfo::GetBudget()
{
	return m_totalBudget;
}

/////////////////////////////////////////////////////////////////
// GetPerAllocationOverhead
//
RED_INLINE Red::System::Uint32 AllocatorInfo::GetPerAllocationOverhead()
{
	return m_perAllocationOverhead;
}

/////////////////////////////////////////////////////////////////
// GetTypeName
//
RED_INLINE const Red::System::Char* AllocatorInfo::GetTypeName()
{
	return m_allocatorType;
}


/////////////////////////////////////////////////////////////////
// SetAllocatorTypeName
//
RED_INLINE void AllocatorInfo::SetAllocatorTypeName( const Red::System::Char* typeName )
{
	Red::System::StringCopy( m_allocatorType, typeName, c_allocatorTypeLength );
}

/////////////////////////////////////////////////////////////////
// SetAllocatorBudget
//
RED_INLINE void AllocatorInfo::SetAllocatorBudget( Red::System::MemSize budget )
{
	m_totalBudget = budget;
}

/////////////////////////////////////////////////////////////////
// SetPerAllocationOverhead
//
RED_INLINE void AllocatorInfo::SetPerAllocationOverhead( Red::System::Uint32 overhead )
{
	m_perAllocationOverhead = overhead;
}

} }
