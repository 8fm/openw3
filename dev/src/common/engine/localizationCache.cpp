#include "build.h"
#include "localizationCache.h"
#include "localizationManager.h"

#ifndef NO_EDITOR
const Uint32 CLocalizationStorage::MAX_CACHE_SIZE = 25 * 1024 * 1024; // increased by mcinek for voiceovers hack
#else
const Uint32 CLocalizationStorage::MAX_CACHE_SIZE = 10 * 1024 * 1024; // increased by mcinek for voiceovers hack
#endif
const Uint32 CLocalizationStorage::MAX_SPEECH_SIZE = 1 * 1024 * 1024; 

CLocalizationStorage::CLocalizationStorage()
	: m_usedCacheSize( 0 )
	, m_usedSpeechMemory( 0 )
{
	m_languagePackCache.Clear();
	m_languagePackUsageHistory.Clear();
	m_lipsyncLockMutex.SetSpinCount( 128 );
}

Bool CLocalizationStorage::GetCachedLanguagePackDuration( Uint32 stringId, Float& duration ) const
{
	if ( m_emptyPackIndices.Exist( stringId ) )
	{
		duration = SLocalizationManager::GetInstance().GetEmptyLanguagePack()->GetSpeechBuffer().GetDuration();
		return duration > 0.f;
	}

	LanguagePack* pack = NULL;
	m_languagePackCache.Find( stringId, pack );

	if ( pack )
	{
		duration = pack->GetSpeechBuffer().GetDuration();
		return duration > 0.f;
	}

	duration = 1.f;

	return false;
}

LanguagePack* CLocalizationStorage::GetLanguagePack( Uint32 stringId ) /* const */
{
	if ( m_emptyPackIndices.Exist( stringId ) == true )
	{
		return SLocalizationManager::GetInstance().GetEmptyLanguagePack();
	}

	LanguagePack* pack = NULL;
	m_languagePackCache.Find( stringId, pack );

	if ( pack != NULL )
	{
		Bool ret = false;

		// Check special hack. This queues are not perfect but I don't want to modify storage code
		TQueue< Uint32 > fackupCheck( m_toRemoveOnNextStore );

		while( !fackupCheck.Empty() )
		{
			Uint32 id = fackupCheck.Front();

			if ( id == stringId )
			{
				ret = true;
			}

			fackupCheck.Pop();
		}

		// Rare situation
		if ( ret )
		{
			fackupCheck = m_toRemoveOnNextStore;
			TQueue< Uint32 > newList;

			while( !fackupCheck.Empty() )
			{
				Uint32 id = fackupCheck.Front();

				if ( id == stringId )
				{
					LanguagePack* pack = NULL;
					m_languagePackCache.Find( id, pack );

					if ( pack && pack->IsLocked() )
					{
						newList.Push( id );
					}
					else
					{
						Remove( id, true );
					}
				}
				else
				{
					newList.Push( id );
				}

				fackupCheck.Pop();
			}

			m_toRemoveOnNextStore = newList;

			return NULL;
		}

		RememberPackUsage( stringId );
	}

	return pack;
}

Bool CLocalizationStorage::IsPackStored( Uint32 stringId )
{
	return m_languagePackCache.KeyExist( stringId ) || m_emptyPackIndices.Exist( stringId );
}

void CLocalizationStorage::Store( LanguagePack* pack, Uint32 stringId )
{
	if ( pack == NULL || stringId == 0 )
	{
		return;
	}

	// mcinek HACK for delaying deletion of sound data
	// I don't like it :(
	{
		// DIALOG_TOMSIN_TODO - WTF?????????????????????????????????????????????????????????????????????????
		TDynArray< Uint32 > fackupCheck;
		if ( m_toRemoveOnNextStore.Size() > 2 )
		{
			Uint32 toDelete = m_toRemoveOnNextStore.Front();
			m_toRemoveOnNextStore.Pop();

			LanguagePack* pack = NULL;
			m_languagePackCache.Find( toDelete, pack );
			if ( pack && pack->IsLocked() )
			{
				fackupCheck.PushBack( toDelete );
			}
			else
			{
				Remove( toDelete, true );
			}
		}
		for ( Uint32 i=0; i<fackupCheck.Size(); ++i )
		{
			m_toRemoveOnNextStore.Push( fackupCheck[ i ] );
		}
	}

	Uint32 packSize = pack->GetPackSize();
	Uint32 speechSize = pack->GetVoiceoverSize();

	// Empty packs optimization
	if ( packSize == 0 )
	{
		m_emptyPackIndices.PushBackUnique( stringId );
		return;
	}

	//if ( m_usedCacheSize + packSize > MAX_CACHE_SIZE )
	//{
	//	FreeCacheMemory( packSize );
	//}
	/*if ( m_usedSpeechMemory + speechSize > MAX_SPEECH_SIZE )
	{
		FreeSpeechMemory( speechSize );
	}*/

	{
		LanguagePack* pack = NULL;
		m_languagePackCache.Find( stringId, pack );

		// Memory leak!
		if ( !(pack && pack->IsLocked()) )
		{
			Remove( stringId, true );
		}
	}
// 	Remove( stringId, true );
 	m_usedCacheSize += packSize;
 	m_usedSpeechMemory += speechSize;
	
	ASSERT( m_usedCacheSize <= MAX_CACHE_SIZE );

	m_languagePackCache.Insert( stringId, pack );

	RememberPackUsage( stringId );

}

void CLocalizationStorage::Remove( Uint32 stringId, Bool instant /* = false */ )
{
	m_emptyPackIndices.Remove( stringId );

	if ( m_languagePackCache.KeyExist( stringId ) == false )
	{
		return;
	}
	
	if( instant || GIsCooker )
	{
		LanguagePack* pack = NULL;
		m_languagePackCache.Find( stringId, pack );
		if ( pack != NULL )
		{
			DeletePack(pack, stringId);

		}

		m_languagePackCache.Erase( stringId );
	}
	else
	{
		// mcinek HACK for delaying deletion of sound data
		// I don't like it :(
		m_toRemoveOnNextStore.Push( stringId );
	}
}

void CLocalizationStorage::ClearCacheMemory()
{
	for ( THashMap< Uint32, LanguagePack* >::iterator packIter = m_languagePackCache.Begin();
		packIter != m_languagePackCache.End(); ++packIter )
	{
		DeletePack( packIter->m_second, packIter->m_first );
	}
	m_languagePackCache.Clear();
	m_lockedLipsyncs.Clear();
	m_emptyPackIndices.Clear();
	
	ASSERT( m_usedCacheSize == 0 );
	ASSERT( m_languagePackCache.Empty() == true );
	ASSERT( m_languagePackUsageHistory.Empty() == true );
}

void CLocalizationStorage::InvalidateEmptyPacks()
{
	m_emptyPackIndices.Clear();
}

void CLocalizationStorage::RememberPackUsage( Uint32 stringId ) const
{
	m_languagePackUsageHistory.Remove( stringId );
	m_languagePackUsageHistory.PushBack( stringId );
}

void CLocalizationStorage::DeletePack( LanguagePack* pack, Uint32 stringId )
{
	if ( pack == NULL )
	{
		return;
	}

	m_usedCacheSize -= pack->GetPackSize();
	m_usedSpeechMemory -= pack->GetVoiceoverSize();
	SLocalizationManager::GetInstance().DeleteLanguagePack( pack );
	m_languagePackUsageHistory.Remove( stringId );
}

void CLocalizationStorage::FreeCacheMemory( Uint32 neededMemory )
{
	while ( m_usedCacheSize + neededMemory > MAX_CACHE_SIZE && m_languagePackUsageHistory.Empty() == false )
	{
		Remove( m_languagePackUsageHistory[ 0 ], true );
	}
}

void CLocalizationStorage::FreeSpeechMemory( Int32 neededMemory )
{
	TDynArray< Uint32 > packsToRemove;

	for ( TDynArray< Uint32 >::const_iterator historyIter = m_languagePackUsageHistory.Begin();
		historyIter != m_languagePackUsageHistory.End(); ++historyIter )
	{
		LanguagePack* pack = NULL;
		Uint32 packId = *historyIter;
		m_languagePackCache.Find( packId, pack );

		if ( pack->GetVoiceoverSize() > 0 )
		{
			packsToRemove.PushBackUnique( packId );
			neededMemory -= (Int32)pack->GetVoiceoverSize();
		}

		if ( neededMemory < 0 )
		{
			break;
		}
	}

	for ( TDynArray< Uint32 >::const_iterator removeIter = packsToRemove.Begin();
		removeIter != packsToRemove.End(); ++removeIter )
	{
		Remove( *removeIter, true );
	}
}

Uint32 CLocalizationStorage::GetMaxCacheSize() const
{
	return MAX_CACHE_SIZE;
}

Uint32 CLocalizationStorage::GetUsedCacheSize() const
{
	return m_usedCacheSize;
}

void CLocalizationStorage::GetCachedPacks( TDynArray< const LanguagePack* >& packs, TDynArray< Uint32 >& stringIds )
{
	packs.Clear();
	stringIds.Clear();
	if ( m_languagePackUsageHistory.Empty() == true )
	{
		return;
	}

	for ( Int32 idIndex = m_languagePackUsageHistory.Size() - 1; idIndex >= 0; --idIndex )
	{
		LanguagePack* pack = NULL;
		m_languagePackCache.Find( m_languagePackUsageHistory[ idIndex ], pack );
		if ( pack != NULL )
		{
			packs.PushBack( pack );
			stringIds.PushBack( m_languagePackUsageHistory[ idIndex ] );
		}
	}
}

void CLocalizationStorage::LockLipsync( const THandle< CSkeletalAnimation >& lipsync )
{
	if ( lipsync.IsValid() )
	{
		Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_lipsyncLockMutex );
		m_lockedLipsyncs.PushBackUnique( lipsync );
	}
}

void CLocalizationStorage::UnlockLipsync( const THandle< CSkeletalAnimation >& lipsync )
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_lipsyncLockMutex );
	m_lockedLipsyncs.Remove( lipsync );
}

Uint32 CLocalizationStorage::GetLockedLipsyncsCount() const
{
	return m_lockedLipsyncs.Size();
}
