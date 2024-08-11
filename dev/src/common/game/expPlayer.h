
#pragma once

#include "expIntarface.h"
#include "expManager.h"

class ExpOracle;

class ExpPlayer
{
	ExpNNQueryData		m_pendingQuery;

	const CEntity*		m_owner;
	const ExpManager*	m_director;

	THandle< IScriptable >	m_listener;

	SExplorationQueryToken m_token;
	IExpExecutor*		m_executor;

	Bool				m_isFinished;

public:
	ExpPlayer( const SExplorationQueryToken& token, const ExpManager* dir, CEntity* ent, const THandle< IScriptable >& listener = THandle< IScriptable >::Null() );
	~ExpPlayer();

	Bool Init();

	void Update( Float dt );

	Bool IsRunning() const;

	void OnActionStoped();

	EExplorationType GetExplorationType() { return m_token.m_type; }

public:
	void ConnectListener( const THandle< IScriptable >& object );
	void DisconnectListener();
	Bool HasListener() const;

public:
	void GenerateDebugFragments( CRenderFrame* frame );

private:
	void RequestNNQuery( const ExpNNQueryData& query );
	void ProcessNNQuery();
	ExpNNQueryData GetNNQueryLastResult() const;

	void SendStartNotification();
	void SendEndNotification();
	void SendNotifications( const ExpExecutorUpdateResult& result );

	void SendStartNotificationToObject( const THandle< IScriptable >& object );
	void SendEndNotificationToObject( const THandle< IScriptable >& object );
	void SendNotificationsToObject( const ExpExecutorUpdateResult& result, const THandle< IScriptable >& object );
};
