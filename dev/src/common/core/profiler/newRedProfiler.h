#ifndef _CDP_R_PROFILER_H_
#define _CDP_R_PROFILER_H_

#define PROFILER_MAX_PROFILES_COUNT 65535

#define NEW_PROFILER_BREAKPOINTS_ENABLED

//////////////////////////////////////////////////////////////////////////
// headers
#include "instrumentedFunction.h"
#include "../dynarray.h"
#include "../hashmap.h"
#include "../string.h"


//////////////////////////////////////////////////////////////////////////
// pre-declarations
class CMemoryFileWriter;


//////////////////////////////////////////////////////////////////////////
// declarations
namespace NewRedProfiler
{

    class ProfilerManager
    {
    public:
        ProfilerManager();
        ~ProfilerManager();

        // common
        void Init( const Uint32 mem, const Uint32 memSignalStrings );
		const String& GetLastOutputPath() const { return m_outputPath; }

		// Accessors
		Bool IsStarted();
        Float GetFilledBufferFactor();
        Uint32 GetStoredFrames();
		Uint32 GetStoredBlocks();
		Uint32 GetStoredSignals();
		Double GetProfilingTime();
		Double GetInternalTime();
		Uint32 GetRegisteredFunctionsCount();

        // profiling
#ifdef NEW_PROFILER_BREAKPOINTS_ENABLED
		Uint64 StartBlock( InstrumentedFunction* instrFunc );
		void EndBlock( InstrumentedFunction* instrFunc, Uint64 startTime );
#else
        void StartBlock( InstrumentedFunction* instrFunc );
        void EndBlock( InstrumentedFunction* instrFunc );
#endif
        void Signal( InstrumentedFunction* instrFunc );
        void Signal( InstrumentedFunction* instrFunc, const Float value );
        void Signal( InstrumentedFunction* instrFunc, const Int32 value );
		//! not supported on 32 bit
        void Signal( InstrumentedFunction* instrFunc, const Char* name);

        void NextFrame();

        // control
        void Start();
        void Stop( Bool skipChangingState = false );
        Bool Store( const String& filename, const String& sessionId = String::EMPTY, Uint64 sessionQpc = 0, Uint64 sessionQpf = 0 );
		Bool StoreToMem( CMemoryFileWriter* buffer );
		
		// instrumented functions
        InstrumentedFunction* RegisterInstrFunc( const char* name, Bool enable = true );
		void EnableInstrFunc( InstrumentedFunction* instrFunc, Bool enable );
		void EnableInstrFunc( Uint32 instrumentedFunctionId, Bool enable );
	
		//! fill instrFuncArray with register instrumented functions names
		void GetInstrFuncArray( TDynArray<InstrumentedFunction>& instrFuncArray );
		//! store register instrumented functions names to instrFuncList.txt
		Bool StoreInstrFuncList();

#ifdef NEW_PROFILER_BREAKPOINTS_ENABLED
		void StartCatchBreakpoint();
		void StopCatchBreakpoint();
		void SetTimeBreakpoint( const char* name, Float timeInMs, Bool stopOnce );
		void SetHitCountBreakpoint( const char* name, Uint32 hitCount );
		void DisableTimeBreakpoint( const char* name );
		void DisableHitCountBreakpoint( const char* name );
#endif
    private:
		
		//! load list of disabled InstrFunc from instrFuncDisabled.txt
		Bool LoadInstrFuncEnabledFile(); 
		//! disable registered instrumented function listed in instrFuncDisabled.txt
		void DisableIntrFuncs();

		//! return free block of memory if present	
		RED_FORCE_INLINE Uint64* GetBufferBlock64Bits( const Uint32 num64bits ); 

#ifdef RED_ARCH_X64 // internal implementation use ATOMIC 64 bit
		//! set atomic properties for Signal Strings
		RED_FORCE_INLINE void SetCurrentPosSignalStrings( const Uint8* newAddress, const Uint32 counter );
		//! get atomic properties for Signal Strings
		RED_FORCE_INLINE Uint64 GetCurrentPosSignalStrings( Uint8** newAddress, Uint32* counter );
#endif //RED_ARCH_X64

	private:
		Red::System::Timer m_timer;

		TDynArray<String> m_outputDirs; //! output directories loaded from config file
		String m_outputPath; //! last output path

        //! overhead info variables
#ifdef RED_ARCH_X64
		Red::Threads::CAtomic<Int64> m_internalTicks;
#endif
		Uint64 m_startTicks;
		Uint64 m_stopTicks;

		//! control flags
		Red::Threads::CAtomic<Bool> m_changingState;
		Red::Threads::CAtomic<Bool> m_init;
        Red::Threads::CAtomic<Bool> m_started;
        Red::Threads::CAtomic<Bool> m_EOM;
		Red::Threads::CAtomic<Bool> m_EOM_String;
		Bool						m_stored; //! is current buffer was stored
		Red::Threads::CAtomic<Bool>	m_instrFuncConfigLoaded;
#ifdef NEW_PROFILER_BREAKPOINTS_ENABLED
		Red::Threads::CAtomic<Bool> m_catchBreakpointStarted;
#endif
		//! blocks data
        Uint32							m_reservedMem;
		Red::Threads::CAtomic<Uint64*>	m_currentPos;
		Uint64*							m_endStopFramePos;
		Uint64*							m_mem;
		Red::Threads::CAtomic<Uint32>	m_storedFrames;
		Red::Threads::CAtomic<Uint32>	m_storedBlocks;
		Red::Threads::CAtomic<Uint32>	m_storedSignals;
				
#ifdef RED_ARCH_X64
		// strings data
		Uint32  m_reservedMemSignalString;
		Uint8*  m_memSignalStrings;
		Red::Threads::CAtomic<Uint64> m_currPosSignalStrings; //! store offset and signals count		      
#endif

		NewRedProfiler::InstrumentedFunction   *m_instrFuncs[PROFILER_MAX_PROFILES_COUNT];
		Red::Threads::CAtomic<Uint32>			m_instrFuncsCounter;
		THashMap<StringAnsi, Bool>				m_instrFuncDisabled;
		
	};
} // namespace

#ifdef NEW_PROFILER_ENABLED

//////////////////////////////////////////////////////////////////////////
// Singleton
typedef TSingleton<NewRedProfiler::ProfilerManager> SProfilerManager;


//////////////////////////////////////////////////////////////////////////
// macros

#define PROFILER_DEFINE( INSTRFUNC ) NewRedProfiler::InstrumentedFunction* __instrFunc__##INSTRFUNC=SProfilerManager::GetInstance().RegisterInstrFunc( #INSTRFUNC )
#define PROFILER_DECLARE( INSTRFUNC ) NewRedProfiler::InstrumentedFunction* __instrFunc__##INSTRFUNC
#define PROFILER_DECLARE_Extern( INSTRFUNC ) extern NewRedProfiler::InstrumentedFunction* __instrFunc__##INSTRFUNC

#define PROFILER_HANDLE_NAME( INSTRFUNC ) ptrHandle##INSTRFUNC

#define PROFILER_Init( MEMORY ) SProfilerManager::GetInstance().Init( MEMORY, MEMORY / 10 )
#define PROFILER_InitEx( MEMORY, MEMORY_SIGNALS ) SProfilerManager::GetInstance().Init( MEMORY, MEMORY_SIGNALS )
#define PROFILER_IsRecording() SProfilerManager::GetInstance().IsStarted()
#define PROFILER_Start() SProfilerManager::GetInstance().Start()
#define PROFILER_Stop() SProfilerManager::GetInstance().Stop()
#define PROFILER_Store( PATH , SESSION_ID, SESSION_QPC, SESSION_QPF ) SProfilerManager::GetInstance().Store( PATH, SESSION_ID, SESSION_QPC, SESSION_QPF )
#define PROFILER_StoreToMem( BUFFER ) SProfilerManager::GetInstance().StoreToMem( (BUFFER) )
#define PROFILER_GetLastStorePath() SProfilerManager::GetInstance().GetLastOutputPath()
#define PROFILER_GetBufferUsage() SProfilerManager::GetInstance().GetFilledBufferFactor()
#define PROFILER_StoreInstrFuncList() SProfilerManager::GetInstance().StoreInstrFuncList()
#define PROFILER_Signal( INSTRFUNC ) SProfilerManager::GetInstance().Signal( __instrFunc__##INSTRFUNC )
#define PROFILER_SignalValue( INSTRFUNC, VALUE ) SProfilerManager::GetInstance().Signal( __instrFunc__##INSTRFUNC, VALUE )
#define PROFILER_NextFrame() SProfilerManager::GetInstance().NextFrame()

#ifdef NEW_PROFILER_BREAKPOINTS_ENABLED

	#define PROFILER_StartBlock( INSTRFUNC ) SProfilerManager::GetInstance().StartBlock( __instrFunc__##INSTRFUNC )
	#define PROFILER_EndBlock( INSTRFUNC ) SProfilerManager::GetInstance().EndBlock( __instrFunc__##INSTRFUNC, 0 )

	#define PROFILER_StartBlockWithBreak( INSTRFUNC ) SProfilerManager::GetInstance().StartBlock( __instrFunc__##INSTRFUNC )
	#define PROFILER_EndBlockWithBreak( INSTRFUNC, START_TIME ) SProfilerManager::GetInstance().EndBlock( __instrFunc__##INSTRFUNC, START_TIME )

	#define PROFILER_StartCatchBreakpoint() SProfilerManager::GetInstance().StartCatchBreakpoint()
	#define PROFILER_StopCatchBreakpoint() SProfilerManager::GetInstance().StopCatchBreakpoint()
	#define PROFILER_SetTimeBreakpoint( NAME, TIME, STOP_ONCE ) SProfilerManager::GetInstance().SetTimeBreakpoint( NAME, TIME, STOP_ONCE )
	#define PROFILER_SetHitCountBreakpoint( NAME, COUNTER ) SProfilerManager::GetInstance().SetHitCountBreakpoint( NAME, COUNTER )
	#define PROFILER_DisableTimeBreakpoint( NAME ) SProfilerManager::GetInstance().DisableTimeBreakpoint( NAME )
	#define PROFILER_DisableHitCountBreakpoint( NAME ) SProfilerManager::GetInstance().DisableHitCountBreakpoint( NAME )

#else

	#define PROFILER_StartBlock( INSTRFUNC ) SProfilerManager::GetInstance().StartBlock( __instrFunc__##INSTRFUNC )
	#define PROFILER_EndBlock( INSTRFUNC ) SProfilerManager::GetInstance().EndBlock( __instrFunc__##INSTRFUNC )

	#define PROFILER_StartBlockWithBreak( INSTRFUNC ) 0
	#define PROFILER_EndBlockWithBreak( INSTRFUNC, START_TIME )

	#define PROFILER_StartCatchBreakpoint()
	#define PROFILER_StopCatchBreakpoint()
	#define PROFILER_SetTimeBreakpoint( NAME, TIME, STOP_ONCE )
	#define PROFILER_SetHitCountBreakpoint( NAME, COUNTER )
	#define PROFILER_DisableTimeBreakpoint( NAME )
	#define PROFILER_DisableHitCountBreakpoint( NAME )

#endif //NEW_PROFILER_BREAKPOINTS_ENABLED
//////////////////////////////////////////////////////////////////////////

#else

#define PROFILER_DEFINE( INSTRFUNC )
#define PROFILER_DECLARE( INSTRFUNC )
#define PROFILER_DECLARE_Extern( INSTRFUNC )

#define PROFILER_HANDLE_NAME( INSTRFUNC ) 

#define PROFILER_Init( MEMORY )
#define PROFILER_InitEx( MEMORY, MEMORY_SIGNALS )
#define PROFILER_Start()
#define PROFILER_IsRecording() false
#define PROFILER_Stop()
#define PROFILER_Store( PATH, SESSION_ID, SESSION_QPC, SESSION_QPF )
#define PROFILER_StoreToMem( BUFFER )
#define PROFILER_GetLastStorePath() TXT("")
#define PROFILER_GetBufferUsage() 0.0f
#define PROFILER_StoreInstrFuncList()

#define PROFILER_StartBlock( INSTRFUNC )
#define PROFILER_EndBlock( INSTRFUNC )
#define PROFILER_StartBlockWithBreak( INSTRFUNC ) 0
#define PROFILER_EndBlockWithBreak( INSTRFUNC, START_TIME )
#define PROFILER_Signal( INSTRFUNC )
#define PROFILER_SignalValue( INSTRFUNC, VALUE )
#define PROFILER_NextFrame()

#define PROFILER_StartCatchBreakpoint()
#define PROFILER_StopCatchBreakpoint()
#define PROFILER_SetTimeBreakpoint( NAME, TIME, STOP_ONCE )
#define PROFILER_SetHitCountBreakpoint( NAME, COUNTER )
#define PROFILER_DisableTimeBreakpoint( NAME )
#define PROFILER_DisableHitCountBreakpoint( NAME )

#endif //NEW_PROFILER_ENABLED
#endif //_CDP_R_PROFILER_H_
