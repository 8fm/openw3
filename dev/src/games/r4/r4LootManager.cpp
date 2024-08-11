/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "r4LootManager.h"
#include "../../common/core/gameSave.h"

RED_DEFINE_STATIC_NAME( loot_manager );
RED_DEFINE_STATIC_NAME( name_name );
RED_DEFINE_STATIC_NAME( maxCount );

IMPLEMENT_ENGINE_CLASS( CR4LootManager );

CR4LootManager::CR4LootManager()
	: m_currentArea( m_areaItemsMaxCount.End() )
{
}

void CR4LootManager::Initialize()
{
	SGameSessionManager::GetInstance().RegisterGameSaveSection( this );
}

void CR4LootManager::Shutdown()
{
	SGameSessionManager::GetInstance().UnregisterGameSaveSection( this );
}

void CR4LootManager::OnGameStart( const CGameInfo& gameInfo )
{
	if ( gameInfo.m_gameLoadStream != nullptr )
	{
		LoadGame( gameInfo.m_gameLoadStream );
	}
	else
	{
		LoadDefinitions();
	}
}

void CR4LootManager::OnGameEnd( const CGameInfo& gameInfo )
{

}

Bool CR4LootManager::OnSaveGame( IGameSaver* saver )
{
	SaveGame( saver );
	return true;
}

Int32 CR4LootManager::GetItemMaxCount( const CName& itemName )
{
	if ( m_currentArea == m_areaItemsMaxCount.End() )
	{
		return -1;
	}
	TItemsMaxCount::iterator it = m_currentArea->m_second.Find( itemName );
	if ( it == m_currentArea->m_second.End() )
	{
		return -1;
	}
	return it->m_second;
}

Bool CR4LootManager::UpdateItemMaxCount( const CName& itemName, Uint32 generatedQuantity )
{
	if ( m_currentArea == m_areaItemsMaxCount.End() )
	{
		return false;
	}
	TItemsMaxCount::iterator it = m_currentArea->m_second.Find( itemName );
	if ( it == m_currentArea->m_second.End() )
	{
		return false;
	}
	if ( it->m_second > generatedQuantity )
	{
		it->m_second = it->m_second - generatedQuantity;
	}
	else
	{
		it->m_second = 0;
	}
	return true;
}

Bool CR4LootManager::LoadDefinitions()
{
	m_areaItemsMaxCount.Clear();

	CDefinitionsManager* dm = GCommonGame->GetDefinitionsManager();
	if ( dm == nullptr )
	{
		RED_ASSERT( false );
		return false;
	}

	const SCustomNode* defNode = dm->GetCustomNode( CNAME( loot_manager ) );
	if ( defNode == nullptr )
	{
		return false;
	}

	TDynArray< SCustomNode >::const_iterator areaIt = defNode->m_subNodes.Begin();
	TDynArray< SCustomNode >::const_iterator areaItEnd = defNode->m_subNodes.End();
	for ( ; areaIt != areaItEnd; ++areaIt )
	{
		String areaName = String::EMPTY;
		for ( TDynArray< SCustomNodeAttribute >::const_iterator attIt = areaIt->m_attributes.Begin();
																attIt != areaIt->m_attributes.End();
																++attIt )
		{
			if ( attIt->GetAttributeName() == CNAME( name_name ) )
			{
				areaName = attIt->GetValueAsCName().AsString();
				break;
			}
		}
		if ( areaName == String::EMPTY )
		{
			continue;
		}
		m_areaItemsMaxCount.Insert( areaName, TItemsMaxCount() );
		TCurrentArea currentArea = m_areaItemsMaxCount.Find( areaName );
		TDynArray< SCustomNode >::const_iterator itemIt = areaIt->m_subNodes.Begin();
		TDynArray< SCustomNode >::const_iterator itemItEnd = areaIt->m_subNodes.End();
		for ( ; itemIt != itemItEnd; ++itemIt )
		{
			CName itemName = CName::NONE;
			Int32 maxCount = 0;
			for ( TDynArray< SCustomNodeAttribute >::const_iterator attIt = itemIt->m_attributes.Begin();
																	attIt != itemIt->m_attributes.End();
																	++attIt )
			{
				CName attributeName = attIt->GetAttributeName();
				if ( attributeName == CNAME( name_name ) )
				{
					itemName = attIt->GetValueAsCName();
				}
				else if ( attributeName == CNAME( maxCount ) )
				{
					attIt->GetValueAsInt( maxCount );
				}
			}
			if ( itemName != CName::NONE )
			{
				currentArea->m_second.Insert( itemName, static_cast< Uint32 >( maxCount ) );
			}
		}
	}

	m_currentArea = m_areaItemsMaxCount.End();
	return true;
}

void CR4LootManager::LoadGame( IGameLoader* loader )
{
	m_areaItemsMaxCount.Clear();

	CGameSaverBlock block( loader, CNAME( loot_manager ) );
	Uint32 areasCount = 0;
	String areaName = String::EMPTY;
	loader->ReadValue< Uint32 >( CNAME( 1 ), areasCount );
	loader->ReadValue< String >( CNAME( 2 ), areaName );
	
	for ( Uint32 i = 0; i < areasCount; i++ )
	{
		CGameSaverBlock block( loader, CNAME( a ) );
		String areaName = String::EMPTY;
		Uint32 itemsCount = 0;
		loader->ReadValue< String >( CNAME( 1 ), areaName );
		loader->ReadValue< Uint32 >( CNAME( 2 ), itemsCount );
		if ( itemsCount == 0 )
		{
			continue;
		}

		m_areaItemsMaxCount.Insert( areaName, TItemsMaxCount() );
		TCurrentArea area = m_areaItemsMaxCount.Find( areaName );
		
		for ( Uint32 j = 0; j < itemsCount; j++ )
		{
			CGameSaverBlock block( loader, CNAME( i ) );
			String itemName = String::EMPTY;
			Uint32 quantity = 0;
			loader->ReadValue< String >( CNAME( 1 ), itemName );
			loader->ReadValue< Uint32 >( CNAME( 2 ), quantity );
			area->m_second.Insert( CName( itemName ), quantity );
		}
	}

	m_currentArea = ( areaName != String::EMPTY ) ? m_areaItemsMaxCount.Find( areaName ) : m_areaItemsMaxCount.End();
}

void CR4LootManager::SaveGame( IGameSaver* saver )
{
	TIMER_BLOCK( time )

	CGameSaverBlock block( saver, CNAME( loot_manager ) );
	saver->WriteValue< Uint32 >( CNAME( 1 ), m_areaItemsMaxCount.Size() );
	saver->WriteValue< String >( CNAME( 2 ), m_currentArea != m_areaItemsMaxCount.End() ? m_currentArea->m_first : String::EMPTY );

	TAreaItemsMaxCount::iterator it = m_areaItemsMaxCount.Begin();
	TAreaItemsMaxCount::iterator itEnd = m_areaItemsMaxCount.End();		
	for ( ; it != itEnd; ++it )
	{
		CGameSaverBlock block( saver, CNAME( a ) );
		saver->WriteValue< String >( CNAME( 1 ), it->m_first );
		saver->WriteValue< Uint32 >( CNAME( 2 ), it->m_second.Size() );
		
		TItemsMaxCount::iterator jt = it->m_second.Begin();
		TItemsMaxCount::iterator jtEnd = it->m_second.End();
		for ( ; jt != jtEnd; ++jt )
		{
			CGameSaverBlock block( saver, CNAME( i ) );
			saver->WriteValue< String >( CNAME( 1 ), jt->m_first.AsString() );
			saver->WriteValue< Uint32 >( CNAME( 2 ), jt->m_second );

		}
	}

	END_TIMER_BLOCK( time )
}

void CR4LootManager::funcSetCurrentArea( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, areaName, String::EMPTY );
	FINISH_PARAMETERS;

	m_currentArea = m_areaItemsMaxCount.Find( areaName );

	RETURN_VOID();
}

void CR4LootManager::funcGetCurrentArea( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	String areaName = String::EMPTY;
	if ( m_currentArea != m_areaItemsMaxCount.End() )
	{
		areaName = m_currentArea->m_first;
	}

	RETURN_STRING( areaName );
}