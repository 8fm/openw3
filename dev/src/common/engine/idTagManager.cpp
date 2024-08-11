/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "idTagManager.h"
#include "../core/gameSave.h"
#include "baseEngine.h"

//////////////////////////////////////////////////////////////////////////////////////////////

CIdTagManager::CIdTagManager( Uint32 reservedSize )
	: m_reserved( reservedSize )
{
}

static Uint32 Rand32() 
{
	Uint32 a = GEngine->GetRandomNumberGenerator().Get< Uint16 >();
	a ^= GEngine->GetRandomNumberGenerator().Get< Uint16 >() << 8;
	a ^= GEngine->GetRandomNumberGenerator().Get< Uint16 >() << 16;
	a ^= GEngine->GetRandomNumberGenerator().Get< Uint16 >() << 24;
	return a;
}

IdTag CIdTagManager::GetReservedId( Uint32 index ) const
{
	ASSERT( index < m_reserved );

	IdTag reservedTag;
	reservedTag.m_isDynamic = true;
	reservedTag.m_guid = CGUID::CreateFromUint32( index + 1 );

	return reservedTag;
}

IdTag CIdTagManager::CreateFromUint64( Uint64 index ) const
{
	IdTag reservedTag;
	reservedTag.m_isDynamic = true;
	reservedTag.m_guid = CGUID::CreateFromUint64( index );

	return reservedTag;
}

IdTag CIdTagManager::Allocate()
{
	// Increment internal TagIndex
	++m_tagIndex;

	// Initialize dynamic tag
	IdTag dynamicTag;
	dynamicTag.m_isDynamic = true;
	dynamicTag.m_guid.parts.A = Rand32();
	dynamicTag.m_guid.parts.B = Rand32();
	dynamicTag.m_guid.parts.C = ( Uint32 ) ( ( m_tagIndex >> 32 ) & 0xFFFFFFFF );
	dynamicTag.m_guid.parts.D = ( Uint32 ) ( ( m_tagIndex >> 0 ) & 0xFFFFFFFF );

	// Return allocated dynamic tag
	return dynamicTag;
}

void CIdTagManager::OnGameStart( const CGameInfo& gameInfo )
{
	SGameSessionManager::GetInstance().RegisterGameSaveSection( this );
	// Reset tag index
	m_tagIndex = m_reserved + 1;

	// We have a save game, load data from it
	if ( gameInfo.m_gameLoadStream )
	{
		OnLoadGame( gameInfo.m_gameLoadStream );
	}
}

void CIdTagManager::OnGameEnd( const CGameInfo& gameInfo )
{
	SGameSessionManager::GetInstance().UnregisterGameSaveSection( this );
}

bool CIdTagManager::OnSaveGame( IGameSaver* saver )
{
	CGameSaverBlock block( saver, CNAME(idTagManager) );

	// Save tag count
	saver->WriteValue< Uint64 >( CNAME(tagIndex), m_tagIndex );
	return true;
}

void CIdTagManager::OnLoadGame( IGameLoader* loader )
{
	CGameSaverBlock block( loader, CNAME(idTagManager) );

	// Load tag count
	m_tagIndex = loader->ReadValue< Uint64 >( CNAME(tagIndex) );
}

//////////////////////////////////////////////////////////////////////////////////////////////

