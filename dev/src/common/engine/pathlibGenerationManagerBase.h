/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "../core/engineTime.h"

namespace PathLib
{

class CAreaDescription;

class IGenerationManagerBase : public Red::System::NonCopyable
{
protected:
	static const Uint32 MAX_SUPPORTED_THREAS_COUNT = 8;							// TODO: make it dependent on compilation settings

	volatile Bool							m_terminationRequest;

public:
	struct CAsyncTask : public Red::Threads::CThread
	{
	private:
		// only limited access - so we make it private and implement all interface from cjob
		IGenerationManagerBase*			m_taskManager;
		Red::Threads::CSemaphore		m_startWork;
		Int32							m_refCount;
	protected:
		EngineTime						m_startTime;
		volatile Float					m_taskProgress;

		struct CScopeSynchronousSection
		{
			CScopeSynchronousSection( CAsyncTask* me )
				: m_me( me )													{ m_me->Begin_SynchronousSection(); }
			~CScopeSynchronousSection()											{ m_me->End_SynchronousSection(); }
			CAsyncTask*							m_me;
		};

		virtual Bool ShouldTerminate();

		Bool Begin_SynchronousSection();
		void End_SynchronousSection();

	public:
		struct CSynchronousSection
		{
			CSynchronousSection( CAsyncTask* task, Bool on = true )
				: m_task( task )
				, m_isOn( on )													{ if ( m_isOn ) { m_canProcess = m_task->Begin_SynchronousSection(); } }
			~CSynchronousSection()												{ if ( m_isOn ) { m_task->End_SynchronousSection(); } }

			Bool Begin()														{ if ( !m_isOn ) { m_isOn = true; m_canProcess = m_task->Begin_SynchronousSection(); } return m_canProcess; }
			void End()															{ if ( m_isOn ) { m_isOn = false; m_task->End_SynchronousSection(); } }

			Bool IsOn() const													{ return m_isOn; }
			Bool FailedToStartSection() const									{ return !m_canProcess; }
			CAsyncTask* GetTask() const											{ return m_task; }

			CAsyncTask*							m_task;
			Bool								m_isOn;
			Bool								m_canProcess;
		};

		CAsyncTask( const AnsiChar* threadName = "PathLib-Undefined" );

		void AddRef();
		void Release();

		Float GetTaskProgrees() const volatile								{ return m_taskProgress; }
		void SetTaskProgress( Float f )	volatile							{ m_taskProgress = f; }

		virtual Bool PreProcessingSync();
		virtual Bool ProcessPathLibTask() = 0;
		virtual CAsyncTask* PostProcessingSync();
		virtual void DescribeTask( String& task ) = 0;
		virtual CAreaDescription* GetTargetArea() const;

		void Run( IGenerationManagerBase* taskManager );
		//! Process the job
		void ThreadFunc() override;

	};

	IGenerationManagerBase();
	virtual ~IGenerationManagerBase();

	Bool					TerminationRequest() const volatile				{ return m_terminationRequest; }

	virtual void			OnTaskFinished( CAsyncTask* task );

	virtual void			RequestSynchronousProcessing() = 0;
	virtual void			FinishSynchronousProcessing() = 0;

};



};				// namespace PathLib