namespace Red { namespace MemoryFramework {

	/////////////////////////////////////////////////////////////////
	// CTor
	//
	template< Red::System::Uint32 MaximumGroups >
	RED_INLINE MemoryClassGroups< MaximumGroups >::MemoryClassGroups()
		: m_nextFreeMemoryClassIndex( 0 )
		, m_groupCount( 0 )
	{
		static_assert( MaximumGroups > 0, "MaximumGroups cannot be zero" );
		Red::System::MemorySet( m_freeMemoryClassEntries, 0, sizeof( m_freeMemoryClassEntries ) );
		Red::System::MemorySet( m_groupEntries, 0, sizeof( m_groupEntries ) );
	}

	/////////////////////////////////////////////////////////////////
	// DTor
	//
	template< Red::System::Uint32 MaximumGroups >
	RED_INLINE MemoryClassGroups< MaximumGroups >::~MemoryClassGroups()
	{

	}

	/////////////////////////////////////////////////////////////////
	// AddGroupDefinition
	//
	template< Red::System::Uint32 MaximumGroups >
	RED_INLINE void MemoryClassGroups< MaximumGroups >::AddGroupDefinition( const AnsiChar* groupLabel, MemoryClass* classesInGroup, Uint32 classCount )
	{
		RED_MEMORY_ASSERT( groupLabel, "Group label is null" );
		RED_MEMORY_ASSERT( classesInGroup, "null array passed to metrics class group" );
		RED_MEMORY_ASSERT( classCount, "do not register groups with no classes" );
		RED_MEMORY_ASSERT( m_nextFreeMemoryClassIndex + classCount <= k_MaximumMemoryClasses, "Ran out of memory class group entries, adding duplicate classes?" );
		RED_MEMORY_ASSERT( m_groupCount < MaximumGroups, "Increase MaximumGroups" );
		m_groupEntries[ m_groupCount ].m_groupLabel = groupLabel;
		m_groupEntries[ m_groupCount ].m_memClassCount = classCount;
		m_groupEntries[ m_groupCount ].m_memClassStartIndex = m_nextFreeMemoryClassIndex;

		for( Uint32 i=0; i<classCount; ++i )
		{
			m_freeMemoryClassEntries[ m_nextFreeMemoryClassIndex + i ] = classesInGroup[ i ];
		}

		m_nextFreeMemoryClassIndex += classCount;
		++m_groupCount;
	}

	/////////////////////////////////////////////////////////////////
	// GetGroupCount
	//
	template< Red::System::Uint32 MaximumGroups >
	RED_INLINE Uint32 MemoryClassGroups< MaximumGroups >::GetGroupCount() const
	{
		return m_groupCount;
	}

	/////////////////////////////////////////////////////////////////
	// GetGroupLabel
	//
	template< Red::System::Uint32 MaximumGroups >
	RED_INLINE const AnsiChar* MemoryClassGroups< MaximumGroups >::GetGroupLabel( Uint32 groupIndex )
	{
		RED_MEMORY_ASSERT( groupIndex < m_groupCount, "Invalid group index" );
		return m_groupEntries[ groupIndex ].m_groupLabel;
	}

	/////////////////////////////////////////////////////////////////
	// GetClassesInGroup
	//
	template< Red::System::Uint32 MaximumGroups >
	RED_INLINE Uint32 MemoryClassGroups< MaximumGroups >::GetClassesInGroup( Uint32 groupIndex )
	{
		RED_MEMORY_ASSERT( groupIndex < m_groupCount, "Invalid group index" );
		return m_groupEntries[ groupIndex ].m_memClassCount;
	}

	/////////////////////////////////////////////////////////////////
	// GetMemoryClassInGroup
	//
	template< Red::System::Uint32 MaximumGroups >
	RED_INLINE MemoryClass MemoryClassGroups< MaximumGroups >::GetMemoryClassInGroup( Uint32 groupIndex, Uint32 memClassIndex )
	{
		RED_MEMORY_ASSERT( groupIndex < m_groupCount, "Invalid group index" );
		RED_MEMORY_ASSERT( memClassIndex < m_groupEntries[ groupIndex ].m_memClassCount, "Invalid memory class index" );
		return m_freeMemoryClassEntries[ m_groupEntries[ groupIndex ].m_memClassStartIndex + memClassIndex ];		// Ewww
	}

} }