/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "../../common/game/gameSystem.h"
#include "../../common/game/lootDefinitions.h"

class CR4LootManager : public IGameSystem, public IGameSaveSection, public ILootManager
{
	DECLARE_ENGINE_CLASS( CR4LootManager, IGameSystem, 0 );

public:
	CR4LootManager();

	virtual void Initialize() override;
	virtual void Shutdown() override;
	virtual void OnGameStart( const CGameInfo& gameInfo ) override;
	virtual void OnGameEnd( const CGameInfo& gameInfo ) override;
	virtual bool OnSaveGame( IGameSaver* saver ) override;

	Int32 GetItemMaxCount( const CName& itemName ) override;
	Bool UpdateItemMaxCount( const CName& itemName, Uint32 generatedQuantity ) override;

private:

	typedef THashMap< CName, Uint32 >			TItemsMaxCount;
	typedef THashMap< String, TItemsMaxCount >	TAreaItemsMaxCount;
	typedef TAreaItemsMaxCount::iterator	TCurrentArea;

	TAreaItemsMaxCount	m_areaItemsMaxCount;
	TCurrentArea		m_currentArea;

	Bool LoadDefinitions();
	void LoadGame( IGameLoader* loader );
	void SaveGame( IGameSaver* saver );

public:
	ASSING_R4_GAME_SYSTEM_ID( GSR4_LootManager );

	void funcSetCurrentArea( CScriptStackFrame& stack, void* result );
	void funcGetCurrentArea( CScriptStackFrame& stack, void* result );
};

BEGIN_CLASS_RTTI( CR4LootManager )
	PARENT_CLASS( IGameSystem )
	NATIVE_FUNCTION( "SetCurrentArea", funcSetCurrentArea );
	NATIVE_FUNCTION( "GetCurrentArea", funcGetCurrentArea );
END_CLASS_RTTI();
