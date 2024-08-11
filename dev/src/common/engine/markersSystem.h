/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#ifndef NO_MARKER_SYSTEMS

#include "abstractMarkerSystem.h"

class CMarkersSystemThread : public Red::Threads::CThread
{
public:
	CMarkersSystemThread( IMarkerSystemInterface* systemToUpdate );
	~CMarkersSystemThread();

	// background thread for updating systems
	virtual void ThreadFunc() override;

private:
	Double					m_lastUpdate;
	IMarkerSystemInterface* m_systemToUpdate;
};

// Main review system class
class CMarkersSystem : public IMarkerSystemInterface
{
public:
	CMarkersSystem	();
	~CMarkersSystem	();

	virtual void Initialize() override;
	virtual void Tick( Float timeDelta ) override;
	virtual void Shutdown() override;

	// register marker system in manager
	void RegisterMarkerSystem( enum EMarkerSystemType systemType, IMarkerSystemInterface* newSystem );

	// implement IMarkerSystemInterface
	virtual void BackgroundUpdate() override;

	virtual void Connect() override;
	virtual bool WaitingForEntity() const override;
	virtual void SetNewEntity(CEntity* newEntity) override;
	virtual void WaitForEntity() override;

	virtual void RegisterListener( enum EMarkerSystemType systemType, IMarkerSystemListener* listener ) override;
	virtual void UnregisterListener( enum EMarkerSystemType systemType, IMarkerSystemListener* listener ) override;

	virtual void SendRequest( enum EMarkerSystemRequestType request ) override;

	virtual EMarkerSystemType GetSystemType() const override;
	IMarkerSystemInterface* GetSystem( enum EMarkerSystemType systemType );

	// 
	Bool IsActive			() const;
	void TurnOnSystems		();
	void TurnOffSystems		();

private:
	virtual Bool CanBeUpdated()  override { return false; }				// is unused
	virtual void LockUpdate() override { /* intentionally empty */ }	// is unused
	virtual void UnlockUpdate() override { /* intentionally empty */ }	// is unused

	void CreateSubsystems();

private:
	Bool								m_systemsAreOn;	//!<
	CMarkersSystemThread*				m_thread;		//!<

	MarkerSystemsMap					m_systems;		//!<
	TDynArray< IMarkerSystemListener* >	m_listeners;	//!<
	EMarkerSystemType					m_systemType;	//!<
	Red::Threads::CMutex				m_updateGuard;	//!<
};

#endif	// NO_MARKER_SYSTEMS
