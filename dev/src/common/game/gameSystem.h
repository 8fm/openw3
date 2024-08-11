/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

class CGameInfo;

/// Base class for all gameplay related systems
class IGameSystem : public CObject
{
	DECLARE_ENGINE_ABSTRACT_CLASS( IGameSystem, CObject );

public:
	IGameSystem();
	virtual ~IGameSystem();

	virtual void Initialize(){}
	virtual void Shutdown(){}

	virtual void OnGameStart( const CGameInfo& gameInfo ) {}
	virtual void OnGameEnd( const CGameInfo& gameInfo ) {}

	virtual void OnWorldStart( const CGameInfo& gameInfo ) {}
	virtual void OnWorldEnd( const CGameInfo& gameInfo ) {}

	virtual void Tick( Float timeDelta ) {}
	virtual void PausedTick() {}

	virtual void OnGenerateDebugFragments( CRenderFrame* frame ) {}
};

BEGIN_ABSTRACT_CLASS_RTTI( IGameSystem );
	PARENT_CLASS( CObject );
END_CLASS_RTTI();