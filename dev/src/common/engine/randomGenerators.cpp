/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "randomGenerators.h"
#include "baseEngine.h"

Uint32 CRandomIndexPool::Rand()
{
	const Uint32 size = m_history.Size();
	if ( size == 0 )
	{
		return 0;
	}

	if ( m_freeSize > 0 )
	{
		Int32 steps = GEngine->GetRandomNumberGenerator().Get< Int32 >( m_freeSize );
		for ( Uint32 i=0; i<size; ++i )
		{
			if ( m_history[ i ] == false )
			{
				if ( steps == 0 )
				{
					m_history[ i ] = true;

					--m_freeSize;

					//////////////////////////////////////////////////////////////////////////
#ifndef NDEBUG
					for ( Uint32 j=0; j<m_temp.Size(); ++j )
					{
						if ( m_temp[ j ] == i )
						{
							ASSERT( !"IndexPool returned the same value twice!" );
						}
					}
					m_temp.PushBack( i );
#endif
					//////////////////////////////////////////////////////////////////////////

					if ( m_freeSize == 0 )
					{
						Reset( i );
					}

					return m_min + i;
				}

				--steps;
			}
		}

		ASSERT( 0 );
		return 0;
	}
	else
	{
		ASSERT( 0 );
		return 0;
	}
}