/**
* Copyright c 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../../common/game/commonGame.h"

class CSwarmSpecies;

enum ERedGame
{
	RG_Unknown,
	RG_R4,
	RG_R6
};

class CWccDummyGame : public CCommonGame
{
	DECLARE_ENGINE_CLASS( CWccDummyGame, CCommonGame, 0 );

public:

	CWccDummyGame( ERedGame game = RG_Unknown ) { m_game = game; }
	virtual ~CWccDummyGame() {}

	// Initialize game
	virtual void Init();

	// Shut down game, unload world
	virtual void ShutDown();

private:

	ERedGame m_game;
};

BEGIN_CLASS_RTTI( CWccDummyGame );
	PARENT_CLASS( CCommonGame );
END_CLASS_RTTI();
