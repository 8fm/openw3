#ifndef _INSTRUMENTED_FUNCTION_H_
#define _INSTRUMENTED_FUNCTION_H_

//////////////////////////////////////////////////////////////////////////

namespace NewRedProfiler
{

    //////////////////////////////////////////////////////////////////////////

    struct InstrumentedFunction
    {
		InstrumentedFunction() : 
			m_name( nullptr ),
			m_id( 0 ),
			m_enabled( true )
#ifdef NEW_PROFILER_BREAKPOINTS_ENABLED
		   ,m_breakpointAtExecTime( 0.0f ),
			m_breakpointAtHitCount( 0 ),
			m_breakOnce( false ),
			m_onceStopped( false )
#endif
        {
        }

		InstrumentedFunction( const InstrumentedFunction& instrumentedFunction ) : 
			m_name( instrumentedFunction.m_name ),
			m_id( instrumentedFunction.m_id ),
			m_enabled( instrumentedFunction.m_enabled.GetValue() )
#ifdef NEW_PROFILER_BREAKPOINTS_ENABLED
			,m_breakpointAtExecTime( instrumentedFunction.m_breakpointAtExecTime ),
			m_breakpointAtHitCount( instrumentedFunction.m_breakpointAtHitCount.GetValue() ),
			m_breakOnce( instrumentedFunction.m_breakOnce),
			m_onceStopped( instrumentedFunction.m_onceStopped.GetValue() )
#endif
		{		
		}

        ~InstrumentedFunction()
        {            
            m_name = nullptr;
        }		

        char* m_name;
		Red::Threads::CAtomic<Bool> m_enabled;
        Uint32 m_id;
#ifdef NEW_PROFILER_BREAKPOINTS_ENABLED
		Float  m_breakpointAtExecTime;
		Red::Threads::CAtomic<Uint32> m_breakpointAtHitCount;
		Bool   m_breakOnce;
		Red::Threads::CAtomic<Bool>   m_onceStopped;
#endif
    };

    //////////////////////////////////////////////////////////////////////////

}

//////////////////////////////////////////////////////////////////////////

#endif

