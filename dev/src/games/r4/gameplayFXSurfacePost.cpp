/**
* Copyright c 2012 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "gameplayFXSurfacePost.h"
#include "customCamera.h"
#include "../../common/engine/renderCommands.h"

IMPLEMENT_ENGINE_CLASS( CGameplayFXSurfacePost );

CGameplayFXSurfacePost::CGameplayFXSurfacePost()
	: m_isActive( false )	
{
}

CGameplayFXSurfacePost::~CGameplayFXSurfacePost()
{
}

void CGameplayFXSurfacePost::OnGameStart( const CGameInfo& gameInfo )
{	
	Init( Vector3(0.5f,0.5f,1.0f) );
}

void CGameplayFXSurfacePost::OnGameEnd( const CGameInfo& gameInfo )
{
	// force disable this effect
	Disable();	
}

void CGameplayFXSurfacePost::Tick( Float timeDelta )
{
	PC_SCOPE_PIX( CGameplayFXSurfacePost );		
	CallEvent( CNAME( OnTick ), timeDelta );	
}

void CGameplayFXSurfacePost::OnGenerateDebugFragments( CRenderFrame * frame )
{
#ifndef RED_FINAL_BUILD
	
	//

#endif
}

void CGameplayFXSurfacePost::AddSurfacePostGroup( const Vector& position, Float fadeInTime, Float fadeOutTime, Float activeTime, Float range, Uint32 type )
{
	if( m_isActive ) ( new CRenderCommand_AddSurfacePostFx( position, fadeInTime, fadeOutTime, activeTime, range, type ) )->Commit();
}

void CGameplayFXSurfacePost::Init( const Vector& fillColor )
{
	if( !m_isActive ) 
	{
		m_isActive = true;
		( new CRenderCommand_InitSurfacePostFx( fillColor ) )->Commit();		
	}	
}

void CGameplayFXSurfacePost::Disable()
{
	m_isActive = false;
}

void CGameplayFXSurfacePost::funcAddSurfacePostFXGroup( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, position, Vector::ZEROS );	 
	GET_PARAMETER( Float, fadeInTime, 0.5f );
	GET_PARAMETER( Float, activeTime, 5.0f );
	GET_PARAMETER( Float, fadeOutTime, 0.5f );
	GET_PARAMETER( Float, range, 8.0f );
	GET_PARAMETER( Int32, type, 0 );

	FINISH_PARAMETERS;
	AddSurfacePostGroup( position, fadeInTime, fadeOutTime, activeTime, range, (Uint32)type );
	RETURN_VOID();
}

void CGameplayFXSurfacePost::funcIsActive( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_BOOL( m_isActive );
}

void CGameplayFXSurfacePost::funcInit( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, fillColor, Vector::ZEROS );	
	FINISH_PARAMETERS;
	Init( fillColor );
	RETURN_VOID();
}