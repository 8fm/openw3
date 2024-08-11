/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "spawnTreeInitializationContext.h"

#include "spawnTreeInitializer.h"

void CSpawnTreeInitializationContext::PushTopInitializers( const TDynArray< ISpawnTreeInitializer* >& initializers, CSpawnTreeInstance* instance, SPopData& undo )
{
	Uint32 prevInitializersCount = m_topInitializers.Size();
	TStaticArray< ISpawnTreeInitializer*, 32 > acceptedInitializers;

	// erase conflicting initializers
	for ( auto it = initializers.Begin(), end = initializers.End(); it != end; ++it )
	{
		ISpawnTreeInitializer* initializer = *it;
		Bool isConflicted = false;
		for ( Uint32 i = 0; i < prevInitializersCount; )
		{
			if ( m_topInitializers[ i ].m_initializer->IsConflicting( initializer ) )
			{
				if ( m_topInitializers[ i ].m_initializer->IsOverridingDeepInitializers() )
				{
					isConflicted = true;
					break;
				}
				else
				{
					undo.m_erasedInitializers.PushBack( m_topInitializers[ i ] );
					m_topInitializers.RemoveAt( i );
					--prevInitializersCount;
				}
			}
			else
			{
				++i;
			}
		}
		if ( !isConflicted )
		{
			acceptedInitializers.PushBack( initializer );
		}
	}
	m_topInitializers.Resize( prevInitializersCount + acceptedInitializers.Size() );
	/// push new initializers
	undo.m_addedInitializers.Resize( acceptedInitializers.Size() );
	for ( Uint32 i = 0, n = acceptedInitializers.Size(); i != n; ++i )
	{
		TopInitializer& topInitializer = m_topInitializers[ prevInitializersCount+i ];
		topInitializer.m_initializer = acceptedInitializers[ i ];
		topInitializer.m_instance = instance;

		undo.m_addedInitializers[ i ] = topInitializer;
	}
}

void CSpawnTreeInitializationContext::PopTopInitializers( const SPopData& undo )
{
	// erase added initializers
	for ( auto it = undo.m_addedInitializers.Begin(), end = undo.m_addedInitializers.End(); it != end; ++it )
	{
		m_topInitializers.Remove( *it );
	}
	// readd erased initializers
	for ( auto it = undo.m_erasedInitializers.Begin(), end = undo.m_erasedInitializers.End(); it != end; ++it )
	{
		m_topInitializers.PushBack( *it );
	}
}
