/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#ifndef _RED_MEMORY_METRICS_CLASS_GROUP_H
#define _RED_MEMORY_METRICS_CLASS_GROUP_H

#include "redMemoryFrameworkTypes.h"

namespace Red { namespace MemoryFramework {

	// A simple metrics structure used to organise memory classes into labeled groups
	template< Red::System::Uint32 MaximumGroups = 16 >
	class MemoryClassGroups
	{
	public:
		MemoryClassGroups();
		~MemoryClassGroups();

		void AddGroupDefinition( const AnsiChar* groupLabel, MemoryClass* classesInGroup, Uint32 classCount );
		Uint32 GetGroupCount() const;
		
		const AnsiChar* GetGroupLabel( Uint32 groupIndex );
		Uint32 GetClassesInGroup( Uint32 groupIndex );
		MemoryClass GetMemoryClassInGroup( Uint32 groupIndex, Uint32 memClassIndex );

	private:
		struct GroupEntry
		{
			const AnsiChar* m_groupLabel;
			Uint32 m_memClassStartIndex;
			Uint32 m_memClassCount;
		};

		MemoryClass m_freeMemoryClassEntries[ k_MaximumMemoryClasses ];					// Memory classes are allocated to groups from this buffer
		Uint32 m_nextFreeMemoryClassIndex;

		GroupEntry m_groupEntries[ MaximumGroups ];
		Uint32 m_groupCount;
	};

} }

#include "redMemoryMetricsClassGroup.inl"

#endif