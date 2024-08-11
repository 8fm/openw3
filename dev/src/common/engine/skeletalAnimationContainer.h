/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../core/sortedSet.h"
#include "../core/scopedPtr.h"
#include "skeletalAnimationEntry.h"
#include "skeletalAnimationSet.h"

typedef TDynArray< THandle< CSkeletalAnimationSet > >	TSkeletalAnimationSetsArray;

/////////////////////////////////////////////////////////////////////////////////////

// Global map of all animations (from all animation sets)
class CAnimationMap
{
	friend class CSkeletalAnimationContainer;
	friend class CSkeletalAnimationSet;
private:

	mutable Red::Threads::CRWLock						m_rwLock;		// Protects map of animations
	THashMap< CName, CSkeletalAnimationSetEntry* >		m_animations;	// Map: name -> list of animation entries of that name (each from a different animation set); this list has length of 1 or 2 in most cases

	static CAnimationMap*						s_instance;

	// Gets map instance; creates it if not done before
	RED_FORCE_INLINE static CAnimationMap* GetInstance()
	{
		if ( !s_instance )
		{
			s_instance = new CAnimationMap();
		}
		return s_instance;
	}

	CAnimationMap();
	~CAnimationMap();

	// Adds animation set entry
	void AddAnimationSetEntry( CSkeletalAnimationSetEntry* entry );
	// Adds an array of animation set entries
	void AddAnimationSetEntries( TDynArray< CSkeletalAnimationSetEntry* >& entries );

	// Removes animation set entry
	void RemoveAnimationSetEntry( CSkeletalAnimationSetEntry* entry );
	// Removes an array of animation set entries
	void RemoveAnimationSetEntries( TDynArray< CSkeletalAnimationSetEntry* >& entries );

	// Finds animation in specific set by name
	RED_FORCE_INLINE CSkeletalAnimationSetEntry* FindAnimation( const CName& animName, const CSkeletalAnimationSet* set )
	{
		Red::Threads::CScopedSharedLock< Red::Threads::CRWLock > lock( m_rwLock );

		CSkeletalAnimationSetEntry* list = FindAnimationList( animName );
		while ( list && set != list->GetAnimSet() )
		{
			list = list->m_nextInGlobalMapList;
		}
		return list;
	}

	// Finds entry for animation in specific set
	RED_FORCE_INLINE CSkeletalAnimationSetEntry* FindAnimation( const CSkeletalAnimation* animation, const CSkeletalAnimationSet* set )
	{
		Red::Threads::CScopedSharedLock< Red::Threads::CRWLock > lock( m_rwLock );

		CSkeletalAnimationSetEntry* list = FindAnimationList( animation->GetName() );
		while ( list && ( list->GetAnimation() != animation || set != list->GetAnimSet() ) )
		{
			list = list->m_nextInGlobalMapList;
		}
		return list;
	}

	CSkeletalAnimationSetEntry* FindAnimationRestricted( const CName& animName, const TSkeletalAnimationSetsArray& setsToMatch );

	RED_FORCE_INLINE Bool HasAnimation( const CName& animName, const TSkeletalAnimationSetsArray& setsToMatch ) const
	{
		Red::Threads::CScopedSharedLock< Red::Threads::CRWLock > lock( m_rwLock );

		const CSkeletalAnimationSetEntry* list = FindAnimationList( animName );
		while ( list && !setsToMatch.Exist( list->GetAnimSet() ) )
		{
			list = list->m_nextInGlobalMapList;
		}
		return list != nullptr;
	}

private:
	RED_FORCE_INLINE CSkeletalAnimationSetEntry* FindAnimationList( const CName& animName )
	{
		CSkeletalAnimationSetEntry** list = m_animations.FindPtr( animName );
		return list ? *list : nullptr;
	}
	RED_FORCE_INLINE const CSkeletalAnimationSetEntry* FindAnimationList( const CName& animName ) const
	{
		return const_cast< CAnimationMap* >( this )->FindAnimationList( animName );
	}

	RED_FORCE_INLINE void AddAnimationSetEntry_Unsafe( CSkeletalAnimationSetEntry* entry )
	{
		ASSERT( !entry->m_nextInGlobalMapList );

		// Add new entry to the list (create the list if needed)

		CSkeletalAnimationSetEntry*& list = m_animations.GetRef( entry->GetName(), nullptr );
		entry->m_nextInGlobalMapList = list;
		list = entry;
	}
	void RemoveAnimationSetEntry_Unsafe( CSkeletalAnimationSetEntry* entry );
};

/////////////////////////////////////////////////////////////////////////////////////

// Animation set container
class CSkeletalAnimationContainer : public CObject
{
	DECLARE_ENGINE_CLASS( CSkeletalAnimationContainer, CObject, CF_AlwaysTransient );

protected:
	TSkeletalAnimationSetsArray	m_loadedSets;		//!< Loaded animation sets

#ifndef NO_DEFAULT_ANIM
	Red::TScopedPtr< CSkeletalAnimationSetEntry > m_debugAnimation;	//!< Debug animation
	CName m_defaultAnimationName; //!< Default animation name
#endif

public:
	CSkeletalAnimationContainer();
	~CSkeletalAnimationContainer();

	// Serialization (GC only )
	virtual void OnSerialize( IFile& file );

	// Add animation set to container
	void AddAnimationSet( CSkeletalAnimationSet *set );

	// Remove animation set from container
	void RemoveAnimationSet( CSkeletalAnimationSet *set );

	// Rebuilds animation container from given anim sets
	void RebuildFromAnimationSets( const TSkeletalAnimationSetsArray& animationSets );

	// Finds animation of given name; falls back to default animation
	CSkeletalAnimationSetEntry* FindAnimation( const CName& animName );
	const CSkeletalAnimationSetEntry* FindAnimation( const CName& animName ) const;

	// Finds animation of given name; returns nullptr if not found
	CSkeletalAnimationSetEntry* FindAnimationRestricted( const CName& animName );
	const CSkeletalAnimationSetEntry* FindAnimationRestricted( const CName& animName ) const;

	// Has animation
	RED_FORCE_INLINE Bool HasAnimation( const CName& animName ) const { return CAnimationMap::GetInstance()->HasAnimation( animName, m_loadedSets ); }

	// Clear animations
	void Clear();

	// Get animation sets
	RED_INLINE const TSkeletalAnimationSetsArray& GetAnimationSets() const { return m_loadedSets; }

#ifndef NO_DEFAULT_ANIM
	// Create debug animation
	void CreateDebugAnimation( const CSkeleton* skeleton, const CName& defaultAnimationName );
#endif
};

BEGIN_CLASS_RTTI_EX( CSkeletalAnimationContainer, CF_AlwaysTransient );
	PARENT_CLASS( CObject );
END_CLASS_RTTI();
