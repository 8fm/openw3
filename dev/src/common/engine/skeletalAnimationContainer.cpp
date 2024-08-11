/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "skeletalAnimationContainer.h"
#include "debugAnimation.h"
#include "animationLogger.h"
#include "game.h"

CAnimationMap* CAnimationMap::s_instance = nullptr;

CAnimationMap::CAnimationMap()
{
}

CAnimationMap::~CAnimationMap()
{
}

void CAnimationMap::AddAnimationSetEntry( CSkeletalAnimationSetEntry* entry )
{
	Red::Threads::CScopedLock< Red::Threads::CRWLock > lock( m_rwLock );

	AddAnimationSetEntry_Unsafe( entry );
}

void CAnimationMap::AddAnimationSetEntries( TDynArray< CSkeletalAnimationSetEntry* >& entries )
{
	Red::Threads::CScopedLock< Red::Threads::CRWLock > lock( m_rwLock );

	for ( CSkeletalAnimationSetEntry* entry : entries )
	{
		AddAnimationSetEntry_Unsafe( entry );
	}
}

void CAnimationMap::RemoveAnimationSetEntry( CSkeletalAnimationSetEntry* entry )
{
	Red::Threads::CScopedLock< Red::Threads::CRWLock > lock( m_rwLock );

	RemoveAnimationSetEntry_Unsafe( entry );
}

void CAnimationMap::RemoveAnimationSetEntries( TDynArray< CSkeletalAnimationSetEntry* >& entries )
{
	Red::Threads::CScopedLock< Red::Threads::CRWLock > lock( m_rwLock );

	for ( CSkeletalAnimationSetEntry* entry : entries )
	{
		RemoveAnimationSetEntry_Unsafe( entry );
	}
}

void CAnimationMap::RemoveAnimationSetEntry_Unsafe( CSkeletalAnimationSetEntry* entry )
{
	// Remove entry from the list
			
	auto list = m_animations.Find( entry->GetName() );
	if ( GIsCooker && list == m_animations.End() )
		return;

	ASSERT( list != m_animations.End() );
	CSkeletalAnimationSetEntry** curr = &list->m_second;
	while ( *curr )
	{
		if ( *curr == entry )
		{
			*curr = ( *curr )->m_nextInGlobalMapList;
			entry->m_nextInGlobalMapList = nullptr;
			break;
		}

		curr = &( *curr )->m_nextInGlobalMapList;
	}

	// Remove list of entries if empty

	if ( !list->m_second )
	{
		m_animations.Erase( list );
	}
}

CSkeletalAnimationSetEntry* CAnimationMap::FindAnimationRestricted( const CName& animName, const TSkeletalAnimationSetsArray& setsToMatch )
{
	Red::Threads::CScopedSharedLock< Red::Threads::CRWLock > lock( m_rwLock );

	if ( CSkeletalAnimationSetEntry* list = FindAnimationList( animName ) )
	{
		// Get animation from latest animset loaded

		for ( auto loadedSet = setsToMatch.End(), firstSet = setsToMatch.Begin(); loadedSet != firstSet; )
		{
			-- loadedSet;
			CSkeletalAnimationSetEntry* current = list;
			while ( current )
			{
				Uint32 loadedSetIdx = 0;
				if ( *loadedSet == current->GetAnimSet() )
				{
#ifdef USE_ANIMATION_LOGGER
					// Handle used animation, animset and sound events
					if ( list->GetAnimation() && GGame->GetGameplayConfig().m_logRequestedAnimations )
					{
						SAnimationLogger::GetInstance().Log( list->GetAnimation(), EAnimationLogType::EALT_RequestedAnim );
					}
#endif
					return current;
				}
				current = current->m_nextInGlobalMapList;
			}
		}
	}
	return nullptr;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CSkeletalAnimationContainer );

CSkeletalAnimationContainer::CSkeletalAnimationContainer()
{
}

CSkeletalAnimationContainer::~CSkeletalAnimationContainer()
{
	Clear();
}

#ifndef NO_DEFAULT_ANIM
void CSkeletalAnimationContainer::CreateDebugAnimation( const CSkeleton* skeleton, const CName& defaultAnimationName )
{
	m_debugAnimation.Reset();

	if ( skeleton )
	{
		CDebugAnimation* anim = new CDebugAnimation;
		anim->Initialize( skeleton );

		m_debugAnimation.Reset( new CSkeletalAnimationSetEntry );
		m_debugAnimation->SetAnimation( anim );

		anim->Bind( m_debugAnimation.Get(), NULL );
	}

	m_defaultAnimationName = defaultAnimationName;
}
#endif

void CSkeletalAnimationContainer::OnSerialize( IFile& file )
{
	// Pass to base class
	TBaseClass::OnSerialize( file );

	// Keep the reference
	file << m_loadedSets;
}

void CSkeletalAnimationContainer::AddAnimationSet( CSkeletalAnimationSet *set )
{
	m_loadedSets.PushBack( set );
}

void CSkeletalAnimationContainer::RemoveAnimationSet( CSkeletalAnimationSet* set )
{
	m_loadedSets.Remove( set );
}

void CSkeletalAnimationContainer::RebuildFromAnimationSets( const TSkeletalAnimationSetsArray& animationSets )
{
	PC_SCOPE_PIX(CSkelAnimCont_RebuildFromAnimSets);
	m_loadedSets.ClearFast();
	for ( auto it = animationSets.Begin(), end = animationSets.End(); it != end; ++it )
	{
		if ( CSkeletalAnimationSet* set = ( *it ).Get()  )
		{
			m_loadedSets.PushBack( set );
		}
	}
}

void CSkeletalAnimationContainer::Clear()
{
	m_loadedSets.ClearFast();
#ifndef NO_DEFAULT_ANIM
	m_debugAnimation.Reset();
	m_defaultAnimationName = CName::NONE;
#endif
}

CSkeletalAnimationSetEntry* CSkeletalAnimationContainer::FindAnimation( const CName& animName )
{
	// Find animation

	CSkeletalAnimationSetEntry* entry = FindAnimationRestricted( animName );
	if ( entry )
	{
		return entry;
	}
#ifndef NO_DEFAULT_ANIM
	if ( m_defaultAnimationName != animName )
	{
		return FindAnimation( m_defaultAnimationName );
	}
	else
	{
		return m_debugAnimation.Get();
	}
#else
	return nullptr;
#endif
}

const CSkeletalAnimationSetEntry* CSkeletalAnimationContainer::FindAnimation( const CName& animName ) const
{
	return const_cast< CSkeletalAnimationContainer* >( this )->FindAnimation( animName );
}

CSkeletalAnimationSetEntry* CSkeletalAnimationContainer::FindAnimationRestricted( const CName& animName )
{
	if ( CSkeletalAnimationSetEntry* entry =  CAnimationMap::GetInstance()->FindAnimationRestricted( animName, m_loadedSets ) )
	{
		return entry;
	}

#ifdef USE_ANIMATION_LOGGER
	if ( GGame->GetGameplayConfig().m_logMissingAnimations )
	{
		ERR_ENGINE( TXT("Couldn't find animation '%ls' for object '%ls'. Debug animation will be sampled."), animName.AsString().AsChar(), GetParent()->GetFriendlyName().AsChar() );
		SAnimationLogger::GetInstance().Log( animName, this, EAnimationLogType::EALT_MissingAnim );
	}
#endif

	return nullptr;
}

const CSkeletalAnimationSetEntry* CSkeletalAnimationContainer::FindAnimationRestricted( const CName& animName ) const
{
	if ( const CSkeletalAnimationSetEntry* entry =  CAnimationMap::GetInstance()->FindAnimationRestricted( animName, m_loadedSets ) )
	{
		return entry;
	}

#ifdef USE_ANIMATION_LOGGER
	if ( GGame->GetGameplayConfig().m_logMissingAnimations )
	{
		ERR_ENGINE( TXT("Couldn't find animation '%ls' for object '%ls'. Debug animation will be sampled."), animName.AsString().AsChar(), GetParent()->GetFriendlyName().AsChar() );
		SAnimationLogger::GetInstance().Log( animName, this, EAnimationLogType::EALT_MissingAnim );
	}
#endif

	return nullptr;
}
