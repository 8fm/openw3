#pragma once

// Memory pool manager for havok animation data.
// Based on time eviction behaves like ring buffer.
/*class CAnimationsCache
{
	struct SAsyncAnimationToken
	{
		CJobLoadData*			asyncLoadToken;
		CSkeletalAnimation*		animation;
	};

public:
	CAnimationsCache(void);
	~CAnimationsCache(void);

public:
	// Return curent size in bytes.
	inline int GetCurrentSize() { return m_currentSize; }

	// Return free size in bytes.
	inline int GetFreeSize() { return m_maxSize - m_currentSize; }
	
	// Return max size in bytes.
	inline int GetMaxSize() { return m_maxSize; }

public:
	// Call just after animation was serialized
	void OnCreate(CSkeletalAnimation* animation);
	
	// Call just before removing animation
	void OnDelete(CSkeletalAnimation* animation);

public:
	// Prefetch animation from given animset
	// TODO: Who and how will call it?
	void Prefetch(const CName& animsetName, const CName& animName);

	// Load havok data for given animation.
	void Load(CSkeletalAnimation* animation);
	
	// Load havok data for given animation asynchronously.
	void LoadAsync(CSkeletalAnimation* animation);

	// Unload havok data for given animation.
	void Unload(CSkeletalAnimation* animation);

public:
	void Tick( Float timeDelta );

	int GetNumTickLoad() { return m_NumTickLoad; }
	int GetNumTickEvict() { return m_NumTickEvict; }

protected:
	// Evict last time used animations to get dataNeeded bytes.
	void Evict(int dataNeeded);

	// Create async loading task.
	void StartLoadAsync(CSkeletalAnimation* animation);

protected:

	TDynArray< CSkeletalAnimation* >		m_animationsNotLoaded;
	TList< SAsyncAnimationToken >			m_animationsAsyncLoading;
	TDynArray< CSkeletalAnimation* >		m_animationsLoaded;
	int										m_currentSize;
	int										m_maxSize;

	int										m_NumTickLoad;
	int										m_NumTickEvict;
};

typedef TSingleton< CAnimationsCache > SAnimationsCache;*/