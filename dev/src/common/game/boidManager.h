#pragma once

#include "boidLairEntity.h"

////////////////////////////////////////////////////////////////////////////
// Global 'manager' object that handles all spatial boid lair tests.
class CBoidManager
{
public:
	template < class Functor >
	RED_INLINE void IterateLairs( Functor& functor );

	template < class Functor >
	RED_INLINE void IterateLairs( const Vector& location, Bool testBothArea, Functor& functor );

	RED_INLINE IBoidLairEntity* FindLair( const Vector& location );

	void AddLair( IBoidLairEntity* lair );
	void RemoveLair( IBoidLairEntity* lair );

	static CBoidManager* GetInstance();

	void OnLairActivated()													{ ++m_activeLairs; }
	void OnLairDeactivated()												{ ASSERT( m_activeLairs ); --m_activeLairs; }

	Uint32 GetActiveLairs() const												{ return m_activeLairs; }

protected:
	// TODO: r-trees
	TDynArray< IBoidLairEntity* >			m_lairs;
	Uint32									m_activeLairs;
};
template < class Functor >
RED_INLINE void CBoidManager::IterateLairs( Functor& functor )
{
	for( auto it = m_lairs.Begin(), end = m_lairs.End(); it != end; ++it )
	{
		IBoidLairEntity* lair = *it;
		functor( lair );
	};
}

template < class Functor >
RED_INLINE void CBoidManager::IterateLairs( const Vector& location, Bool testBothArea, Functor& functor )
{
	auto modifiedFunctor =
		[ &location, &testBothArea, &functor ] ( IBoidLairEntity* lair )
	{
		if ( lair->ContainsPoint( location, testBothArea ) )
		{
			functor( lair );
		}
	};

	IterateLairs( modifiedFunctor );
}

RED_INLINE IBoidLairEntity* CBoidManager::FindLair( const Vector& location )
{
	for( auto it = m_lairs.Begin(), end = m_lairs.End(); it != end; ++it )
	{
		IBoidLairEntity* lair = *it;
		if ( lair->ContainsPoint( location, false ) )
		{
			return lair;
		}
	};
	return NULL;
}