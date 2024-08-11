/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "distanceChecker.h"

class IEntitiesDetectorListener
{
public:

	virtual void BeginEntitiesDetection() { }
	virtual void EndEntitiesDetection() { }
	virtual void ProcessDetectedEntity( CGameplayEntity* ) { }
};

class CEntitiesDetector : public IGameSystem
{
	DECLARE_ENGINE_CLASS( CEntitiesDetector, IGameSystem, 0 );

public:

	ASSIGN_GAME_SYSTEM_ID( GS_EntitiesDetector );

	CEntitiesDetector();
	~CEntitiesDetector();

	// IGameSysytem
	virtual void OnGameStart( const CGameInfo& gameInfo ) override;
	virtual void OnGameEnd( const CGameInfo& gameInfo ) override;
	virtual void OnWorldStart( const CGameInfo& gameInfo ) override;
	virtual void OnWorldEnd( const CGameInfo& gameInfo ) override;
	virtual void Tick( Float timeDelta ) override;
	virtual void OnGenerateDebugFragments( CRenderFrame* frame ) override;

	Bool Register( IEntitiesDetectorListener* listener, Float frequency, Float range, Bool explicitRange = false );
	Bool Unregister( IEntitiesDetectorListener* listener );
	void Detect( IEntitiesDetectorListener* listener, Float range );

	struct SListenerEntry
	{
		IEntitiesDetectorListener*	m_listener;
		Float						m_frequency;
		EngineTime					m_nextUpdate;
		Float						m_range;
		Bool						m_explicitRange;

		SListenerEntry();
		SListenerEntry( IEntitiesDetectorListener* listener, Float frequency, EngineTime nextUpdate, Float range, Bool explicitRange );
	};

	typedef TDynArray< SListenerEntry* > TListeners;

private:

	static const Float UPDATE_RANGE;

	SListenerEntry* FindListenerEntry( IEntitiesDetectorListener* listener );
	
	void UpdateMinFrequency();
	Bool NeedUpdate();
	void Update();
	void Update( TListeners& listeners, Bool forceUpdate, Bool setupNextUpdate );

	TListeners				m_listeners;	
	TListeners				m_newListeners;
	Float					m_minFrequency;
	EngineTime				m_nextUpdate;
	CPlayerDistanceChecker	m_distanceChecker;
};

BEGIN_CLASS_RTTI( CEntitiesDetector )
	PARENT_CLASS( IGameSystem )
END_CLASS_RTTI();
