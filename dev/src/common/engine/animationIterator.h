
#pragma once
#include "skeletalAnimation.h"

class AnimationIterator
{
	CSkeletalAnimation* m_curr;
	Bool				m_onlyLoaded;

public:
	//! Ctor
	RED_INLINE AnimationIterator( Bool onlyLoaded = false )
		: m_onlyLoaded( onlyLoaded )
	{
		m_curr = CSkeletalAnimation::AnimationList;

		if ( m_onlyLoaded && m_curr && !m_curr->IsLoaded() )
		{
			Next();
		}
	}

	//! Is current animation valid
	RED_INLINE operator Bool () const
	{
		return m_curr != NULL;
	}

	//! Advance to next
	RED_INLINE void operator++ ()
	{
		Next();
	}

	//! Get current animation
	RED_INLINE CSkeletalAnimation* operator*()
	{
		return m_curr;
	}

private:
	void Next()
	{
		ASSERT( m_curr );

		m_curr = m_curr->m_nextAnimation;

		if ( m_onlyLoaded )
		{
			while ( m_curr && !m_curr->IsLoaded() )
			{
				m_curr = m_curr->m_nextAnimation;
			}
		}
	}
};

