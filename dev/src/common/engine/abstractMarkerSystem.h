/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "../core/sortedmap.h"

#ifndef NO_MARKER_SYSTEMS

// Forward declaration
class IMarkerSystemInterface;

enum EMarkerSystemSortOrder
{
	MSSO_Ascending,
	MSSO_Descending,

	MSSO_Count
};

// Types of all marker systems
enum EMarkerSystemType
{
	MST_MarkerManager,

	MST_POI,
	MST_Sticker,
	MST_Review,

	MST_Count
};
typedef TSortedMap< EMarkerSystemType, IMarkerSystemInterface* > MarkerSystemsMap;

// 
enum EMarkerSystemRequestType
{
	MSRT_ReconnectWithDatabase,
	MSRT_LoadData,
	MSRT_ReleaseData,
	MSRT_ReloadData,
	MSRT_SynchronizeData,
	MSRT_UpdateData,
	MSRT_SortData,

	MSRT_Count
};
typedef TQueue< EMarkerSystemRequestType > MarkerRequests;

// Message types which can be send by marker system
enum EMarkerSystemMessage
{
	MSM_SystemRegistered,
	MSM_SystemUnregistered,

	MSM_DatabaseConnectionStart,
	MSM_DatabaseConnected,
	MSM_DatabaseLostConnection,

	MSM_TestTrackLostConnection,

	MSM_SynchronizationStart,
	MSM_SynchronizationEnd,

	MSM_DataAreUpdated,
	MSM_DataAreSorted,

	MSM_Count
};

// Interface for marker system listener
class IMarkerSystemListener
{
public:
	virtual void ProcessMessage( enum EMarkerSystemMessage message, enum EMarkerSystemType systemType, IMarkerSystemInterface* system ) = 0;
};

// Interface for marker system
class IMarkerSystemInterface
{
public:
	IMarkerSystemInterface() { /* intentionally empty */ };
	virtual ~IMarkerSystemInterface() { /* intentionally empty */ };

	virtual void Initialize					() = 0;
	virtual void Tick						( Float timeDelta ) = 0;
	virtual void Shutdown					() = 0;

	virtual Bool CanBeUpdated				() = 0;
	virtual void BackgroundUpdate			() = 0;
	virtual void LockUpdate					() = 0;
	virtual void UnlockUpdate				() = 0;

	virtual void Connect					() = 0;
	virtual void WaitForEntity				() = 0;
	virtual Bool WaitingForEntity			() const = 0;
	virtual void SetNewEntity				( CEntity* newEntity ) = 0;	// it can't be const because I modify entity in system

	virtual void RegisterListener			( enum EMarkerSystemType systemType, IMarkerSystemListener* listener ) = 0;
	virtual void UnregisterListener			( enum EMarkerSystemType systemType, IMarkerSystemListener* listener ) = 0;

	virtual EMarkerSystemType GetSystemType	() const = 0;

	// send request to marker system
	virtual void SendRequest( enum EMarkerSystemRequestType request ) = 0;
};

// Abstract class for marker subsystems
class AbstractMarkerSystem : public IMarkerSystemInterface
{
public:
	AbstractMarkerSystem( enum EMarkerSystemType systemType );
	virtual ~AbstractMarkerSystem();

	virtual void WaitForEntity() override;
	virtual Bool WaitingForEntity() const override;

	virtual Bool CanBeUpdated() override;
	virtual void LockUpdate() override;
	virtual void UnlockUpdate() override;

	virtual EMarkerSystemType GetSystemType() const override;

	virtual void SendRequest( enum EMarkerSystemRequestType request ) override;

	virtual void RegisterListener( enum EMarkerSystemType systemType, IMarkerSystemListener* listener ) override;
	virtual void UnregisterListener	( enum EMarkerSystemType systemType, IMarkerSystemListener* listener ) override;

protected:
	// send information to listeners about change marker system state
	void SendMessage( enum EMarkerSystemMessage message );

protected:
	Bool								m_updateLocked;				//!<
	Bool								m_waitingForEntity;			//!<

	TDynArray< IMarkerSystemListener* >	m_listeners;				//!<
	EMarkerSystemType					m_systemType;				//!<

	MarkerRequests						m_requestsQueue;			//!<
	Red::Threads::CMutex				m_synchronizationMutex;		//!<
	Red::Threads::CMutex				m_updateMutex;				//!<
};

#endif	// NO_MARKER_SYSTEMS
