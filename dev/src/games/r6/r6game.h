/**
* Copyright c 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../../common/game/commonGame.h"

#include "coversManager.h"
#include "r6Player.h"

class CR6DialogInterlocutor;
class CCombatUtils;
class CFlashMovie;
class CCrowdManager;

void InitializeR6GameSystems( CCommonGame *game );
void ShutdownR6GameSystems( CCommonGame *game );

class CR6Game : public CCommonGame 
{
	DECLARE_ENGINE_CLASS( CR6Game, CCommonGame, 0 );

private:
	CCoversManager*							m_coversManager;	// TODO: implement as IGameSystem, remove member variable from here
	CCombatUtils*							m_combatUtils;		// TODO: implement as IGameSystem, remove member variable from here
	Float									m_gameplayTime;		// TODO: This can not be Float

#ifndef NO_DEBUG_PAGES
	IDebugPage*								m_idialogDebugPage;
#endif

	CFlashMovie*							m_tempLoadingScreen;

public:
	CR6Game();

	// Initialize game
	virtual void Init();

	// Shut down game, unload world
	virtual void ShutDown();

	virtual void Tick( Float timeDelta );

	RED_INLINE CCoversManager*		GetCoversManager()	const { return m_coversManager; }
	RED_INLINE CCombatUtils*			GetCombatUtils()	const { return m_combatUtils;	}	

	RED_INLINE CIDInterlocutorComponent*				GetPlayerInterlocutorComponent()	const			{ return ( static_cast< CR6Player* > ( GetPlayerEntity() ) )->GetInterlocutorComponent(); };

public:
	virtual Bool SetupGameResourceFromFile( const String& filePath );

	void SetupGameResource();

	virtual const Char* GetGamePrefix() const { return TXT("R6"); }

	// Handle engine input
	virtual Bool OnViewportInput( IViewport* view, enum EInputKey key, enum EInputAction action, Float data );

	void OnGameplaySystemsGameStart( const CGameInfo& info ) override;

protected:
	virtual void OnGameplaySystemsWorldStart( const CGameInfo& info );
	virtual void OnGameplaySystemsWorldEnd( const CGameInfo& info );	

	// Script functions
	//void funcGetActionCamera( CScriptStackFrame& stack, void* result );
	void funcGetViewPortWidthAndHeight( CScriptStackFrame& stack, void* result );
	void funcEndCurrentDialog( CScriptStackFrame& stack, void* result );
	void funcGetCoverManager( CScriptStackFrame& stack, void* result );
	void funcGetCombatUtils( CScriptStackFrame& stack, void* result );
	void funcChangeWorld( CScriptStackFrame& stack, void* result );
	void funcGetGameplayTime( CScriptStackFrame& stack, void* result );
};

BEGIN_CLASS_RTTI( CR6Game ); 
	PARENT_CLASS( CCommonGame );	
	PROPERTY( m_coversManager );
	PROPERTY( m_combatUtils );	

	NATIVE_FUNCTION( "I_GetViewPortWidthAndHeight"	, funcGetViewPortWidthAndHeight );
	NATIVE_FUNCTION( "I_EndCurrentDialog"			, funcEndCurrentDialog			);
	NATIVE_FUNCTION( "I_GetCoverManager"			, funcGetCoverManager			);
	NATIVE_FUNCTION( "I_GetCombatUtils"				, funcGetCombatUtils			);	
	NATIVE_FUNCTION( "I_GetGameplayTime"			, funcGetGameplayTime			);
	NATIVE_FUNCTION( "ChangeWorld"					, funcChangeWorld				);	
END_CLASS_RTTI();
