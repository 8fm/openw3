#include "build.h"
#include "AnimationsCache.h"

/*CAnimationsCache::CAnimationsCache(void) :
	 m_currentSize(0)
	,m_maxSize(30*1024*1024)
	,m_NumTickLoad(0)
	,m_NumTickEvict(0)
{
}

CAnimationsCache::~CAnimationsCache(void)
{
	m_animationsLoaded.Clear();
	m_animationsNotLoaded.Clear();
	m_animationsAsyncLoading.Clear();
}

void CAnimationsCache::OnCreate(CSkeletalAnimation* animation)
{
	m_animationsNotLoaded.PushBack(animation);
}

void CAnimationsCache::OnDelete(CSkeletalAnimation* animation)
{
	// Remove from loaded list or not loaded list
	int datasize = animation->GetDataSize();
	if (m_animationsLoaded.Remove(animation))
	{
		m_currentSize -= datasize;
	}
	else
		m_animationsNotLoaded.Remove(animation);
}
	
void CAnimationsCache::Prefetch(const CName& animsetName, const CName& animName)
{
	// Search for animation in 'not loaded' list
	for (Uint32 i=0; i<m_animationsNotLoaded.Size(); ++i)
	{
		CSkeletalAnimation* animation = m_animationsNotLoaded[i];
		if (animation->GetAnimsetName() == animsetName && animation->GetName() == animName)
		{
			LoadAsync(animation);
			break;
		}
	}
}

void CAnimationsCache::Load(CSkeletalAnimation* animation)
{
	PC_CHECKER_SCOPE( 0.02f, TXT("LOADING"), TXT("Loading animation %s from %s"), animation->GetName().AsChar(), animation->GetAnimsetName().AsChar() );

	if ((int)animation->GetDataSize() + m_currentSize > m_maxSize)
	{
		// Need to evict something
		Evict((int)animation->GetDataSize() + m_currentSize - m_maxSize);
	}

	animation->Load();

	// Increment size and add to list.
	m_currentSize += animation->GetDataSize();
	m_animationsLoaded.PushBack(animation);

	++m_NumTickLoad;

	m_animationsNotLoaded.Remove(animation);
}
	
void CAnimationsCache::LoadAsync(CSkeletalAnimation* animation)
{
	if ((int)animation->GetDataSize() + m_currentSize > m_maxSize)
	{
		// Need to evict something
		Evict((int)animation->GetDataSize() + m_currentSize - m_maxSize);
	}

	StartLoadAsync(animation);

	m_animationsNotLoaded.Remove(animation);
}

void CAnimationsCache::Unload(CSkeletalAnimation* animation)
{
	int datasize = animation->GetDataSize();
	if (m_animationsLoaded.Remove(animation))
	{
		m_currentSize -= datasize;
		m_animationsNotLoaded.PushBack(animation);
	}
}

static int CompareByLastUseTime(const void *a, const void* b)
{
	CSkeletalAnimation* animA = * ( CSkeletalAnimation** ) a;
	CSkeletalAnimation* animB = * ( CSkeletalAnimation** ) b;
	if ( animA->GetLastUseTime() > animB->GetLastUseTime() ) return -1;
	if ( animA->GetLastUseTime() < animB->GetLastUseTime() ) return 1;
	return 0;
}

void CAnimationsCache::Evict(int dataNeeded)
{
	Double currentTime = Red::System::Clock::GetInstance().GetTimer().GetSeconds();
	qsort( m_animationsLoaded.TypedData(), m_animationsLoaded.Size(), sizeof( CSkeletalAnimation* ), &CompareByLastUseTime );

	// Remove some old animations
	while(dataNeeded>0 && m_animationsLoaded.Size()>0)
	{
		// Remove till we get enough free space.
		CSkeletalAnimation* anim = m_animationsLoaded[m_animationsLoaded.Size()-1];
		if (currentTime-anim->GetLastUseTime()<1.0)
			//Allow buffer to grow, don't remove animations used in last second 
			break;

		dataNeeded -= (int)anim->GetDataSize();
		m_currentSize -= (int)anim->GetDataSize();
		anim->Unload();

		m_animationsLoaded.Erase(m_animationsLoaded.End()-1);
		m_animationsNotLoaded.PushBack(anim);
		++m_NumTickEvict;
	}
}

void CAnimationsCache::Tick( Float timeDelta )
{
	m_NumTickLoad = 0;
	m_NumTickEvict = 0;

	// Check async loading status
	TList<SAsyncAnimationToken>::iterator it = m_animationsAsyncLoading.Begin();
	while ( it != m_animationsAsyncLoading.End() )
	{
		SAsyncAnimationToken token = *it;
		if ( token.asyncLoadToken->HasEnded() )
		{
			if ( token.asyncLoadToken->HasFinishedWithoutErrors() )
			{
				// Extract loaded data
				token.animation->OnLoadAsyncEnd( token.asyncLoadToken->GetDataBuffer() );

				// Add to list
				m_animationsLoaded.PushBack(token.animation);
			}

			// Remove from list
			it = m_animationsAsyncLoading.Erase( it );
		}
		else
		{
			++it;
		}
	} 
}

void CAnimationsCache::StartLoadAsync(CSkeletalAnimation* animation)
{
	SAsyncAnimationToken token;

	// Get loading token
	token.asyncLoadToken = animation->OnLoadAsyncStart();
	token.animation = animation;

	// Increment size and add to loading list
	m_currentSize += animation->GetDataSize();
	m_animationsAsyncLoading.PushBack( token );
}*/