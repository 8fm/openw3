/**
* Copyright c 2012 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#include "../../common/game/gameSystem.h"

class CGameplayFXSurfacePost : public IGameSystem
{
	DECLARE_ENGINE_CLASS( CGameplayFXSurfacePost, IGameSystem, 0 );
	
protected:
		
	Bool								m_isActive;	
	//CClass*							m_focusEntityClass;

public:
	CGameplayFXSurfacePost();
	~CGameplayFXSurfacePost();

	virtual void OnGameStart( const CGameInfo& gameInfo ) override;
	virtual void OnGameEnd( const CGameInfo& gameInfo ) override;
	virtual void Tick( Float timeDelta ) override;

	virtual void OnGenerateDebugFragments( CRenderFrame* frame ) override;
		
	void Init( const Vector& fillColor );
	void AddSurfacePostGroup( const Vector& position, Float fadeInTime, Float fadeOutTime, Float activeTime, Float range, Uint32 type );
	void Disable();

public:
	ASSING_R4_GAME_SYSTEM_ID( GSR4_SurfacePost );
		
	void funcIsActive( CScriptStackFrame& stack, void* result );	
	void funcInit(  CScriptStackFrame& stack, void* result );	
	void funcAddSurfacePostFXGroup(  CScriptStackFrame& stack, void* result );	
};

BEGIN_CLASS_RTTI( CGameplayFXSurfacePost )
	PARENT_CLASS( IGameSystem )
	NATIVE_FUNCTION( "Init", funcInit );
	NATIVE_FUNCTION( "IsActive", funcIsActive );
	NATIVE_FUNCTION( "AddSurfacePostFXGroup", funcAddSurfacePostFXGroup );
END_CLASS_RTTI();