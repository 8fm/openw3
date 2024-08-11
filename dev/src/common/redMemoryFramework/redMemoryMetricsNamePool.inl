/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "../redSystem/crt.h"
#include "redMemoryAssert.h"
#include "../redMath/numericalutils.h"

namespace Red { namespace MemoryFramework {

///////////////////////////////////////////////////////////////////////
// MetricsNameEntry CTor
//
template< Red::System::Uint32 MaximumNames, Red::System::Uint32 MaximumNameLength >
RED_INLINE MetricsNamePool<MaximumNames, MaximumNameLength>::MetricsNameEntry::MetricsNameEntry()
	: m_nameLabel(0)
	, m_nameLength(0)
{

}

///////////////////////////////////////////////////////////////////////
// MetricsNameEntry CTor
//
template< Red::System::Uint32 MaximumNames, Red::System::Uint32 MaximumNameLength >
RED_INLINE MetricsNamePool<MaximumNames, MaximumNameLength>::MetricsNameEntry::MetricsNameEntry( Red::System::Uint32 label, const Red::System::AnsiChar* name )
	: m_nameLabel(label)
{
	Red::System::Uint32 nameLength = Red::Math::NumericalUtils::Min( MaximumNameLength-1, static_cast<Red::System::Uint32>( Red::System::StringLength( name )+1 ) );
	Red::System::StringCopy( m_nameValue, name, nameLength );
	m_nameValue[nameLength] = '\0';
	m_nameLength = nameLength;
}

///////////////////////////////////////////////////////////////////////
// CTor
//
template< Red::System::Uint32 MaximumNames, Red::System::Uint32 MaximumNameLength >
RED_INLINE MetricsNamePool<MaximumNames, MaximumNameLength>::MetricsNamePool()
	: m_nameCount(0)
{

}

///////////////////////////////////////////////////////////////////////
// DTor
//
template< Red::System::Uint32 MaximumNames, Red::System::Uint32 MaximumNameLength >
RED_INLINE MetricsNamePool<MaximumNames, MaximumNameLength>::~MetricsNamePool()
{

}

///////////////////////////////////////////////////////////////////////
// RegisterName
// Add a name to the list
template< Red::System::Uint32 MaximumNames, Red::System::Uint32 MaximumNameLength >
RED_INLINE void MetricsNamePool<MaximumNames, MaximumNameLength>::RegisterName( Red::System::Uint32 label, const Red::System::AnsiChar* name )
{
	RED_MEMORY_ASSERT( m_nameCount < MaximumNames, "Too many names registered with this name pool" );
	if( name != nullptr && m_nameCount < MaximumNames )
	{
		m_names[ m_nameCount ] = MetricsNameEntry( label, name );
		m_nameCount++;
	}
}

///////////////////////////////////////////////////////////////////////
// GetNameByLabel
// Search the list for the name associated with a label
template< Red::System::Uint32 MaximumNames, Red::System::Uint32 MaximumNameLength >
RED_INLINE Red::System::Bool MetricsNamePool<MaximumNames, MaximumNameLength>::GetNameByLabel( Red::System::Uint32 label, Red::System::AnsiChar* name, Red::System::Uint32 maxCharacters ) const
{
	if( maxCharacters == 0 )
	{
		return false;
	}

	// Find the name in the list
	Red::System::Uint32 indexCounter = 0;
	while( indexCounter < MaximumNames )
	{
		if( m_names[indexCounter].m_nameLabel == label )
		{
			// This is the label, take a copy of the name
			Red::System::MemSize nameLength = Red::System::StringLength( m_names[indexCounter].m_nameValue );
			if( nameLength > 0 )
			{
				Red::System::MemSize clampedLength = Red::Math::NumericalUtils::Min( nameLength, ( Red::System::MemSize )maxCharacters - 1 );
				Red::System::MemoryCopy( name, m_names[indexCounter].m_nameValue, clampedLength * sizeof( Red::System::AnsiChar ) );
				name[ clampedLength ] = 0;
			}
			return true;
		}
		++indexCounter;
	}

	return false;
}

///////////////////////////////////////////////////////////////////////
// GetNameByIndex
// Get the entry at a specific index
template< Red::System::Uint32 MaximumNames, Red::System::Uint32 MaximumNameLength >
RED_INLINE Red::System::Bool MetricsNamePool<MaximumNames, MaximumNameLength>::GetNameByIndex( Red::System::Uint32 index, Red::System::AnsiChar* name, Red::System::Uint32 maxCharacters ) const
{
	if( maxCharacters == 0 || index >= MaximumNames )
	{
		return false;
	}

	Red::System::Uint32 minLength = Red::Math::NumericalUtils::Min(m_names[index].m_nameLength, maxCharacters);
	Red::System::MemoryCopy( name, m_names[index].m_nameValue, minLength );
	name[minLength] = '\0';

	return true;
}

///////////////////////////////////////////////////////////////////////
// GetLabelByIndex
// Get the label at a specific index
template< Red::System::Uint32 MaximumNames, Red::System::Uint32 MaximumNameLength >
RED_INLINE Red::System::Uint32 MetricsNamePool<MaximumNames, MaximumNameLength>::GetLabelByIndex( Red::System::Uint32 index )
{
	if( index >= MaximumNames )
	{
		return (Red::System::Uint32)-1;
	}

	return m_names[index].m_nameLabel;
}

///////////////////////////////////////////////////////////////////////
// GetNameCount
// Get the number of names registered
template< Red::System::Uint32 MaximumNames, Red::System::Uint32 MaximumNameLength >
RED_INLINE Red::System::Uint32 MetricsNamePool<MaximumNames, MaximumNameLength>::GetNameCount() const
{
	return m_nameCount;
}

} }