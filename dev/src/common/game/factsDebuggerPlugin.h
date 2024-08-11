/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "factsDB.h"
#include "../engine/debugServerPlugin.h"

class CFactsDebuggerPlugin : public CDebugServerPlugin, public IGlobalEventsListener
{
public:
	// common
	virtual Bool Init() final;
	virtual Bool ShutDown() final;

	// life-time
	virtual void GameStarted() final;
	virtual void GameStopped() final;
	virtual void AttachToWorld() final;
	virtual void DetachFromWorld() final;	
	virtual void Tick() final;

	Uint32			SendFactIDs() const;
	Uint32			SendFactData( const String& factId ) const;
	void			OnFactsDBChanged( const String& id );
	virtual void	OnGlobalEvent( EGlobalEventCategory eventCategory, EGlobalEventType eventType, const SGlobalEventParam& param ) final;

private:
	struct SFactInfo
	{
		EGlobalEventType	eventType;
		String				factId;
	};

	CFactsDB*			m_factsDB;
	Red::System::Timer	m_factsSendingTimer;
	Double				m_lastSendTime;
	TQueue< SFactInfo >	m_pendingFacts;

	String	FactAsString( const CFactsDB::Fact* fact ) const;
	void	SendFactAdded( const String& factId ) const;
	void	SendFactRemoved( const String& factId ) const;
	Bool	ShouldSendFacts( const Double currentTime ) const;
	void	SendPendingFacts();
};
