/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#include "gameSession.h"
#include "idTag.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////////

/// Dynamic tag manager 
class CIdTagManager : public IGameSaveSection
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Gameplay );

private:
	Uint64		m_tagIndex;			//!< Current tag index
	Uint32		m_reserved;			//!< Number of reserved id's

public:
	CIdTagManager( Uint32 reservedSize = 10 );

	//! Allocates a special ID for the player
	IdTag GetReservedId( Uint32 index ) const;

	IdTag CreateFromUint64( Uint64 index ) const;

	//! Allocate IdTag from pool, will return empty tag if pool was exhausted
	IdTag Allocate();

public:
	//! Initialize at game start, called directly in StartGame, but should always be implemented
	virtual void OnGameStart( const CGameInfo& gameInfo );

	//! Shutdown at game end, called directly in EndGame, but should always be implemented
	virtual void OnGameEnd( const CGameInfo& gameInfo );

	//! Called when we are creating saving the save game
	virtual bool OnSaveGame( IGameSaver* saver );

	//! Restore state
	virtual void OnLoadGame( IGameLoader* loader );
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////
