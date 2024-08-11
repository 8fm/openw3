/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "skeletalAnimationContainer.h"
#include "animatedComponent.h"
#include "skeletalAnimationSet.h"
#include "skeleton.h"

/////////////////////////////////////////////////////////////////////////////////////

/// Iterator for iterating over bones in animated component 
//dex++
class BoneIterator : public Red::System::NonCopyable
//dex--
{
private:
	//dex++
	const CSkeleton*	m_skeleton;
	//dex--
	Uint32				m_numBones;
	Uint32				m_index;

public:
	RED_INLINE BoneIterator( const CAnimatedComponent* ac )
		//dex++
		: m_skeleton( ac->GetSkeleton() )
		//dex--
		, m_numBones( 0 )
		, m_index( 0 ) 
	{
		//dex++
		if ( NULL != m_skeleton )
		{
			m_numBones = m_skeleton->GetBonesNum();
		}
		//dex-
	};

	RED_INLINE BoneIterator( const CSkeleton* skeleton )
		//dex++
		: m_skeleton( skeleton )
		//dex--
		, m_numBones( 0 )
		, m_index( 0 ) 
	{
		//dex++
		if ( NULL != skeleton )
		{
			m_numBones = skeleton->GetBonesNum();
		}
		//dex-
	};

	RED_INLINE void operator++()
	{
		++m_index;
	}

	RED_INLINE operator Bool () const
	{
		return m_index < m_numBones;
	}

	RED_INLINE Int32 GetParent() const
	{
		ASSERT( m_index < m_numBones );
		//dex++
		ASSERT( NULL != m_skeleton );
		return m_skeleton->GetParentBoneIndex( m_index );
		//dex--
	}

	RED_INLINE const AnsiChar* GetParentName() const
	{
		const Int32 parentIndex = GetParent();
		ASSERT( parentIndex != -1 && parentIndex < (Int32)m_numBones );
		//dex++
		ASSERT( NULL != m_skeleton );
		return m_skeleton->GetBoneNameAnsi( parentIndex );
		//dex--
	}

	RED_INLINE const AnsiChar* GetName() const
	{
		ASSERT( m_index < m_numBones );
		//dex++
		ASSERT( NULL != m_skeleton );
		return m_skeleton->GetBoneNameAnsi( m_index );
		//dex--
	}

	RED_INLINE Uint32 GetIndex() const
	{
		return m_index;
	}
};

//////////////////////////////////////////////////////////////////////////

/// Iterator for iterating over all root bone's children - slow, use it and cache results
//dex++
class BoneChildrenIterator : public Red::System::NonCopyable
//dex--
{
private:
	//dex++
	const CSkeleton*		m_skeleton;
	//dex--
	Int32					m_rootBone;
	Int32					m_index;
	Int32					m_numBones;

public:
	RED_INLINE BoneChildrenIterator( const CAnimatedComponent* ac, Int32 rootBone )
		//dex++
		: m_skeleton( ac->GetSkeleton() )
		//dex--
		, m_numBones( 0 )
		, m_index( -1 )
		, m_rootBone( rootBone )
	{
		//dex+
		if ( NULL != m_skeleton )
		{
			m_numBones = m_skeleton->GetBonesNum();
		}
		//dex--

		Next();
	};

	RED_INLINE BoneChildrenIterator( const CSkeleton* skeleton, Int32 rootBone )
		//dex++
		: m_skeleton( skeleton )
		//dex--
		, m_numBones( 0 )
		, m_index( -1 ) 
		, m_rootBone( rootBone )
	{
		//dex++
		if ( NULL != skeleton )
		{
			m_numBones = skeleton->GetBonesNum();
		}
		//dex--

		Next();
	};

	RED_INLINE void operator++()
	{
		Next();
	}

	RED_INLINE operator Bool () const
	{
		return m_rootBone != -1 && m_index < m_numBones;
	}

	RED_INLINE Int32 operator* () const
	{
		ASSERT( m_index > 0 );
		ASSERT( m_index < m_numBones );

		return m_index;
	}

private:
	void Next()
	{
		while ( 1 )
		{
			// Go to next bone
			m_index += 1;

			// Last bone?
			if ( m_index >= m_numBones )
			{
				ASSERT( m_index == m_numBones );
				return;
			}

			// Is parent valid
			if ( IsRootsChild( m_index ) )
			{
				return;
			}
		}
	}

	Bool IsRootsChild( Int32 index ) const
	{
		while( 1 )
		{
			ASSERT( NULL != m_skeleton );
			const Int32 parent = m_skeleton->GetParentBoneIndex( index );
			if ( parent == m_rootBone )
			{
				return true;
			}
			else if ( parent == -1 )
			{
				return false;
			}

			index = parent;
		}
	}
};

/////////////////////////////////////////////////////////////////////////////////////

/// Iterator for bones in chain
//dex++
class BoneChainIterator : public Red::System::NonCopyable
//dex--
{
private:
	//dex++
	const CSkeleton*		m_skeleton;
	//dex--
	Uint32					m_firstBoneIndex;
	Uint32					m_lastBoneIndex;
	Uint32					m_lastParentBoneIndex;
	Uint32					m_index;

public:
	RED_INLINE BoneChainIterator( const CSkeleton* skeleton, const Char* firstBone, const Char* lastBone )
		//dex++
		: m_skeleton( skeleton )
		//dex--
		, m_index( 0 )		 
		, m_firstBoneIndex( 0 )
		, m_lastBoneIndex( 0 )
		, m_lastParentBoneIndex( 0 )
	{
		//dex++
		if ( NULL != skeleton )
		{
			const Int32 firstBoneIndex = skeleton->FindBoneByName( firstBone );
			const Int32 lastBoneIndex = skeleton->FindBoneByName( lastBone );

			if ( firstBoneIndex != -1 && lastBoneIndex != -1 && firstBoneIndex < lastBoneIndex )
			{
				Int32 currentParent = lastBoneIndex;

				Bool depthOk = false;

				while ( currentParent != -1 )
				{
					if ( currentParent == firstBoneIndex )
					{
						depthOk = true;
						break;
					}

					currentParent = skeleton->GetParentBoneIndex( currentParent );
				}

				if ( depthOk )
				{
					m_firstBoneIndex = firstBoneIndex;
					m_lastBoneIndex = lastBoneIndex;

					m_lastParentBoneIndex = skeleton->GetParentBoneIndex( m_firstBoneIndex );

					m_index = m_lastBoneIndex;
				}
			}
		}
		//dex--
	};

	RED_INLINE void operator++()
	{
		m_index = GetParent();
		ASSERT( ( m_firstBoneIndex <= m_index && m_index <= m_lastBoneIndex ) || m_index == m_lastParentBoneIndex );
	}

	RED_INLINE operator Bool () const
	{
		ASSERT( ( m_firstBoneIndex <= m_index && m_index <= m_lastBoneIndex ) || m_index == m_lastParentBoneIndex );
		return m_index != m_lastParentBoneIndex;
	}

	RED_INLINE Int32 GetParent() const
	{
		ASSERT( m_firstBoneIndex <= m_index && m_index <= m_lastBoneIndex );
		ASSERT( NULL != m_skeleton );
		return m_skeleton->GetParentBoneIndex( m_index );
	}

	RED_INLINE const AnsiChar* GetParentName() const
	{
		const Int32 parentIndex = GetParent();
		ASSERT( parentIndex != -1 );
		//dex++
		ASSERT( NULL != m_skeleton );
		return m_skeleton->GetBoneNameAnsi(parentIndex);
		//dex--
	}

	RED_INLINE const AnsiChar* GetName() const
	{
		ASSERT( m_firstBoneIndex <= m_index && m_index <= m_lastBoneIndex );
		//dex++
		ASSERT( NULL != m_skeleton );
		return m_skeleton->GetBoneNameAnsi(m_index);
		//dex--
	}

	RED_INLINE Uint32 GetIndex() const
	{
		return m_index;
	}
};

//////////////////////////////////////////////////////////////////////////

//dex++
class ComponentAnimationIterator : public Red::System::NonCopyable
//dex--
{
	const TSkeletalAnimationSetsArray&							m_sets;
	TSkeletalAnimationSetsArray::const_iterator					m_setIterator;
	TDynArray< CSkeletalAnimationSetEntry* >::const_iterator	m_animIterator;

public:
	RED_INLINE ComponentAnimationIterator( const CAnimatedComponent* ac )
		: m_sets( ac->GetAnimationContainer()->GetAnimationSets() )
	{
		m_setIterator = m_sets.Begin();
		GoToFirstSetAnimation();
	}

	RED_INLINE void operator++()
	{
		if ( m_setIterator != m_sets.End() )
		{
			++m_animIterator;
			if ( m_animIterator != ( *m_setIterator )->GetAnimations().End() )
			{
				return;
			}
			++m_setIterator;
			GoToFirstSetAnimation();
		}
	}

	RED_INLINE operator Bool () const
	{
		return m_setIterator != m_sets.End();
	}

	RED_INLINE const CSkeletalAnimationSetEntry* operator*()
	{
		ASSERT( m_sets.End() != m_setIterator );
		return *m_animIterator;
	}

private:
	void GoToFirstSetAnimation()
	{
		while ( m_setIterator != m_sets.End() && !( *m_setIterator )->GetNumAnimations() )
		{
			++m_setIterator;
		}
		if ( m_setIterator != m_sets.End() )
		{
			m_animIterator = ( *m_setIterator ).Get()->GetAnimations().Begin();
		}
	}
};

//////////////////////////////////////////////////////////////////////////

class ComponentAnimsetIterator : public Red::System::NonCopyable
{
private:
	const TSkeletalAnimationSetsArray&			m_sets;
	TSkeletalAnimationSetsArray::const_iterator	m_setIt;
	//EAnimationSetType							m_type;

public:
	RED_INLINE ComponentAnimsetIterator( const CAnimatedComponent* ac /*, EAnimationSetType type*/ )
		: m_sets( ac->GetAnimationContainer()->GetAnimationSets() )
		//, m_type( type )
	{
		m_setIt = m_sets.Begin();
		while ( m_setIt != m_sets.End() /*&& ( *m_setIt )->GetType() != m_type*/ )
		{
			++m_setIt;
		}
	};

	RED_INLINE void operator++()
	{
		if ( m_setIt != m_sets.End() )
		{
			++m_setIt;
			while ( m_setIt != m_sets.End() /*&& ( *m_setIt )->GetType() != m_type*/ )
			{
				++m_setIt;
			}
		}
	}

	RED_INLINE operator Bool () const
	{
		return m_setIt != m_sets.End();
	}

	RED_INLINE const CSkeletalAnimationSet* operator*()
	{
		ASSERT( m_setIt != m_sets.End() );
		return ( *m_setIt ).Get();
	}
};
