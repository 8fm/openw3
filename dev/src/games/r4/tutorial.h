/*
* Copyright c 2014 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "r4SystemOrder.h"

class CR4TutorialSystem	: public IGameSystem, public IGameSaveSection
{
	DECLARE_ENGINE_CLASS( CR4TutorialSystem, IGameSystem, 0 );

protected:
	Bool m_needsTickEvent;

public:
	CR4TutorialSystem();
	virtual ~CR4TutorialSystem();

	virtual void Initialize() override;
	virtual void Shutdown() override;

	virtual void OnGameStart( const CGameInfo& gameInfo ) override;
	virtual void OnGameEnd( const CGameInfo& gameInfo ) override;

	virtual void OnWorldStart( const CGameInfo& gameInfo ) override;
	virtual void OnWorldEnd( const CGameInfo& gameInfo ) override;

	virtual void Tick( Float timeDelta ) override;

	virtual void OnGenerateDebugFragments( CRenderFrame* frame ) override;

	virtual Bool OnSaveGame( IGameSaver* saver ) override;

	ASSING_R4_GAME_SYSTEM_ID( GSR4_TutorialSystem )
};

BEGIN_CLASS_RTTI( CR4TutorialSystem );
	PARENT_CLASS( IGameSystem );
	PROPERTY_SAVED( m_needsTickEvent );
END_CLASS_RTTI();