#pragma once

class LanguagePack;

class CLocalizationStorage
{
protected:
	static const Uint32 MAX_CACHE_SIZE;
	static const Uint32 MAX_SPEECH_SIZE;

	Uint32	m_usedCacheSize;
	Uint32	m_usedSpeechMemory;

	THashMap< Uint32, LanguagePack* >		m_languagePackCache;
	mutable TDynArray< Uint32 >			m_languagePackUsageHistory;

	TDynArray< THandle< CSkeletalAnimation > >	m_lockedLipsyncs;

	// mcinek HACK
	TQueue< Uint32 >					m_toRemoveOnNextStore;

	TDynArray< Uint32 >					m_emptyPackIndices;

	Red::Threads::CMutex				m_lipsyncLockMutex;

public:
	CLocalizationStorage();

	LanguagePack* GetLanguagePack( Uint32 stringId );

	Bool GetCachedLanguagePackDuration( Uint32 stringId, Float& duration ) const;

	Bool IsPackStored( Uint32 stringId );
	void Store( LanguagePack* pack, Uint32 stringId );
	void Remove( Uint32 stringId, Bool instant = false );

	void ClearCacheMemory();
	void InvalidateEmptyPacks();

	void LockLipsync( const THandle< CSkeletalAnimation >& lipsync );
	void UnlockLipsync( const THandle< CSkeletalAnimation >& lipsync );

	void FreeSpeechMemory( Int32 neededMemory );
public:
	// Debug methods
	Uint32	GetMaxCacheSize() const;
	Uint32	GetUsedCacheSize() const;
	void	GetCachedPacks( TDynArray< const LanguagePack* >& packs, TDynArray< Uint32 >& stringIds );
	Uint32	GetLockedLipsyncsCount() const;

protected:
	void RememberPackUsage( Uint32 stringId ) const;
	void DeletePack( LanguagePack* pack, Uint32 stringId );
	void FreeCacheMemory( Uint32 neededMemory );
};
