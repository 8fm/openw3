#include "build.h"
#include "containerManager.h"
#include "..\core\gameSave.h"
#include "gameSaveManager.h"
#include "gameDataStorage.h"

CContainerManager::SContainerData::SContainerData()
	: m_lootBitmask( 0xFFFF )
{
}

void CContainerManager::SContainerData::StreamLoad( ISaveFile* loader, Uint32 version )
{
	*loader << m_lootBitmask;
}

void CContainerManager::SContainerData::StreamSave( ISaveFile* saver )
{
	*saver << m_lootBitmask;
}

void CContainerManager::Reset()
{
	m_containerData.Clear();
}

Bool CContainerManager::WasItemLooted( IdTag idTag, Uint32 itemID ) const
{
	if( ( itemID > 16 ) || !idTag.IsValid() )
	{
		return false; // all items above max quantity will be restored
	}

	auto it = m_containerData.Find( idTag );
	if( it == m_containerData.End() )
	{
		return false;
	}

	const SContainerData& containerData = it->m_second;
	Uint32 bitID = 1;
	bitID <<= ( itemID - 1 );

	return ( ( bitID & containerData.m_lootBitmask ) == 0 );
}

void CContainerManager::NotifyItemLooted( IdTag idTag, Uint32 itemID, Bool looted /* = true */ )
{
	if( ( itemID > 16 ) || !idTag.IsValid() )
	{
		return;
	}

	Uint32 bitID = 1;
	bitID <<= ( itemID - 1 );

	auto it = m_containerData.Find( idTag );
	if( it == m_containerData.End() )
	{
		if ( looted )
		{
			SContainerData containerData;
			containerData.m_lootBitmask &= ~bitID;
			m_containerData.Insert( idTag, containerData );
		}
		// if there's no SContainerData for specified idTag it means that no items were looted
	}
	else
	{
		SContainerData& containerData = it->m_second;
		if ( looted )
		{
			containerData.m_lootBitmask &= ~bitID;
		}
		else
		{
			containerData.m_lootBitmask |= bitID;
		}
	}
}

void CContainerManager::NotifyQuestItemLooted( IdTag idTag, Uint32 itemID )
{
	if( ( itemID > 15 ) || !idTag.IsValid() )
	{
		return;
	}

	Uint16 targetBitID = 1;
	Uint16 sourceBitID = 1;
	targetBitID <<= ( itemID - 1 );
	sourceBitID <<= ( itemID );

	auto it = m_containerData.Find( idTag );
	if( it == m_containerData.End() )
	{
		return;
	}

	SContainerData& containerData = it->m_second;

	for( Uint32 i = 0; i < ( 16 - itemID ); ++i )
	{
		Uint16 sourceBitVal = sourceBitID & ~( containerData.m_lootBitmask );
		if( sourceBitVal )
		{
			containerData.m_lootBitmask &= ~targetBitID;
		}
		else
		{
			containerData.m_lootBitmask |= targetBitID;
		}

		targetBitID <<= 1;
		sourceBitID <<= 1;
	}
}

void CContainerManager::ResetContainerData( IdTag idTag )
{
	if( !idTag.IsValid() )
	{
		return;
	}

	auto it = m_containerData.Find( idTag );
	if( it == m_containerData.End() )
	{
		return;
	}


	SContainerData& containerData = it->m_second;
	containerData.m_lootBitmask = 0xFFFF;
}

void CContainerManager::StreamLoad( ISaveFile* loader, Uint32 version )
{
	Reset();

	Uint32 count = 0;
	*loader << count;

	for( Uint32 i = 0; i < count; ++i )
	{
		IdTag tmpTag;
		*loader << tmpTag;

		SContainerData tmpData;
		tmpData.StreamLoad( loader, version );
		m_containerData.Insert( tmpTag, tmpData );
	}
}

void CContainerManager::StreamSave( ISaveFile* saver )
{
	Uint32 count = m_containerData.Size();
	*saver << count;

	for( auto it = m_containerData.Begin(); it != m_containerData.End(); ++it )
	{
		*saver << ( it->m_first );
		it->m_second.StreamSave( saver );
	}
}

void CContainerManager::Load( IGameLoader* loader )
{
	IGameDataStorage* data = loader->ExtractDataStorage();
	if( data )
	{
		Uint32 version = 0;
		ISaveFile* directStream = nullptr;
		IGameLoader* loader = SGameSaveManager::GetInstance().CreateLoader( data, &directStream, &version );
		if ( loader )
		{
			StreamLoad( directStream, version );
			delete loader;
		}

		delete data;
	}
}

void CContainerManager::Save( IGameSaver* saver )
{
	IGameDataStorage* data = CGameDataStorage< MC_Gameplay, MemoryPool_GameSave >::Create( 1024, 0 );

	if ( data )
	{
		ISaveFile* directStream = nullptr;
		IGameSaver* mysaver = SGameSaveManager::GetInstance().CreateSaver( data, &directStream );
		
		if ( mysaver )
		{
			StreamSave( directStream );
			delete mysaver;
		}

		saver->AddStorageStream( data );
		delete data;
	}
}
