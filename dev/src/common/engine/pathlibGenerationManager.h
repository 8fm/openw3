#pragma once

#ifndef NO_EDITOR_PATHLIB_SUPPORT

#include "../core/longBitField.h"

#include "pathlibGenerationManagerBase.h"
#include "pathlib.h"

class CPathLibWorld;

namespace PathLib
{
	class CAreaDescription;
	class CNavmeshAreaDescription;
	class CTerrainAreaDescription;

////////////////////////////////////////////////////////////////////////////
// Integral part of CPathLibWorld, separated to easily turn off all
// functionality on release builds
class CGenerationManager : public IGenerationManagerBase
{
public:
	class CStatusListener
	{
	protected:
		typedef Red::Threads::CScopedLock< Red::Threads::CMutex > Lock;
		Red::Threads::CMutex	m_mutex;
		THandle< CWorld >		m_world;
		void UnregisterLocked();
		void ProcessAllTasksLocked();
	public:

		enum ETaskStatus
		{
			Status_Pending = 0,
			Status_Active,
			Status_Complete,
			Status_Failed,

			Status_Max
		};

		CStatusListener( CWorld* world );
		virtual ~CStatusListener()											{}

		virtual void WorldDestroyedLocked() = 0;
		virtual void TaskStatusChangedLocked( AreaId taskId, ETaskStatus status, const String& taskDescription ) = 0;
		void Unregister()													{ Lock lock( m_mutex ); UnregisterLocked(); }
		void WorldDestroyed()												{ Lock lock( m_mutex ); WorldDestroyedLocked(); }
		void TaskStatusChanged( AreaId taskId, ETaskStatus status, const String& taskDescription )	{ }
		void TaskProcessed( CAreaDescription* areaDescription );
		void TaskIsActive( CAreaDescription* areaDescription );
		void ProcessAllTasks()												{ Lock lock( m_mutex ); ProcessAllTasksLocked(); }
	};

protected:
	CPathLibWorld&					m_owner;							// friendly world
	Uint8							m_isDisabled;
	Bool							m_isDirty;							// dirty flag
	Bool							m_recalculateWaypoints;				// waypoints are potentially dirty
	Bool							m_updateObstacles;
	Bool							m_checkObsolateObstacles;
	volatile Bool					m_isProcessing;						// is job currently running
	
	EngineTime						m_modyficationDelay;				// delay until next automated processing job
	EngineTime						m_recalculateWaypointsDelay;
	Box								m_recalculateWaypointsBox;

	EngineTime						m_updateObstaclesDelay;
	TDynArray< Box >				m_updateObstaclesAreas;

	Red::Threads::CAtomic< Int32 >	m_syncProcessingRequestsCount;
	Red::Threads::CSemaphore		m_syncProcessingRequests;
	Red::Threads::CSemaphore		m_syncProcessingRequestsDone;

	EngineTime						m_checkObsolateObstaclesDelay;

	CAsyncTask*						m_currentAsyncTask;
	mutable Red::Threads::CMutex	m_mutex;
	CStatusListener*				m_statusListener;

	static const EngineTime			s_updateDelay;						// time delay for automated processing

	void							RunTask( CAsyncTask* job );

	template < class Functor >
	void							IterateActiveTasks( Functor& functor );

	void							ProcessThreadSyncSection();
public:
	enum EDisableReason
	{
		DISABLE_GAME				= FLAG(1),
		DISABLE_TOOL				= FLAG(2)
	};
	CGenerationManager( CPathLibWorld& owner );
	~CGenerationManager();

	void Initialize();
	void BreakProcessing();
	void Shutdown();

	void HandleFinishedTask();
	void Tick();
	Bool GameTick();
	void MarkDirty( AreaId dirtyArea = INVALID_AREA_ID );

	void RecalculateWaypoints( const Box& box = Box( Vector( -FLT_MAX, -FLT_MAX, -FLT_MAX ), Vector( FLT_MAX, FLT_MAX, FLT_MAX ) ) );
	void UpdateObstacles( const Box& bbox, Float updateDelay = 3.f );
	void CheckObsolateObstacles();

	Bool IsOn() const														{ return m_isDisabled == 0; }
	Bool IsDisabledByTools() const											{ return (m_isDisabled & DISABLE_TOOL) != 0; }
	void TurnOn( Bool b, EDisableReason r )									{ m_isDisabled = b ? Uint8(m_isDisabled & (~r)) : Uint8(m_isDisabled | r); }
	Bool IsProcessing() const												{ return m_isProcessing; }
	Bool TerminationRequest() const volatile								{ return m_terminationRequest; }
	CAsyncTask* GetCurrentTask() const										{ return m_currentAsyncTask; }
	Bool IsDirty() const													{ return m_isDirty; }

	void GetPendingTasks( CStatusListener* lisener );

	void OnTaskFinished( CAsyncTask* task ) override;
	CPathLibWorld& GetOwner()												{ return m_owner; }

	void RegisterStatusListener( CStatusListener* lisener );
	void UnregisterStatusListener()											{ m_statusListener = NULL; }

	void RequestSynchronousProcessing() override;
	void FinishSynchronousProcessing() override;
};


};		// namespace PathLib


#endif // NO_EDITOR_PATHLIB_SUPPORT