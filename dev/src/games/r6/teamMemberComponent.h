#pragma once

#include "r6Component.h"

class CTeam;

class CTeamMemberComponent : public CR6Component
{
	DECLARE_ENGINE_CLASS( CTeamMemberComponent, CR6Component, 0 );

private:
	CName	m_teamName;
	CTeam*	m_team;
	Bool	m_movementTickedAcquired;
	Int32	m_subteamFlag;

private:
	void FetchTeamIfNeeded();

protected:
	void OnEventOccure( CName eventName, CObject* param ) override; 

public:

	CTeamMemberComponent() : m_subteamFlag(0) {}

	RED_INLINE void SetMovementTickedAcquired( Bool val ){ m_movementTickedAcquired = val; }
	RED_INLINE Bool GetMovementTickedAcquired(){ return m_movementTickedAcquired; }
	RED_INLINE Int32 GetSubteamFlag(){ return m_subteamFlag; }
	RED_INLINE void SetSubteamFlag( Int32 flag ){ m_subteamFlag = flag; }

	void OnAttached( CWorld* world ) override;
	void OnDetached( CWorld* world ) override;

	Bool IfAnyAllyInShootingCorridor( const Vector& shootingDirection, const Vector& enemyPosition, Float corridorWidth );

private:
	void funcGetTeam( CScriptStackFrame& stack, void* result );
	void funcIfAnyAllyInShootingCorridor( CScriptStackFrame& stack, void* result );
	void funcAcquireMovementTicket( CScriptStackFrame& stack, void* result );
	void funcReturnMovementTicket( CScriptStackFrame& stack, void* result );
	void funcSetSubteamFlag( CScriptStackFrame& stack, void* result );
	void funcGetSubteamFlag( CScriptStackFrame& stack, void* result );
	void funcResetSubteamFlag( CScriptStackFrame& stack, void* result );
};

BEGIN_CLASS_RTTI( CTeamMemberComponent );
	PARENT_CLASS( CR6Component );	
	PROPERTY_EDIT_NAME( m_teamName, TXT("i_teamName"), TXT("Name of team") );
	PROPERTY( m_team );
	PROPERTY_NAME( m_subteamFlag, TXT("i_subteamFlag") );
	NATIVE_FUNCTION( "I_GetTeam"						, funcGetTeam						);
	NATIVE_FUNCTION( "I_IfAnyAllyInShootingCorridor"	, funcIfAnyAllyInShootingCorridor	);
	NATIVE_FUNCTION( "I_AcquireMovementTicket"			, funcAcquireMovementTicket			);	
	NATIVE_FUNCTION( "I_ReturnMovementTicket"			, funcReturnMovementTicket			);	
	NATIVE_FUNCTION( "I_SetSubteamFlag"					, funcSetSubteamFlag				);	
	NATIVE_FUNCTION( "I_GetSubteamFlag"					, funcGetSubteamFlag				);	
	NATIVE_FUNCTION( "I_ResetSubteamFlag"				, funcResetSubteamFlag				);	
END_CLASS_RTTI();