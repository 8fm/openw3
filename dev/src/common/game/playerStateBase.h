/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#pragma once

/// Base player state
class CPlayerStateBase : public CScriptableState
{	
	DECLARE_RTTI_SIMPLE_CLASS( CPlayerStateBase );

private:
	Int32	m_noSaveLock;

	static const Int32 NO_SAVE_LOCK_INVALID = -1;

public:
	//! Activate state
	virtual void OnEnterState( const CName& previousState );

	//! Leave state
	virtual void OnLeaveState( const CName& newState );

	virtual Bool SetPlayerMovable( Bool movable, Bool enableSteerAgent ) { return false; }
	virtual Bool GetPlayerMovable() const { return false; }
	virtual Bool IsPlayerMovingEnabled() const { return false; }

	virtual void GenerateDebugFragments( CRenderFrame* frame ) {}

public:
	//! Get player
	CPlayer* GetPlayer() const;	

private:
	void funcCreateNoSaveLock( CScriptStackFrame& stack, void* result );
};

BEGIN_CLASS_RTTI( CPlayerStateBase );
	PARENT_CLASS( CScriptableState );
	NATIVE_FUNCTION( "CreateNoSaveLock", funcCreateNoSaveLock );
END_CLASS_RTTI();
