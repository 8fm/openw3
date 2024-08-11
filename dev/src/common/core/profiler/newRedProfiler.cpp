/**
* Copyright © 2013-14 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "newRedProfiler.h"
#include "profilerFile.h"
#include "../configVar.h"
#include "../fileSys.h"
#include "../memoryHelpers.h"

namespace Config
{

	TConfigVar< Int32, Validation::IntRange<1024*1024,INT_MAX> > cvBufferSize( "Profiler", "BufferSize", 100*1024*1024, eConsoleVarFlag_ReadOnly );
	TConfigVar< String > cvOutputPath( "Profiler", "OutputPath", TXT(""), eConsoleVarFlag_ReadOnly );

} // Config

#ifndef NO_EDITOR
#include "../events.h"
#endif

//////////////////////////////////////////////////////////////////////////
// defines
#define FRAME_QWORDS_COUNT 3
#define START_QWORDS_COUNT 3
#define END_QWORDS_COUNT 3
#define SIGNAL_QWORDS_COUNT 3
#define STARTPROFILE_QWORDS_COUNT FRAME_QWORDS_COUNT + 2 

RED_TLS Red::System::Internal::ThreadId* g_thredId = nullptr;
RED_FORCE_INLINE static const Red::System::Internal::ThreadId* GetThredId()
{
	if( g_thredId == nullptr )
	{
		g_thredId = new Red::System::Internal::ThreadId();
		*g_thredId = Red::System::Internal::ThreadId::CurrentThread();
	}
	return g_thredId;
}
#define GET_THREAD_ID GetThredId()->id

// manual debug usage of profiler - dump timing for block start/stop to the log
namespace DebugProfiler
{
	class CDebugProfilerPrinter
	{
	public:
		CDebugProfilerPrinter();

		void Start( const Char* name );
		void End();

		void SignalStart( const char* name, Red::System::Timer& timer );
		void SignalEnd( const char* name, Red::System::Timer& timer );

	private:
		static const Uint32 MAX_DEBUG_PROFILER_DEPTH = 30;
		static const Uint32 MAX_ENTRIES = 1 << 20; // 1M

		struct Entry
		{
			Uint64			m_namePtr;	// bit0 is used to mark push/pop
			Uint64			m_ticks;
		};

		Uint32 m_enabled;
		Int32 m_depth;
		Entry m_entries[ MAX_ENTRIES ];
		Uint32 m_numEntries;
		String m_name;

		void Dump( const Char* name );

	} GDebugProfilerPrinter;

	CDebugProfilerPrinter::CDebugProfilerPrinter()
		: m_enabled( 0 )
		, m_numEntries( 0 )
		, m_depth( 0 )
	{}

	void CDebugProfilerPrinter::Start( const Char* name )
	{
		if ( 0 == m_enabled++ )
		{
			m_name = name;
			m_numEntries = 0;
			m_depth = 0;
		}
	}

	void CDebugProfilerPrinter::End()
	{
		if ( 0 == --m_enabled )
		{
			Dump( m_name.AsChar() );
			m_name = String::EMPTY;
		}
	}

	void CDebugProfilerPrinter::SignalStart( const char* name, Red::System::Timer& timer )
	{
		if ( m_enabled && ::SIsMainThread() )
		{
			if ( m_depth < MAX_DEBUG_PROFILER_DEPTH )
			{
				if ( m_numEntries < MAX_ENTRIES )
				{
					m_entries[ m_numEntries ].m_namePtr = (Uint64)(name) | 1; // start
					m_entries[ m_numEntries ].m_ticks = timer.GetTicks();
					++m_numEntries;
				}
			}

			++m_depth;
		}
	}

	void CDebugProfilerPrinter::SignalEnd( const char* name, Red::System::Timer& timer )
	{
		if ( m_enabled && ::SIsMainThread() )
		{
			--m_depth;

			if ( m_depth >= 0 && m_depth < MAX_DEBUG_PROFILER_DEPTH )
			{
				if ( m_numEntries < MAX_ENTRIES )
				{
					m_entries[ m_numEntries ].m_namePtr = (Uint64)(name) | 0; // end
					m_entries[ m_numEntries ].m_ticks = timer.GetTicks();
					++m_numEntries;
				}
			}
		}
	}

	void CDebugProfilerPrinter::Dump( const Char* name )
	{
		// get current system time
		Red::DateTime time;
		Red::Clock::GetInstance().GetUTCTime( time );

		// get base output path
		const Char* baseOutputPath = GFileManager->GetUserDirectory().AsChar();

		// format file name
		Char fileName[ 512 ];
		Red::SNPrintF( fileName, ARRAY_COUNT_U32(fileName), TXT("%lsprofile_%ls_[%04d_%02d_%02d][%02d_%02d_%02d].txt"),
			baseOutputPath, name,
			time.GetYear(), time.GetMonth(), time.GetDay(), time.GetHour(), time.GetMinute(), time.GetSecond() );

		LOG_CORE( TXT("Dumping profiling data to '%ls'..."), fileName );

		// open file writer
#if defined(RED_PLATFORM_ORBIS)
		FILE* f = fopen( UNICODE_TO_ANSI(fileName), "w" );
#else
		FILE* f = _wfopen( fileName, L"w" );
#endif
		if ( f )
		{
			Uint32 depthLevel = 0;

			// frequency
			Uint64 freq = 1;
			Red::System::Timer timer;
			timer.GetFrequency( freq );

			// starting times for each depth
			Uint64 startTicks[ MAX_DEBUG_PROFILER_DEPTH ];

			// process entries
			fwprintf( f, TXT("%d profiling entries\n"), m_numEntries );
			for ( Uint32 i=0; i<m_numEntries; ++i )
			{
				const Entry& info = m_entries[ i ];

				// start ?
				const char* name = (const char*)( info.m_namePtr & ~1 );
				if ( info.m_namePtr & 1 )
				{
					startTicks[ depthLevel ] = info.m_ticks;

					// format lead string
					AnsiChar leadString[ MAX_DEBUG_PROFILER_DEPTH*2 + 1 ];
					for ( Uint32 i=0; i<depthLevel; ++i )
					{
						leadString[i] = '\t';
					}
					leadString[depthLevel] = 0;

					/*if ( depthLevel > 0 )
					{
						const Uint64 delta = startTicks[ depthLevel ] - startTicks[ depthLevel-1 ];
						fprintf( f, "%s+%s @%1.4f\n", leadString, name, ((Double)(delta) / (Double)freq) * 1000.0 );
					}
					else*/
					{
						fprintf( f, "%s+%s\n", leadString, name );
					}

					++depthLevel;
				}
				else if ( depthLevel > 0 )
				{
					--depthLevel;

					// format lead string
					AnsiChar leadString[ MAX_DEBUG_PROFILER_DEPTH*2 + 1 ];
					for ( Uint32 i=0; i<depthLevel; ++i )
					{
						leadString[i] = '\t';
					}
					leadString[depthLevel] = 0;

					const Uint64 delta = info.m_ticks - startTicks[ depthLevel ];
					fprintf( f, "%s-%s %1.4f\n", leadString, name, ((Double)(delta) / (Double)freq) * 1000.0 );
				}
			}

			fclose(f);
		}

		LOG_CORE( TXT("Finished dumping profiling data") );
	}

	void StartDebugProfiler( const Char* name )
	{
		GDebugProfilerPrinter.Start( name );
	}

	void EndDebugProfiler()
	{
		GDebugProfilerPrinter.End();
	}
};

//////////////////////////////////////////////////////////////////////////
// implementations

//////////////////////////////////////////////////////////////////////////
// namespaces
namespace NewRedProfiler
{    
    //////////////////////////////////////////////////////////////////////////
    //
    // ctor
    ProfilerManager::ProfilerManager():
		//! control flags
	m_init( false ),
	m_started( false ),
	m_EOM( false ),   
	m_EOM_String( false ),
	m_stored( false ),
	m_changingState( false ),
	m_instrFuncConfigLoaded( false ),
#ifdef NEW_PROFILER_BREAKPOINTS_ENABLED
	m_catchBreakpointStarted( false ),
#endif
	//! blocks data
	m_reservedMem( 0 ),
	m_currentPos( nullptr ),
	m_endStopFramePos( nullptr ),
	m_mem( nullptr ),
	m_storedFrames( 0 ),
	m_storedBlocks( 0 ),
	m_storedSignals( 0 ),
	// strings data
#ifdef RED_ARCH_X64
	m_reservedMemSignalString( 0 ),
	m_memSignalStrings( nullptr ),
#endif //RED_ARCH_X64
	m_instrFuncsCounter( 0 )
    {

		// strings data
#ifdef RED_ARCH_X64
		SetCurrentPosSignalStrings( nullptr, 0 );
#endif //RED_ARCH_X64
				
		memset( m_instrFuncs, 0, sizeof(NewRedProfiler::InstrumentedFunction*)*PROFILER_MAX_PROFILES_COUNT );
		
#ifdef RED_ARCH_X64
		m_internalTicks.SetValue( 0 );
#endif
		m_startTicks = m_timer.GetTicks();
		m_stopTicks = m_startTicks;
    }
    //////////////////////////////////////////////////////////////////////////
    //
    // dtor
    ProfilerManager::~ProfilerManager()
    {

#ifdef NEW_PROFILER_BREAKPOINTS_ENABLED
		m_catchBreakpointStarted.SetValue( false );
#endif
        Stop();

        // free mem
        if ( m_mem )
		{
            RED_MEMORY_FREE( MemoryPool_Default, MC_Profiler, m_mem );
		}

#ifdef RED_ARCH_X64
		// free mem for signal strings
		if ( m_memSignalStrings )
		{
			RED_MEMORY_FREE( MemoryPool_Default, MC_Profiler, m_memSignalStrings );
		}
#endif 
        // free instrumented functions
        if (m_instrFuncsCounter.GetValue())
        {
            for ( Uint32 no = 0; no < m_instrFuncsCounter.GetValue(); ++no )
			{
                if ( m_instrFuncs[ no ] )
				{
                    RED_MEMORY_FREE( MemoryPool_Default, MC_Profiler, m_instrFuncs[ no ] );
				}
			}
        }
    }
    //////////////////////////////////////////////////////////////////////////
    //
    // init
    void ProfilerManager::Init( const Uint32 _mem, const Uint32 _memSignalString )
    {
		if( m_changingState.CompareExchange( true, false ) == false )
		{
			Uint32 mem				= _mem;
			Uint32 memSignalString	= _memSignalString;

			if ( mem == 0 )
				mem = Config::cvBufferSize.Get();

			if( mem == 0 )
			{
				m_changingState.SetValue( false );
				return;
			}

			if( memSignalString == 0 )
				memSignalString = mem/10;

			memSignalString += 19; // for default text "STRING_MEMORY_FULL"

			const String& outputDir = Config::cvOutputPath.Get();
			if( outputDir.Empty() )
			{
				m_outputDirs.PushBack( TXT("profileFiles") );
			}
			else
			{
				m_outputDirs = outputDir.Split(TXT(";"));
			}

			if ( m_mem )
			{
				RED_MEMORY_FREE( MemoryPool_Default, MC_Profiler, m_mem );
			}
			m_mem = (Uint64*)RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_Profiler, mem );
			memset( m_mem, 0, mem );
			m_reservedMem	= mem;
			m_currentPos.SetValue( m_mem );
			m_endStopFramePos = m_mem;
			m_storedFrames.SetValue( 0 );
			m_storedBlocks.SetValue( 0 );
			m_storedSignals.SetValue( 0 );
			
#ifdef RED_ARCH_X64
			if ( m_memSignalStrings )
			{
				RED_MEMORY_FREE( MemoryPool_Default, MC_Profiler, m_memSignalStrings );
			}
			m_memSignalStrings = (Uint8*)RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_Profiler, memSignalString );
			memset( m_memSignalStrings, 0, memSignalString );
			memcpy( m_memSignalStrings, UNICODE_TO_ANSI( TXT("STRING_MEMORY_FULL") ), 18 );
			m_memSignalStrings[19] = '\0';			
			m_reservedMemSignalString	= memSignalString;
			SetCurrentPosSignalStrings( m_memSignalStrings + 19, 1 );
#endif

			m_instrFuncConfigLoaded.SetValue( LoadInstrFuncEnabledFile() );
			DisableIntrFuncs();
	       
			m_EOM.SetValue( false );
			m_EOM_String.SetValue( false );
			m_started.SetValue( false );
			m_stored = false;
		
#ifdef RED_ARCH_X64
			m_internalTicks.SetValue( 0 );
#endif
			m_startTicks = m_timer.GetTicks();
			m_stopTicks = m_startTicks;
			
			RED_LOG( RED_LOG_CHANNEL(Profiler), TXT("%d bytes buffer initiated [%p-%p].."), m_reservedMem, m_mem, m_mem+mem );
			
			m_init.SetValue( true );

			m_changingState.SetValue( false );
		}
		else
		{
			RED_LOG_WARNING( RED_LOG_CHANNEL(Profiler), TXT( "Another changing profiler state command processing!!!" ) );
		}
    }
    //////////////////////////////////////////////////////////////////////////
    //
    // return internal memory fill factor
    Float ProfilerManager::GetFilledBufferFactor()
    {
		Double bufferFactor = 0.0;
		if( m_reservedMem )
		{
			Double factor = ((Double)sizeof( Uint64 ))/((Double)m_reservedMem);
			RED_THREADS_MEMORY_BARRIER(); // get m_currentPos.GetValue() in last posible moment
			bufferFactor = (m_currentPos.GetValue()-m_mem)*factor;
		}
        return (Float)bufferFactor;
    }
    //////////////////////////////////////////////////////////////////////////
    //
    // return no. of stored frames
    Uint32 ProfilerManager::GetStoredFrames()
    {
        return m_storedFrames.GetValue();
    }
	//////////////////////////////////////////////////////////////////////////
	//
	// return stored blocks count
	Uint32 ProfilerManager::GetStoredBlocks()
	{
		return m_storedBlocks.GetValue();
	}
	//////////////////////////////////////////////////////////////////////////
	//
	// return stored signals count
	Uint32 ProfilerManager::GetStoredSignals()
	{
		return m_storedSignals.GetValue();
	}
	//////////////////////////////////////////////////////////////////////////
	//
	// return total profiler timer
	Double ProfilerManager::GetProfilingTime()
	{
		if ( m_started.GetValue() )  return (Double)(m_timer.GetTicks()-m_startTicks) / m_timer.GetFrequency();
		return (Double)(m_stopTicks-m_startTicks) / m_timer.GetFrequency();
	}
	//////////////////////////////////////////////////////////////////////////
	//
	// return time spend on profiler internal functions
	Double ProfilerManager::GetInternalTime()
	{
		#ifdef RED_ARCH_X64
		return (Double)m_internalTicks.GetValue() / m_timer.GetFrequency();
		#else
		return 0.0;
		#endif
	}
	//////////////////////////////////////////////////////////////////////////
	//
	// return registered functions count
	Uint32 ProfilerManager::GetRegisteredFunctionsCount()
	{
		return m_instrFuncsCounter.GetValue();
	}
	//////////////////////////////////////////////////////////////////////////
	//
	// return no. of stored frames
	Bool ProfilerManager::IsStarted()
	{
		return m_started.GetValue();
	}
    //////////////////////////////////////////////////////////////////////////
    //
    // check does current pos is in memory range, count as 64 bits
	// return pointer to memory block, if not return null
    Uint64* ProfilerManager::GetBufferBlock64Bits( const Uint32 num64bits )
    {
        if ( m_EOM.GetValue() )
		{
            return NULL;
		}
		Uint64* newCurrentPos;   
		Uint64* oldCurrentPos; 
		do 
		{
			oldCurrentPos = m_currentPos.GetValue();
			newCurrentPos = oldCurrentPos + num64bits;
			// Plus 3*64 bits for next frame marker (invoked in ProfilerManager::Store)
			if ( (Uint32)((newCurrentPos-m_mem)*sizeof(Uint64) + FRAME_QWORDS_COUNT*sizeof(Uint64)) > m_reservedMem )
			{
				RED_LOG_WARNING( RED_LOG_CHANNEL(Profiler), TXT("Memory is full!!") );
				Stop();
				m_EOM.SetValue( true );
				return NULL;
			}
		} while ( m_currentPos.CompareExchange( newCurrentPos, oldCurrentPos ) != oldCurrentPos );
		
        return oldCurrentPos;
    }
    //////////////////////////////////////////////////////////////////////////
    //
    // insert start block
#ifdef NEW_PROFILER_BREAKPOINTS_ENABLED
    Uint64 ProfilerManager::StartBlock( InstrumentedFunction* instrFunc )
    {
		Uint64 time = 0;

		if ( !instrFunc )
		{
            return time;
		}

#ifdef RED_ARCH_X64
		DebugProfiler::GDebugProfilerPrinter.SignalStart( instrFunc->m_name, m_timer );
#endif
						 
		if( m_started.GetValue() )
		{
			if( instrFunc->m_enabled.GetValue() == false )
				return time;

			time = m_timer.GetTicks();

			Uint64* bufferPtr = GetBufferBlock64Bits( START_QWORDS_COUNT );

			if ( bufferPtr != NULL )
			{		    
				*bufferPtr++ = 0x1111111111110000 | instrFunc->m_id;
				*bufferPtr++ = GET_THREAD_ID;
				*bufferPtr   = time;
				m_storedBlocks.Increment();
			}

		}
		else if( m_catchBreakpointStarted.GetValue() ) 
		{
			if( instrFunc->m_enabled.GetValue() == false )
				return time;
			
			time = m_timer.GetTicks();

			if( instrFunc->m_breakpointAtHitCount.GetValue() != 0 )
			{
				Uint32 oldHitCount;
				Uint32 newHitCount;
				do 
				{
					oldHitCount = instrFunc->m_breakpointAtHitCount.GetValue();
					if( oldHitCount == 0 )
					{
						newHitCount = 0xFFFFFFFF; //! max value to avoid hit breakpoint (another counter hit breakpoint earlier)
						break;
					}
					newHitCount = oldHitCount - 1;				

				} while ( instrFunc->m_breakpointAtHitCount.CompareExchange( newHitCount, oldHitCount ) != oldHitCount );

				if( newHitCount == 0 )
				{
					RED_LOG_WARNING( RED_LOG_CHANNEL(Profiler), TXT("Breakpoint hit: hit count on %s reached!!!"), ANSI_TO_UNICODE(instrFunc->m_name) );
					RED_BREAKPOINT();
				}
			}			
		}
		else
		{
			return time;
		}

		// internal timer
		#ifdef RED_ARCH_X64
		const Uint64 timeDelta = (m_timer.GetTicks()-time);
		RED_THREADS_MEMORY_BARRIER();
		m_internalTicks.SetValue( timeDelta + m_internalTicks.GetValue() );
		#endif
		return time;
    }
#else
	void ProfilerManager::StartBlock( InstrumentedFunction* instrFunc )
	{
		if ( !instrFunc || !m_started.GetValue() )
		{
			return;
		}

		if( instrFunc->m_enabled.GetValue() == false )
			return;

		const Uint64 time = m_timer.GetTicks();

		Uint64* bufferPtr = GetBufferBlock64Bits( START_QWORDS_COUNT );

		if ( bufferPtr != NULL )
		{		    
			*bufferPtr++ = 0x1111111111110000 | instrFunc->m_id;
			*bufferPtr++ = GET_THREAD_ID;
			*bufferPtr   = time;
			m_storedBlocks.Increment();
		}

		// internal timer
#ifdef RED_ARCH_X64
		const Uint64 timeDelta = (m_timer.GetTicks()-time);
		RED_THREADS_MEMORY_BARRIER();
		m_internalTicks.SetValue( timeDelta + m_internalTicks.GetValue() );
#endif
	}
#endif
    //////////////////////////////////////////////////////////////////////////
    //
    // insert end of block
#ifdef NEW_PROFILER_BREAKPOINTS_ENABLED
	void ProfilerManager::EndBlock( InstrumentedFunction* instrFunc, Uint64 startTime )
	{
		Uint64 time = 0;

		if ( !instrFunc )
		{
			return;
		}

#ifdef RED_ARCH_X64
		DebugProfiler::GDebugProfilerPrinter.SignalEnd( instrFunc->m_name, m_timer );
#endif

		if( m_started.GetValue() )
		{
			if( instrFunc->m_enabled.GetValue() == false )
				return;

			time = m_timer.GetTicks();

			Uint64* bufferPtr = GetBufferBlock64Bits( END_QWORDS_COUNT );

			if ( bufferPtr != NULL )
			{
				*bufferPtr++ = 0x2222222222220000 | instrFunc->m_id;
				*bufferPtr++ = GET_THREAD_ID;
				*bufferPtr   = time;		    
			}
		}
		else if( m_catchBreakpointStarted.GetValue() && instrFunc->m_breakpointAtExecTime != 0.0f )
		{
			if( instrFunc->m_enabled.GetValue() == false )
				return;

			time = m_timer.GetTicks();

			Uint64 execTime = time-startTime;
			Float ticksPerMs =  (Float)(m_timer.GetFrequency()/1000.0);
			Float execTimeInMs = execTime/ticksPerMs;// exec time in ms
			if( instrFunc->m_breakpointAtExecTime < execTimeInMs )
			{
				Bool chackBreakConndition = false;
				if( instrFunc->m_breakOnce )
				{
					if( instrFunc->m_onceStopped.CompareExchange( true, false ) == false )
					{
						chackBreakConndition = true;
					}				
				}
				else
				{
					chackBreakConndition = true;
				}
				if( chackBreakConndition )
				{
					RED_LOG_WARNING( RED_LOG_CHANNEL(Profiler), TXT("Breakpoint hit: execution time (%fms) of %s is longer then %fms!!!"), execTimeInMs, ANSI_TO_UNICODE(instrFunc->m_name), instrFunc->m_breakpointAtExecTime );
					RED_BREAKPOINT();
				}
			}
		}
		else
		{
			return;
		}

		// internal timer
#ifdef RED_ARCH_X64
		const Uint64 timeDelta = (m_timer.GetTicks()-time);
		RED_THREADS_MEMORY_BARRIER();
		m_internalTicks.SetValue( timeDelta + m_internalTicks.GetValue() );
#endif
	}
#else
    void ProfilerManager::EndBlock( InstrumentedFunction* instrFunc )
    {
		if ( !instrFunc || !m_started.GetValue() )
		{
			return;
		}

		if( instrFunc->m_enabled.GetValue() == false )
			return;

		const Uint64 time = m_timer.GetTicks();

		Uint64* bufferPtr = GetBufferBlock64Bits( END_QWORDS_COUNT );

		if ( bufferPtr != NULL )
		{
			*bufferPtr++ = 0x2222222222220000 | instrFunc->m_id;
			*bufferPtr++ = GET_THREAD_ID;
			*bufferPtr   = time;		    
        }

		// internal timer
		#ifdef RED_ARCH_X64
		const Uint64 timeDelta = (m_timer.GetTicks()-time);
		RED_THREADS_MEMORY_BARRIER();
		m_internalTicks.SetValue( timeDelta + m_internalTicks.GetValue() );
		#endif
    }
#endif
    //////////////////////////////////////////////////////////////////////////
    //
    // simple signal
    void ProfilerManager::Signal( InstrumentedFunction* instrFunc )
    {
		if ( !instrFunc || !m_started.GetValue() )
		{
			return;
		}

		if( instrFunc->m_enabled.GetValue() == false )
			return;

		const Uint64 time = m_timer.GetTicks();

		Uint64* bufferPtr = GetBufferBlock64Bits( SIGNAL_QWORDS_COUNT );

		if ( bufferPtr != NULL )
		{			
            *bufferPtr++ = 0x3333333333330000 | instrFunc->m_id;
			*bufferPtr++ = GET_THREAD_ID;
            *bufferPtr   = time;
			m_storedSignals.Increment();
        }

		// internal timer
		#ifdef RED_ARCH_X64
		const Uint64 timeDelta = (m_timer.GetTicks()-time);
		RED_THREADS_MEMORY_BARRIER();
		m_internalTicks.SetValue( timeDelta + m_internalTicks.GetValue() );
		#endif
    }
    //////////////////////////////////////////////////////////////////////////
    //
    // signal with float value
    void ProfilerManager::Signal( InstrumentedFunction* instrFunc, const Float value )
    {
		if ( !instrFunc || !m_started.GetValue() )
		{
			return;
		}

		if( instrFunc->m_enabled.GetValue() == false )
			return;

		const Uint64 time = m_timer.GetTicks();

		Uint64* bufferPtr = GetBufferBlock64Bits( SIGNAL_QWORDS_COUNT );

		if ( bufferPtr != NULL )
		{		    
            *bufferPtr++ = 0x0000000044440000 | instrFunc->m_id | (((Uint64)*((Uint32*)&value))<<32);
			*bufferPtr++ = GET_THREAD_ID;
            *bufferPtr   = time;
			m_storedSignals.Increment();
        }

		// internal timer
		#ifdef RED_ARCH_X64
		const Uint64 timeDelta = (m_timer.GetTicks()-time);
		RED_THREADS_MEMORY_BARRIER();
		m_internalTicks.SetValue( timeDelta + m_internalTicks.GetValue() );
		#endif
    }
    //////////////////////////////////////////////////////////////////////////
    //
    // signal with int value
    void ProfilerManager::Signal( InstrumentedFunction* instrFunc, const Int32 value )
    {
		if ( !instrFunc || !m_started.GetValue() )
		{
			return;
		}

		if( instrFunc->m_enabled.GetValue() == false )
			return;

		const Uint64 time = m_timer.GetTicks();

		Uint64* bufferPtr = GetBufferBlock64Bits( SIGNAL_QWORDS_COUNT );

		if ( bufferPtr != NULL )
		{			
            *bufferPtr++ = 0x0000000055550000 | instrFunc->m_id | (((Uint64)value)<<32);
			*bufferPtr++ = GET_THREAD_ID;
            *bufferPtr   = time;
			m_storedSignals.Increment();
        }

		// internal timer
		#ifdef RED_ARCH_X64
		const Uint64 timeDelta = (m_timer.GetTicks()-time);
		RED_THREADS_MEMORY_BARRIER();
		m_internalTicks.SetValue( timeDelta + m_internalTicks.GetValue() );
		#endif
    }
    //////////////////////////////////////////////////////////////////////////
    //
    // signal with string value
    void ProfilerManager::Signal( InstrumentedFunction* instrFunc, const Char* name)
    {
#ifdef RED_ARCH_X64
        if ( !instrFunc || !m_started.GetValue() || name == NULL || m_EOM_String.GetValue() )
		{
            return;
		}

		if( instrFunc->m_enabled.GetValue() == false )
			return;

		const Uint64 time = m_timer.GetTicks();
		Uint64* bufferPtr = GetBufferBlock64Bits( SIGNAL_QWORDS_COUNT );

		if ( bufferPtr != NULL )
		{			
			Uint32 strSize = (Uint32)Red::System::StringLength( name ) + 1;

			Uint64 newValue = 0;
			Uint64 oldValue = 0;
			Uint8* oldCurrentPosSignalStrings = NULL;
			Uint32 oldStoredSignalStrings = 0;
			do 
			{
				oldValue = GetCurrentPosSignalStrings( &oldCurrentPosSignalStrings, &oldStoredSignalStrings );
				newValue = (Uint32)(oldCurrentPosSignalStrings - m_memSignalStrings);
				newValue += strSize;
				if( newValue > m_reservedMemSignalString )
				{
					newValue = 0;
					break;
				}
				newValue = newValue << 32;
				newValue |= (Uint64)(oldStoredSignalStrings+1);
			} while ( m_currPosSignalStrings.CompareExchange( newValue, oldValue ) != oldValue );

			if( newValue )
			{
				// copy name				
				memcpy( oldCurrentPosSignalStrings, UNICODE_TO_ANSI( name ), strSize-1 );
				oldCurrentPosSignalStrings[strSize-1] = '\0';
				*bufferPtr++ = 0x0000000066660000 | instrFunc->m_id | ( ( (Uint64)oldStoredSignalStrings ) << 32 );					
			}
			else
			{
				*bufferPtr++ = 0x0000000066660000 | instrFunc->m_id ;
				RED_LOG_WARNING( RED_LOG_CHANNEL(Profiler), TXT("String memory is full!!") );
				m_EOM_String.SetValue( true );
			}			
			*bufferPtr++ = GET_THREAD_ID;
			*bufferPtr = time;
        }
		
		// internal timer
		const Uint64 timeDelta = (m_timer.GetTicks()-time);
		RED_THREADS_MEMORY_BARRIER();
		m_internalTicks.SetValue( timeDelta + m_internalTicks.GetValue() );
#else
		RED_UNUSED(instrFunc);
		RED_UNUSED(name);
#endif //RED_ARCH_X64
    }
    //////////////////////////////////////////////////////////////////////////
    //
    // insert next frame marker
    void ProfilerManager::NextFrame( void )
    {
        if ( !m_started.GetValue() )
		{
            return;
		}

		const Uint64 time = m_timer.GetTicks();
		Uint64* bufferPtr = GetBufferBlock64Bits( FRAME_QWORDS_COUNT );

		if ( bufferPtr != NULL )
		{			
			*bufferPtr++ = 0xFFFFFFFF00000001;
			*bufferPtr++ = (Uint64)Memory::GetTotalBytesAllocated();
			*bufferPtr   = time;
			m_storedFrames.Increment();
        }

		// internal timer
		#ifdef RED_ARCH_X64
		const Uint64 timeDelta = (m_timer.GetTicks()-time);
		RED_THREADS_MEMORY_BARRIER();
		m_internalTicks.SetValue( timeDelta + m_internalTicks.GetValue() );
		#endif
    }

    //////////////////////////////////////////////////////////////////////////
    //
    // start profiling
    void ProfilerManager::Start()
    {
		if ( !m_init.GetValue() )
		{
			Init( 0, 0 ); // if NewRedProfiler.ini have BufferSize entry, initialization should be OK
		}

		if( m_changingState.CompareExchange( true, false ) == false )
		{
#ifdef NEW_PROFILER_BREAKPOINTS_ENABLED
			if ( m_catchBreakpointStarted.GetValue() )
			{
				RED_LOG_WARNING( RED_LOG_CHANNEL(Profiler), TXT( "Profiling can not be started during catch breakpoint!!!" ) );
				m_changingState.SetValue( false );
				return;
			}
#endif
			if ( !m_init.GetValue() )
			{
				RED_LOG_WARNING( RED_LOG_CHANNEL(Profiler), TXT( "Profiling isn't initialized!!!" ) );
				m_changingState.SetValue( false );
				return;
			}
			else if( m_started.GetValue() )
			{
				RED_LOG_WARNING( RED_LOG_CHANNEL(Profiler), TXT( "Profiling already started!!!" ) );
				m_changingState.SetValue( false );
				return;
			}

			RED_LOG( RED_LOG_CHANNEL(Profiler), TXT( "started.." ) );

			m_currentPos.SetValue( m_mem );
			m_endStopFramePos = m_mem;
			m_storedFrames.SetValue( 0 );
			m_storedBlocks.SetValue( 0 );
			m_storedSignals.SetValue( 0 );

#ifdef RED_ARCH_X64
			SetCurrentPosSignalStrings( m_memSignalStrings + 19, 1 );
#endif

			m_EOM.SetValue( false );
			m_EOM_String.SetValue( false );
			m_started.SetValue( false );
			m_stored = false;

			DisableIntrFuncs();

			// store freq and first frame at start        
			Uint64 freq = 0;
			m_timer.GetFrequency( freq );

			Uint64* bufferPtr = GetBufferBlock64Bits( STARTPROFILE_QWORDS_COUNT );
		
			if ( bufferPtr != NULL )
			{		
				*bufferPtr++ = 0xFFFFFFFFFFFFFFFF;
				*bufferPtr++ = freq;
				*bufferPtr++ = 0xFFFFFFFF00000001;
				*bufferPtr++ = (Uint64)Memory::GetTotalBytesAllocated();
				*bufferPtr   = m_startTicks;
				m_storedFrames.Increment();
			}

#ifdef RED_ARCH_X64
			m_internalTicks.SetValue( 0 );
#endif
			m_startTicks = m_timer.GetTicks();
			m_stopTicks = m_startTicks;

			m_started.SetValue( true );

#ifndef NO_EDITOR
#ifndef RED_PLATFORM_CONSOLE
			SEvents::GetInstance().QueueEvent( CNAME( ProfilerChangeState ), CreateEventData( m_started.GetValue() ) );
#endif
#endif

			m_changingState.SetValue( false );
		}
		else
		{
			RED_LOG_WARNING( RED_LOG_CHANNEL(Profiler), TXT( "Another changing profiler state command processing!!!" ) );
		}
    }

    //////////////////////////////////////////////////////////////////////////
    //
    // stop profiling
    void ProfilerManager::Stop(  Bool skipChangingState /*= false*/  )
    {
		if( skipChangingState == true || m_changingState.CompareExchange( true, false ) == false )
		{
			if( m_started.GetValue() == false )
			{
				RED_LOG_WARNING( RED_LOG_CHANNEL(Profiler), TXT( "Profiling not started!!!" ) );
				m_changingState.SetValue( false );
				return;
			}
		
			const Uint64 time = m_timer.GetTicks();
						
			m_stopTicks = time;
			
			m_started.SetValue( false );
#ifndef NO_EDITOR
#ifndef RED_PLATFORM_CONSOLE
			SEvents::GetInstance().QueueEvent( CNAME( ProfilerChangeState ), CreateEventData( m_started.GetValue() ) );
#endif
#endif
			// Save last frame
			Uint64* bufferPtr = GetBufferBlock64Bits( 0 ); // should be reserved
			if ( bufferPtr != NULL )
			{			
				*bufferPtr++ = 0xFFFFFFFF00000001;
				*bufferPtr++ = (Uint64)Memory::GetTotalBytesAllocated();
				*bufferPtr++ = time;
				m_endStopFramePos = bufferPtr;
				m_storedFrames.Increment();
			}

			RED_LOG( RED_LOG_CHANNEL(Profiler), TXT( "Stopped.." ) );

			// internal timer
#ifdef RED_ARCH_X64
			const Uint64 timeDelta = (m_timer.GetTicks()-time);
			RED_THREADS_MEMORY_BARRIER();
			m_internalTicks.SetValue( timeDelta + m_internalTicks.GetValue() );
#endif
			if( skipChangingState == false )
			{
				m_changingState.SetValue( false );
			}
		}
		else
		{
			RED_LOG_WARNING( RED_LOG_CHANNEL(Profiler), TXT( "Another changing profiler state command processing!!!" ) );
		}
    }
    //////////////////////////////////////////////////////////////////////////
    //
    // store profile data
    bool ProfilerManager::Store( const String& filename, const String& sessionId /*= String::EMPTY*/, Uint64 sessionQpc /*= 0*/, Uint64 sessionQpf /*= 0*/ )
    {
		if ( !m_init.GetValue() )
		{
			RED_LOG_WARNING( RED_LOG_CHANNEL(Profiler), TXT( "Profiling isn't initialized!!!" ) );
			return false;
		}

		if( m_changingState.CompareExchange( true, false ) == false )
		{
			// stop profiler
			if ( m_started.GetValue() )
			{
				Stop( true );
			}

			m_outputPath.Clear();

	#if defined( RED_PLATFORM_DURANGO )		
			m_outputPath = TXT( "d:\\" );
	#elif defined( RED_PLATFORM_ORBIS )
			m_outputPath = TXT( "/data/" );
	#else

			// Find existed file path
			for( Red::System::Uint32 index = 0; index < m_outputDirs.Size(); index++ )
			{
				if( GSystemIO.FileExist( m_outputDirs[index].AsChar() ) )
				{
					if( GSystemIO.IsFileReadOnly( m_outputDirs[index].AsChar() ) == false )
					{
						m_outputPath  = m_outputDirs[index];
						break;
					}				
				}			
			}
	#endif

			if( m_outputPath.Empty() == false )
			{

		#if defined( RED_PLATFORM_WIN32 ) || defined ( RED_PLATFORM_WIN64 )		
				Char lpszUsername[255];
				DWORD dUsername = sizeof(lpszUsername);
				if(GetUserName(lpszUsername, &dUsername))
				{
					m_outputPath += TXT("/");
					m_outputPath += lpszUsername;
				}
		#endif
				GSystemIO.CreateDirectory( m_outputPath.AsChar() );
				String outputSessionIndexesFolder = m_outputPath + TXT("/session_indexes");
				String prdFileName;
				// check path
				if ( filename.Empty() )
				{
					prdFileName = String::Printf( TXT( "default_%llu.prd"), m_timer.GetTicks() );				
				}
				else
				{
					prdFileName =  String::Printf( TXT( "%s.prd" ),  filename.AsChar() );
				}

				m_outputPath += TXT( "/") + prdFileName;

				RED_LOG( RED_LOG_CHANNEL(Profiler), TXT( "Storing profile data: [%s].." ), m_outputPath.AsChar());

				Uint8* currentPosSignalStrings = nullptr;
				Uint32 storedSignalStrings = 0;
				Uint8* memSignalStrings = nullptr;
#ifdef RED_ARCH_X64
				GetCurrentPosSignalStrings( &currentPosSignalStrings, &storedSignalStrings );
				memSignalStrings = m_memSignalStrings;
#endif

				ProfilerFile pFile;
				pFile.SetDataForSaving( m_mem, (Uint32)((m_endStopFramePos-m_mem)*sizeof(Int64)), (const InstrumentedFunction**)m_instrFuncs, m_instrFuncsCounter.GetValue(), memSignalStrings, (Uint32)((currentPosSignalStrings-memSignalStrings)), storedSignalStrings );
				if( pFile.SaveToFile(  m_outputPath.AsChar() ) == false )
				{
					RED_LOG_WARNING( RED_LOG_CHANNEL(Profiler), TXT( "Cannot save profile!!" ) );
				}
				else
				{
					if( m_stored == false ) // to avoid many profile session in one time in index session file
					{
						m_stored = true;
						if(sessionId.Empty() == false)
						{
							GSystemIO.CreateDirectory(outputSessionIndexesFolder.AsChar() );
							String sessionFile = outputSessionIndexesFolder + TXT("/") + sessionId + TXT(".txt");
							String newLine = String( TXT(";") );
							if( GSystemIO.FileExist( sessionFile.AsChar() ) )
							{
								IFile* writer = GFileManager->CreateFileWriter( sessionFile.AsChar(), FOF_AbsolutePath | FOF_Buffered | FOF_Append );
								if ( writer )
								{
									writer->Serialize( UNICODE_TO_ANSI( newLine.AsChar() ), newLine.GetLength() );

									// Path to PRD file
									writer->Serialize( UNICODE_TO_ANSI( prdFileName.AsChar() ), prdFileName.GetLength() );
									writer->Serialize( UNICODE_TO_ANSI( newLine.AsChar() ), newLine.GetLength() );

									// QPC for first frame of profile buffer
									String convertString = String::Printf( TXT( "%llu"), *(m_mem+(STARTPROFILE_QWORDS_COUNT-1)) );
									writer->Serialize( UNICODE_TO_ANSI( convertString.AsChar() ), convertString.GetLength() );
									writer->Serialize( UNICODE_TO_ANSI( newLine.AsChar() ), newLine.GetLength() );

									// QPC for last frame of profile buffer
									convertString = String::Printf( TXT( "%llu"), *(m_endStopFramePos-1) );
									writer->Serialize( UNICODE_TO_ANSI( convertString.AsChar() ), convertString.GetLength() );
									delete writer;
								}
							}
							else
							{
								IFile* writer = GFileManager->CreateFileWriter( sessionFile.AsChar(), FOF_AbsolutePath | FOF_Buffered );
								if ( writer )
								{	
									// QPC for start session timer
									String convertString = String::Printf( TXT( "%llu"), sessionQpc );
									writer->Serialize( UNICODE_TO_ANSI( convertString.AsChar() ), convertString.GetLength() );
									writer->Serialize( UNICODE_TO_ANSI( newLine.AsChar() ), newLine.GetLength() );

									// QPF for machine
									convertString = String::Printf( TXT( "%llu"), sessionQpf );
									writer->Serialize( UNICODE_TO_ANSI( convertString.AsChar() ), convertString.GetLength() );
									writer->Serialize( UNICODE_TO_ANSI( newLine.AsChar() ), newLine.GetLength() );

									// Path to PRD file
									writer->Serialize( UNICODE_TO_ANSI( prdFileName.AsChar() ), prdFileName.GetLength() );
									writer->Serialize( UNICODE_TO_ANSI( newLine.AsChar() ), newLine.GetLength() );

									// QPC for first frame of profile buffer
									convertString = String::Printf( TXT( "%llu"), *(m_mem+(STARTPROFILE_QWORDS_COUNT-1)) );
									writer->Serialize( UNICODE_TO_ANSI( convertString.AsChar() ), convertString.GetLength() );
									writer->Serialize( UNICODE_TO_ANSI( newLine.AsChar() ), newLine.GetLength() );

									// QPC for last frame of profile buffer
									convertString = String::Printf( TXT( "%llu"), *(m_endStopFramePos-1) );
									writer->Serialize( UNICODE_TO_ANSI( convertString.AsChar() ), convertString.GetLength() );
									delete writer;
								}
							}
						}
					}
				}
			}
			else
			{
				RED_LOG_WARNING( RED_LOG_CHANNEL(Profiler), TXT( "Cannot save profile (Output directory not exists)!!" ) );
			}
			m_changingState.SetValue( false );
		}
		else
		{
			RED_LOG_WARNING( RED_LOG_CHANNEL(Profiler), TXT( "Another changing profiler state command processing!!!" ) );
		}

        // def result
        return true;
    }

	Bool ProfilerManager::StoreToMem( CMemoryFileWriter* buffer )
	{
		if ( !buffer )
		{
			RED_LOG_WARNING( RED_LOG_CHANNEL(Profiler), TXT( "Buffer is NULL!!!" ) );
			return false;
		}

		if ( !m_init.GetValue() )
		{
			RED_LOG_WARNING( RED_LOG_CHANNEL(Profiler), TXT( "Profiling isn't initialized!!!" ) );
			return false;
		}

		if ( m_changingState.CompareExchange( true, false ) == false )
		{
			// stop profiler
			if ( m_started.GetValue() )
			{
				Stop( true );
			}

			RED_LOG( RED_LOG_CHANNEL(Profiler), TXT( "Storing profile data to mem: [0x%08x].." ), buffer );

			Uint8* currentPosSignalStrings = nullptr;
			Uint32 storedSignalStrings = 0;
			Uint8* memSignalStrings = nullptr;

			#ifdef RED_ARCH_X64
			GetCurrentPosSignalStrings( &currentPosSignalStrings, &storedSignalStrings );
			memSignalStrings = m_memSignalStrings;
			#endif

			ProfilerFile pFile;
			pFile.SetDataForSaving( m_mem, (Uint32)((m_endStopFramePos-m_mem)*sizeof(Int64)), (const InstrumentedFunction**)m_instrFuncs, m_instrFuncsCounter.GetValue(), memSignalStrings, (Uint32)((currentPosSignalStrings-memSignalStrings)), storedSignalStrings );
			Bool saved = pFile.SaveToBuffer( buffer );
			if( saved == false )
			{
				RED_LOG_WARNING( RED_LOG_CHANNEL(Profiler), TXT( "Cannot save profile!!" ) );
			}

			m_changingState.SetValue( false );
			
			return saved;
		}
		else
		{
			RED_LOG_WARNING( RED_LOG_CHANNEL(Profiler), TXT( "Another changing profiler state command processing!!!" ) );
			return false;
		}

		// def result
		return true;
	}

	Bool ProfilerManager::StoreInstrFuncList()
	{
		TDynArray<InstrumentedFunction> instrFuncArray;
		GetInstrFuncArray( instrFuncArray );
#ifdef RED_PLATFORM_DURANGO
		String pathToInstrFuncFile = TXT("d:\\");		
#else
		String pathToInstrFuncFile = GFileManager->GetUserDirectory();
#endif // RED_PLATFORM_DURANGO

		pathToInstrFuncFile += TXT("instrFuncList.txt");

		IFile* writer = GFileManager->CreateFileWriter( pathToInstrFuncFile.AsChar(), FOF_AbsolutePath | FOF_Buffered );
		if ( writer )
		{	
			for( TDynArray<InstrumentedFunction>::const_iterator iter = instrFuncArray.Begin(); iter != instrFuncArray.End(); ++iter )
			{
				size_t nameLength = strlen( iter->m_name );
				if( nameLength > 0 )
				{
					writer->Serialize( iter->m_name, nameLength );
					writer->Serialize( (void*)"\n", 1 );
				}				
			}
			delete writer;
		}

		// def result
		return true;
	}
    //////////////////////////////////////////////////////////////////////////
    //
    // register function 4 instrumentation
    InstrumentedFunction* ProfilerManager::RegisterInstrFunc( const char* name, Bool enable /*= true*/ )
    {
		const Uint32 oldCounter = m_instrFuncsCounter.Increment()-1;
		if( oldCounter >= PROFILER_MAX_PROFILES_COUNT )
		{
			m_instrFuncsCounter.Decrement();
			RED_LOG_WARNING( RED_LOG_CHANNEL(Profiler), TXT( "Maximum number [%i] of InstrumentedFunction registrations exceeded!!!" ), PROFILER_MAX_PROFILES_COUNT );
			return NULL;
		}

		InstrumentedFunction* instrFunc = new ( RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_Profiler, sizeof(InstrumentedFunction) ) ) InstrumentedFunction();
		instrFunc->m_id = oldCounter;
        instrFunc->m_name = const_cast<char*>(name);
		if( m_instrFuncConfigLoaded.GetValue() )
		{
			THashMap<StringAnsi, Bool>::iterator foundElement = m_instrFuncDisabled.Find( instrFunc->m_name );
			if( foundElement != m_instrFuncDisabled.End() )
			{
				instrFunc->m_enabled.SetValue( false );
			}
			else 
			{
				instrFunc->m_enabled.SetValue( enable );
			}
		}
		else 
		{
			instrFunc->m_enabled.SetValue( enable );
		}
		
		
		m_instrFuncs[oldCounter] = instrFunc;

        return instrFunc;
    }
	//////////////////////////////////////////////////////////////////////////
	//
	// disabled instrumented function
	void ProfilerManager::EnableInstrFunc( InstrumentedFunction* instrFunc, Bool enable )
	{
		instrFunc->m_enabled.SetValue( enable );
	}
	//////////////////////////////////////////////////////////////////////////
	//
	// disabled instrumented function by ID
	void ProfilerManager::EnableInstrFunc( Uint32 instrumentedFunctionId, Bool enable )
	{
		if( instrumentedFunctionId < PROFILER_MAX_PROFILES_COUNT && m_instrFuncs[instrumentedFunctionId] )
		{
			m_instrFuncs[instrumentedFunctionId]->m_enabled.SetValue( enable );
		}
	}
	//////////////////////////////////////////////////////////////////////////
	//
	// fill instrFuncArray with register instrumented functions names
	void ProfilerManager::GetInstrFuncArray( TDynArray<InstrumentedFunction>& instrFuncArray )
	{
		const Uint32 instrFuncCount = m_instrFuncsCounter.GetValue();
		Uint32 instrFuncIndex = 0;
		while ( instrFuncIndex < instrFuncCount)
		{
			instrFuncArray.PushBack( *m_instrFuncs[instrFuncIndex] );
			instrFuncIndex++;
		}		
	}
	//////////////////////////////////////////////////////////////////////////
	//
	// disable instrumented functions present in m_instrFuncDisabled list
	void ProfilerManager::DisableIntrFuncs()
	{
		if( m_instrFuncConfigLoaded.GetValue() )
		{
			const Uint32 instrFuncCount = m_instrFuncsCounter.GetValue();
			Uint32 instrFuncIndex = 0;
			while ( instrFuncIndex < instrFuncCount)
			{
				THashMap<StringAnsi, Bool>::iterator foundElement = m_instrFuncDisabled.Find( m_instrFuncs[instrFuncIndex]->m_name );
				if( foundElement != m_instrFuncDisabled.End() )
				{
					m_instrFuncs[instrFuncIndex]->m_enabled.SetValue( false );
				}
				++instrFuncIndex;
			}
		}				
	}
	////////////////////////////////////////////////////////////////////////////////////////////////////
	//
	// load list of disabled instrumented functions from instrFuncDisabled.txt m_instrFuncDisabled list
	Bool ProfilerManager::LoadInstrFuncEnabledFile()
	{
		Bool fileLoaded = false;

#ifdef RED_PLATFORM_DURANGO
		String pathToInstrFuncEnabledFile = TXT("d:\\");		
#else
		String pathToInstrFuncEnabledFile = GFileManager->GetUserDirectory();
#endif // RED_PLATFORM_DURANGO

		pathToInstrFuncEnabledFile += TXT("instrFuncDisabled.txt");

		RED_LOG( RED_LOG_CHANNEL(Profiler), TXT( "Try to load: %s" ), pathToInstrFuncEnabledFile.AsChar() );

		IFile* instrFuncDisabledFileData = GFileManager->CreateFileReader( pathToInstrFuncEnabledFile, FOF_Buffered|FOF_AbsolutePath );

		if( instrFuncDisabledFileData )
		{
			RED_LOG( RED_LOG_CHANNEL(Profiler), TXT( "File %s opened." ), pathToInstrFuncEnabledFile.AsChar() );
			size_t dataSize = (size_t)instrFuncDisabledFileData->GetSize();
			if( dataSize > 0 )
			{
				Uint8* buffer = new Uint8[dataSize];
				instrFuncDisabledFileData->Serialize( buffer, dataSize ); 
				StringAnsi newInstFuncName;
				Uint32 dataIndex = 0;
				while ( dataIndex < dataSize )
				{
					if( buffer[dataIndex] == '\n' )
					{
						if( newInstFuncName.GetLength() > 0 )
						{
							m_instrFuncDisabled.Set( newInstFuncName, false );
							fileLoaded = true;
						}	
						newInstFuncName.Clear();
					}
					else if( buffer[dataIndex] == '\r' )
					{
						//skip this character
					}
					else
					{
						newInstFuncName.Append( buffer[dataIndex] );
					}
					++dataIndex;
				}
				if( newInstFuncName.GetLength() > 0 )
				{
					m_instrFuncDisabled.Set( newInstFuncName, false );
					fileLoaded = true;
				}
			}
			delete instrFuncDisabledFileData;
			RED_LOG( RED_LOG_CHANNEL(Profiler), TXT( "%i instrumented functions disabled." ), m_instrFuncDisabled.Size() );
		}
		else 
		{
			RED_LOG( RED_LOG_CHANNEL(Profiler), TXT( "Could not open %s." ), pathToInstrFuncEnabledFile.AsChar() );
		}
		return fileLoaded;
	}

#ifdef RED_ARCH_X64
	////////////////////////////////////////////////////////////////////////////////////////////////////
	//
	// set atomic address and counter for string data
	void ProfilerManager::SetCurrentPosSignalStrings( const Uint8* newAddress, const Uint32 counter )
	{
		Uint64 newValue = (Uint64)(newAddress - m_memSignalStrings);
		newValue = newValue << 32;
		newValue |= counter;
		m_currPosSignalStrings.SetValue( newValue );
	}
	////////////////////////////////////////////////////////////////////////////////////////////////////
	//
	// get atomic address and counter of string data
	Uint64 ProfilerManager::GetCurrentPosSignalStrings( Uint8** newAddress, Uint32* counter )
	{
		Uint64 value = m_currPosSignalStrings.GetValue();
		*counter = (Uint32)(value & 0x00000000FFFFFFFF);
		*newAddress = m_memSignalStrings;
		*newAddress += (Uint32)(value >> 32);		
		return value;
	}
#endif

#ifdef NEW_PROFILER_BREAKPOINTS_ENABLED
////////////////////////////////////////////////////////////////////////////////////////////////////
//
// start catch profiler breakpoints
void ProfilerManager::StartCatchBreakpoint()
{
	if( m_changingState.CompareExchange( true, false ) == false )
	{
		if ( m_started.GetValue() )
		{
			RED_LOG_WARNING( RED_LOG_CHANNEL(Profiler), TXT( "Catch breakpoint can not be started during profile session!!!" ) );
			m_changingState.SetValue( false );
			return;
		}
		if( m_catchBreakpointStarted.GetValue() )
		{
			RED_LOG_WARNING( RED_LOG_CHANNEL(Profiler), TXT( "Catch breakpoint already started!!!" ) );
			m_changingState.SetValue( false );
			return;
		}
		m_catchBreakpointStarted.SetValue( true );
		m_changingState.SetValue( false );
	}
	else
	{
		RED_LOG_WARNING( RED_LOG_CHANNEL(Profiler), TXT( "Another changing profiler state command processing!!!" ) );
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////
//
// stop catch profiler breakpoint
void ProfilerManager::StopCatchBreakpoint()
{
	if( m_changingState.CompareExchange( true, false ) == false )
	{
		if( !m_catchBreakpointStarted.GetValue() )
		{
			RED_LOG_WARNING( RED_LOG_CHANNEL(Profiler), TXT( "Catch breakpoint not started!!!" ) );
			m_changingState.SetValue( false );
			return;
		}
		m_catchBreakpointStarted.SetValue( false );
		m_changingState.SetValue( false );
	}
	else
	{
		RED_LOG_WARNING( RED_LOG_CHANNEL(Profiler), TXT( "Another changing profiler state command processing!!!" ) );
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////
//
// set time base profiler breakpoint
// timeInMs - time in MS, if execute time is longer then timeInMs breakpoint is hit
// stopOnce - if true breakpoint is hit only once
void ProfilerManager::SetTimeBreakpoint( const char* name, Float timeInMs, Bool stopOnce )
{
	if ( m_catchBreakpointStarted.GetValue() )
	{
		RED_LOG_WARNING( RED_LOG_CHANNEL(Profiler), TXT( "Before set time breakpoint stop catching breakpoint proccess!!!") );
		m_changingState.SetValue( false );
		return;
	}
	const Uint32 instrFuncCount = m_instrFuncsCounter.GetValue();
	Uint32 instrFuncIndex = 0;
	while ( instrFuncIndex < instrFuncCount)
	{
		if( strcmp( m_instrFuncs[instrFuncIndex]->m_name, name ) == 0 )
		{
			m_instrFuncs[instrFuncIndex]->m_breakpointAtExecTime = timeInMs;
			m_instrFuncs[instrFuncIndex]->m_breakOnce = stopOnce;
			m_instrFuncs[instrFuncIndex]->m_onceStopped.SetValue( false );
			return;
		}
		++instrFuncIndex;
	}
	RED_LOG_WARNING( RED_LOG_CHANNEL(Profiler), TXT( "%s is not registered!!!" ), ANSI_TO_UNICODE(name) );
}
////////////////////////////////////////////////////////////////////////////////////////////////////
//
// set hit count profiler breakpoint
// hitCount - count of hits
void ProfilerManager::SetHitCountBreakpoint( const char* name, Uint32 hitCount )
{
	if ( m_catchBreakpointStarted.GetValue() )
	{
		RED_LOG_WARNING( RED_LOG_CHANNEL(Profiler), TXT( "Before set hit count breakpoint stop catching breakpoint proccess!!!") );
		m_changingState.SetValue( false );
		return;
	}
	const Uint32 instrFuncCount = m_instrFuncsCounter.GetValue();
	Uint32 instrFuncIndex = 0;
	while ( instrFuncIndex < instrFuncCount)
	{
		if( strcmp( m_instrFuncs[instrFuncIndex]->m_name, name ) == 0 )
		{
			m_instrFuncs[instrFuncIndex]->m_breakpointAtHitCount.SetValue( hitCount );
			return;
		}
		++instrFuncIndex;
	}
	RED_LOG_WARNING( RED_LOG_CHANNEL(Profiler), TXT( "%s is not registered!!!" ), ANSI_TO_UNICODE(name) );
}
////////////////////////////////////////////////////////////////////////////////////////////////////
//
// remove time base profiler breakpoint
void ProfilerManager::DisableTimeBreakpoint( const char* name )
{
	if ( m_catchBreakpointStarted.GetValue() )
	{
		RED_LOG_WARNING( RED_LOG_CHANNEL(Profiler), TXT( "Before disable time breakpoint stop catching breakpoint proccess!!!") );
		m_changingState.SetValue( false );
		return;
	}
	SetTimeBreakpoint( name, 0.0f, false );
}
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//remove hit count profiler breakpoint
void ProfilerManager::DisableHitCountBreakpoint( const char* name )
{
	if ( m_catchBreakpointStarted.GetValue() )
	{
		RED_LOG_WARNING( RED_LOG_CHANNEL(Profiler), TXT( "Before disable hit count breakpoint stop catching breakpoint proccess!!!") );
		m_changingState.SetValue( false );
		return;
	}
	SetHitCountBreakpoint( name, 0 );
}

#endif
} // namespace Profiler
//////////////////////////////////////////////////////////////////////////
// EOF
