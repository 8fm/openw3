/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../core/gameSave.h"
#include "gameSaveFile.h"
#include "gameSession.h"

/// Game data storage
template< EMemoryClass memClass, RED_CONTAINER_POOL_TYPE memPool >
class CGameDataStorage : public IGameDataStorage
{
	DECLARE_CLASS_MEMORY_POOL( MemoryPool_Default, memClass );

public:
	typedef TDynArray< Uint8, memClass > TStorageType;

protected:
	TStorageType m_data;

public:
	//! Get data
	RED_INLINE TStorageType& GetData() { return m_data; }

public:

	CGameDataStorage()
	{}

	//! Get data size
	virtual Uint32 GetSize() const
	{
		return m_data.Size();
	}

	//! Get data size
	virtual const void* GetData() const
	{
		return m_data.Data();
	}

	//! Create memory writer
	virtual CGameStorageWriter* CreateWriter()
	{
		m_data.ClearFast();
		return new CGameStorageWriter( new CMemoryFileWriter ( m_data ), SGameSessionManager::GetInstance().GetCNamesRemapper() );	
	}

	//! Create memory reader
	virtual ISaveFile* CreateReader() const
	{
		return new CGameStorageReader( new CMemoryFileReader( (const Uint8*) m_data.Data(), m_data.DataSize(), 0 ), SGameSessionManager::GetInstance().GetCNamesRemapper() );
	}

	//! Reserve memory for storage
	virtual void Reserve( Uint32 bytes ) override
	{
		m_data.Reserve( bytes );
	}

	static IGameDataStorage* Create( Uint32 initialReservation, Uint32 initialDataSize )
	{
		CGameDataStorage< memClass, memPool >* storage = new CGameDataStorage< memClass, memPool >();
		if( initialReservation != 0 )
		{
			storage->Reserve( initialReservation );
		}
		if( initialDataSize != 0 )
		{
			storage->GetData().ResizeFast( initialDataSize );
		}
		return storage;
	}
};