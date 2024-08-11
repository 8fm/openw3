/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "taskManager.h"
#include "task.h"
#include "messagePump.h"

//#define PROFILE_PARALLEL_FOR

class CParallelForTaskBase : public CTask
{
public:
	struct SParams
	{
		DECLARE_STRUCT_MEMORY_POOL( MemoryPool_SmallObjects, MC_Engine );

		friend class CParallelForTaskBase;

		Int32							m_numElements;				// number of array elements
		ETaskSchedulePriority			m_priority;					// task priority
		Uint32							m_numTasksIncludingInline;	// number of tasks, including inline 

	protected:
		Red::Threads::CAtomic< Int32 >	m_internalAtomicCounter;	// currently processed element
		Red::Threads::CAtomic< Int32 >	m_internalRefCount;
		Red::Threads::CAtomic< Int32 >	m_tasksRunning;

		static const Uint32				MAX_NUM_TASKS = 8;

	#ifdef PROFILE_PARALLEL_FOR
		Double							m_issuingTime;
		Double							m_endingTime;
		Double							m_yieldingTime;
		Double							m_timeStarted;		
		Double							m_taskTimers[ MAX_NUM_TASKS ];
		Uint32							m_nextTimer;
		const Char*						m_debugName;             

	public:
		RED_INLINE void SetDebugName( const Char* name ) { m_debugName = name; }
	#else
	public:
		RED_INLINE void SetDebugName( const Char* name ) { RED_UNUSED( name ); }
	#endif // PROFILE_PARALLEL_FOR

	public:
		RED_INLINE void AddRef() { m_internalRefCount.Increment(); }
		RED_INLINE void Release() 
		{ 
			if ( 0 == m_internalRefCount.Decrement() )
			{
				delete this;
			}
		}

		RED_INLINE void OnTaskStarted() { m_tasksRunning.Increment(); }
		RED_INLINE void OnTaskFinished() { m_tasksRunning.Decrement(); }

		RED_INLINE void StartProcessing()
		{
			#ifdef PROFILE_PARALLEL_FOR
				Red::ScopedStopClock clk( m_issuingTime );
			#endif // PROFILE_PARALLEL_FOR

			// Ensure there is minimum 1 task to spawn and we do mot exceed the limit
			ASSERT( m_numTasksIncludingInline > 0 && m_numTasksIncludingInline <= MAX_NUM_TASKS );
			const Uint32 numTasks = ::Clamp< Uint32 > ( m_numTasksIncludingInline - 1, 1, MAX_NUM_TASKS - 1 ); 

			CTask* tasks[ MAX_NUM_TASKS ];
			for ( Uint32 i = 0; i < numTasks; ++i )
			{
				// Create
				tasks[ i ] = CreateTask();
			}

			GTaskManager->Issue( tasks, numTasks, m_priority );

			for ( Uint32 i = 0; i < numTasks; ++i )
			{
				// Make it auto-delete itself after processing
				tasks[ i ]->Release();
			}	
		}

		RED_INLINE void FinalizeProcessing()
		{
			{
				#ifdef PROFILE_PARALLEL_FOR
					Red::ScopedStopClock clk( m_endingTime );
				#endif // PROFILE_PARALLEL_FOR
				
				if ( m_internalAtomicCounter.GetValue() < m_numElements ) // isn't finished yet
				{
					CTask* localTask = CreateTask();
					localTask->Run();
					localTask->Release();
				}
			}

			{
				#ifdef PROFILE_PARALLEL_FOR
					Red::ScopedStopClock clk( m_yieldingTime );
				#endif // PROFILE_PARALLEL_FOR
				
				// ok, all array elements have at least started processing, and the one on current thread is actually finished,
				// but we cannot leave this function until ALL of the STARTED tasks are finished.
				// Otherwise we would end up with user thinking that job is done, while it acually isn't quite finished yet.
				// This should happen very soon, as all of the started workers are processing last of their elements now.

#ifdef RED_PLATFORM_DURANGO
				if ( SIsMainThread() )
				{
					while ( m_tasksRunning.GetValue() > 0 )
					{
						PUMP_MESSAGES_DURANGO_CERTHACK();
						Red::Threads::YieldCurrentThread();
					}
				}
				else
#endif
				while ( m_tasksRunning.GetValue() > 0 )
				{
					Red::Threads::YieldCurrentThread();
				}
			}

			// remember to call Release() after processing
		}

		RED_INLINE void ProcessNow()
		{
			StartProcessing();
			FinalizeProcessing();
		}

	protected:
		SParams() 			
			: m_numElements( 0 )
			, m_priority( TSP_Normal )
			, m_numTasksIncludingInline( GTaskManager->GetNumDedicatedTaskThreads() + 1 ) // TODO: re-profile this
			, m_internalAtomicCounter( -1 )	
			, m_internalRefCount( 1 )
			, m_tasksRunning( 0 )
		{
			#ifdef PROFILE_PARALLEL_FOR
				m_issuingTime = 0.0;
				m_nextTimer = 0;
				m_timeStarted = Red::Clock::GetInstance().GetTimer().GetSeconds();
			#endif // PROFILE_PARALLEL_FOR
		}

		virtual ~SParams()
		{
			ASSERT( m_internalAtomicCounter.GetValue() >= m_numElements, TXT("Deleting SParams before the work is done! DEBUG!") ); 
			#ifndef RED_PLATFORM_ORBIS
			#ifdef PROFILE_PARALLEL_FOR
				Double time = Red::Clock::GetInstance().GetTimer().GetSeconds() - m_timeStarted;
				//if ( time > 0.001 )
				{
					Double longestTimer( 0.0 );
					for ( Uint32 i = 0; i < m_nextTimer; ++i )
					{
						if ( m_taskTimers[ i ] > longestTimer )
						{
							longestTimer = m_taskTimers[ i ];
						}
					}

					//if ( time - longestTimer > 0.001 )
					{
						OutputDebugStringW( String::Printf( TXT("Parallel for %ls took %.2lf msec, while longest thread took %.2lf msec, for %ld elements, issuing time: %.2lf, ending time: %.2lf, yielding time: %.2lf.\n"),
							m_debugName, time * 1000.0, longestTimer * 1000.0, m_numElements, m_issuingTime * 1000.0, m_endingTime * 1000.0, m_yieldingTime * 1000.0 ).AsChar() );
					}
				}
			#endif // PROFILE_PARALLEL_FOR
			#endif // ndef RED_PLATFORM_ORBIS
		}

		virtual CParallelForTaskBase* CreateTask() = 0;

	private:
		SParams( const SParams& ); // DO NOT copy SParams object!
		const SParams& operator=( const SParams& ); // DO NOT copy SParams object!
	};

	protected:
		Int32								m_numElements;
		Red::Threads::CAtomic< Int32 >&		m_atomicCounter;  // IMPORTANT to have a reference here
	#ifdef PROFILE_PARALLEL_FOR
		Double&								m_timer;
	#endif // PROFILE_PARALLEL_FOR
		SParams&							m_params; // only for ref counts

protected:
		CParallelForTaskBase( SParams& params ) 
			: m_numElements( params.m_numElements )
			, m_atomicCounter( params.m_internalAtomicCounter )
		#ifdef PROFILE_PARALLEL_FOR
			, m_timer( params.m_taskTimers[ params.m_nextTimer++ ] )
		#endif // PROFILE_PARALLEL_FOR
			, m_params( params )
		{
			params.AddRef();
		}

		virtual ~CParallelForTaskBase()
		{
			m_params.Release();
		}

	#ifndef NO_DEBUG_PAGES
public:
		virtual const Char* GetDebugName() const { return TXT("CParallelForTask"); }
		virtual Uint32 GetDebugColor() const { return 0xaaaaaaaa; }
	#endif // NO_DEBUG_PAGES
};

template< typename TElement, typename TProcessFunctionOwner >
class CParallelForTaskSingleArray : public CParallelForTaskBase
{
public:
	typedef void ( TProcessFunctionOwner::*TProcessFunc )( TElement& ); 

	struct SParams : public CParallelForTaskBase::SParams
	{
		DECLARE_STRUCT_MEMORY_POOL( MemoryPool_SmallObjects, MC_Engine );

		TElement*						m_array;
		TProcessFunc					m_processFunc;
		TProcessFunctionOwner*			m_processFuncOwner;

		static SParams* Create() { return new SParams(); } // this sets the refcount to 1, remember to call Release() when you're done 
	
	protected:
		virtual CParallelForTaskBase* CreateTask() override { return new ( CTask::Root ) CParallelForTaskSingleArray( *this ); }
	};

protected:
	TElement*							m_array;
	TProcessFunc						m_processFunc;
	TProcessFunctionOwner*				m_processFuncOwner;

	CParallelForTaskSingleArray( SParams& params ) 
		: CParallelForTaskBase( params )
		, m_array( params.m_array )
		, m_processFunc( params.m_processFunc )
		, m_processFuncOwner( params.m_processFuncOwner )
	{
	}

	void Run() override
	{
		PC_SCOPE( CParallelForTaskSingleArray_Run );

		m_params.OnTaskStarted();
		{
			#ifdef PROFILE_PARALLEL_FOR
				Red::ScopedStopClock clk( m_timer );
			#endif // PROFILE_PARALLEL_FOR

			Int32 currentElement = m_atomicCounter.Increment();
			while ( currentElement < m_numElements )
			{
				// Call process func
				( m_processFuncOwner->*m_processFunc )( m_array[ currentElement ] ); 

				// Bump the counter
				currentElement = m_atomicCounter.Increment();
			};
		}
		m_params.OnTaskFinished();
	}
};


template< typename TInputElement, typename TOutputElement, typename TProcessFunctionOwner >
class CParallelForTaskDoubleArray : public CParallelForTaskBase
{
public:
	typedef void ( TProcessFunctionOwner::*TProcessFunc )( const TInputElement&, TOutputElement& ); 

	struct SParams : public CParallelForTaskBase::SParams
	{
		const TInputElement*			m_inputArray;
		TOutputElement*					m_outputArray;
		TProcessFunc					m_processFunc;
		TProcessFunctionOwner*			m_processFuncOwner;

		static SParams* Create() { return new SParams(); } // this sets the refcount to 1, remember to call Release() when you're done 
	
	protected:
		virtual CParallelForTaskBase* CreateTask() override { return new ( CTask::Root ) CParallelForTaskDoubleArray( *this ); }
	};

protected:
	const TInputElement*				m_inputArray;
	TOutputElement*						m_outputArray;
	TProcessFunc						m_processFunc;
	TProcessFunctionOwner*				m_processFuncOwner;

	CParallelForTaskDoubleArray( SParams& params ) 
		: CParallelForTaskBase( params )
		, m_inputArray( params.m_inputArray )
		, m_outputArray( params.m_outputArray )
		, m_processFunc( params.m_processFunc )
		, m_processFuncOwner( params.m_processFuncOwner )
	{
	}

	void Run() override
	{
		m_params.OnTaskStarted();
		{
			#ifdef PROFILE_PARALLEL_FOR
				Red::ScopedStopClock clk( m_timer );
			#endif // PROFILE_PARALLEL_FOR

			Int32 currentElement = m_atomicCounter.Increment();
			while ( currentElement < m_numElements )
			{
				// Call process func
				( m_processFuncOwner->*m_processFunc )( m_inputArray[ currentElement ], m_outputArray[ currentElement ] ); 

				// Bump the counter
				currentElement = m_atomicCounter.Increment();
			};
		}
		m_params.OnTaskFinished();
	}
};

