/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#ifndef _RED_MEMORY_METRICS_NAME_POOL_H
#define _RED_MEMORY_METRICS_NAME_POOL_H

namespace Red { namespace MemoryFramework { 

// MetricsNamePool 
// Keep a lookup of index -> name in static memory
// Fixed-size class so we dont need to allocate anything extra
template< Red::System::Uint32 MaximumNames, Red::System::Uint32 MaximumNameLength >
class MetricsNamePool
{
public:
	RED_INLINE MetricsNamePool();
	RED_INLINE ~MetricsNamePool();

	// Add a name to the list
	RED_INLINE void RegisterName( Red::System::Uint32 label, const Red::System::AnsiChar* name );

	// Search the list for the name associated with a label
	RED_INLINE Red::System::Bool GetNameByLabel( Red::System::Uint32 label, Red::System::AnsiChar* name, Red::System::Uint32 maxCharacters ) const;

	// Get the name entry at a specific index
	RED_INLINE Red::System::Bool GetNameByIndex( Red::System::Uint32 index, Red::System::AnsiChar* name, Red::System::Uint32 maxCharacters ) const;

	// Get the name label at a specific index
	RED_INLINE Red::System::Uint32 GetLabelByIndex( Red::System::Uint32 index );

	// Get the number of names registered
	RED_INLINE Red::System::Uint32 GetNameCount() const;
private:
	// Entrys are saved as a label / name array
	class MetricsNameEntry
	{
	public:
		RED_INLINE MetricsNameEntry();
		RED_INLINE MetricsNameEntry( Red::System::Uint32 label, const Red::System::AnsiChar* name );

		Red::System::Uint32   m_nameLabel;
		Red::System::Uint32	  m_nameLength;
		Red::System::AnsiChar m_nameValue[MaximumNameLength];
	};

	MetricsNameEntry		m_names[MaximumNames];
	Red::System::Uint32		m_nameCount;
};

} }

#include "redMemoryMetricsNamePool.inl"

#endif