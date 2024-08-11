/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "scriptThread.h"
#include "scriptingSystem.h"
#include "scriptThreadSerializer.h"
#include "scriptStackFrame.h"

IMPLEMENT_RTTI_CLASS( CScriptThread );

Uint32 CScriptThread::s_latentIndex = 0;

CScriptThread::CScriptThread( Uint32 id, IScriptable* context, const CFunction*, CScriptStackFrame& topFrame, void * returnValue )
	: m_isKilled( false )
	, m_isYielding( false )
	, m_lastYield( NULL )
	, m_context( context )
	, m_contextUsed( context != NULL )
	, m_timer( 0.0f )
	, m_latentIndex( 0 )
	, m_id( id )
	, m_returnValue( returnValue )
	, m_listener( NULL )
	, m_totalTicks( 0 )
	, m_totalTime( 0.0f )
	, m_serializer( NULL )
	, m_contextName( TXT("Unknown") )
{
	// Get name of context
	if ( context )
	{
		m_contextName = context->GetFriendlyName();
	}

	// Add initial frame to the list
	m_frames.PushBack( &topFrame );
	topFrame.m_thread = this;
}

CScriptThread::~CScriptThread()
{
	// Purge frames
	ForceKill();

	// Delete pending list
	DeletePendingFrames();

	// dispose of the serializer, if one is set
	if( m_serializer )
	{
		delete m_serializer;
		m_serializer = NULL;
	}
}

void CScriptThread::DeletePendingFrames()
{
	// Delete frames
	for ( Uint32 i=0; i<m_deletedFrames.Size(); i++ )
	{
		CScriptStackFrame* frame = m_deletedFrames[i];
		frame->m_function->DestroyEntryFrame( frame );
	}

	// Clear list
	m_deletedFrames.Clear();
}

void CScriptThread::ForceKill()
{
	if ( !m_isKilled )
	{
		// Kill thread
		m_isKilled = true;
		m_context = NULL;

		// Pass to listener
		if ( m_listener )
		{
			m_listener->OnScriptThreadKilled( this, false );
		}

		// Destroy frames
		for ( Uint32 i=0; i<m_frames.Size(); i++ )
		{
			CScriptStackFrame* frame = m_frames[i];

			// Reset debug
			frame->m_debugFlags = 0;
			ASSERT( frame->m_thread == this );

			// Delete in some later time
			m_deletedFrames.PushBack( frame );
		}

		// Clear frame list
		m_frames.Clear();
	}
}

void CScriptThread::ForceYield()
{
	if ( !m_isKilled )
	{
		m_isYielding = true;
	}
}

void CScriptThread::SetListener( IListener* listener )
{
	m_listener = listener;

	if( m_listener )
	{
		RED_LOG( RED_LOG_CHANNEL( ScriptThread ), TXT( "Listener for thread %u set to: %" ) RED_PRIWs, m_id, m_listener->GetDebugName().AsChar() );
	}
	else
	{
		RED_LOG( RED_LOG_CHANNEL( ScriptThread ), TXT( "Listener for thread %u cleared" ), m_id );
	}
}

Bool CScriptThread::CompareListener( IListener* listener ) const
{
	return listener == m_listener;
}

Uint32 CScriptThread::GenerateLatentIndex()
{
	m_latentIndex = s_latentIndex++;
	return m_latentIndex;
}

// TODO: rethink where to place it
Bool GLatentFunctionStart = false;

struct TimeAccumulator
{
	Float*		m_counter;
	Uint32*		m_ticks;
	Double		m_startTime;
	Uint32		m_startTicks;

	TimeAccumulator( Float* counter, Uint32* ticks )
		: m_counter( counter )
		, m_ticks( ticks )
	{
		m_startTicks = *m_ticks;
		m_startTime = Red::System::Clock::GetInstance().GetTimer().GetSeconds();
	}

	~TimeAccumulator()
	{
		if ( *m_ticks > m_startTicks )
		{
			Double tickDelta = Red::System::Clock::GetInstance().GetTimer().GetSeconds() - m_startTime;
			(*m_counter) += ( Float )tickDelta;
		}
	}
};

Bool CScriptThread::Advance( Float timeDelta )
{
#ifndef NO_DEBUG_PAGES
	TimeAccumulator acc( &m_totalTime, &m_totalTicks );
#endif
	// Update timer
	m_timer += timeDelta;

	// Context lost
	if ( m_contextUsed && !m_context.Get() )
	{
		ForceKill();
	}

	// Killed, no more to do
	while ( !m_isKilled && !m_isYielding )
	{
		// Get the top function to process
		CScriptStackFrame* topFrame = m_frames.Back();
		ASSERT( topFrame && topFrame->m_thread == this );

#ifndef NO_PERFCOUNTERS
		FunctionCallProfiler profiler( ( CFunction* ) topFrame->m_function, NULL );
#endif
		// Execute the code from the function		
		const Uint8* codeEnd = topFrame->m_function->GetCode().GetCodeEnd();
		while ( topFrame->m_code < codeEnd )
		{
			// Remember the starting position in case we have yielded
			const Uint8* codePos = topFrame->m_code;
			GLatentFunctionStart = codePos != m_lastYield;

			// New function, reset timer
			if ( GLatentFunctionStart )
			{
				m_timer = 0.0f;
			}

			// Context handle got invalidated
			if ( m_contextUsed && !topFrame->GetContext() )
			{
				{
					HALT( "Object context of active thread stack frame got invalidated. Context object was: '%ls'.", m_contextName.AsChar() );
					DeletePendingFrames();
					return true;
				}
			}
			
			// Call opcode
			const Bool isReturn = *codePos == OP_Return;
			if ( isReturn )
			{
				// Grab return value
				void* result = topFrame->m_parentResult;
				if ( m_frames.Size() == 1 && m_returnValue )
				{
					result = m_returnValue;
				}

				// Step the stack to calculate result
				topFrame->Step( topFrame->GetContext(), result );
			}
			else
			{
				// Just execute
				topFrame->Step( topFrame->GetContext(), NULL );
			}

			// Thread was killed during the process, exit now
			if ( m_isKilled )
			{
				DeletePendingFrames();
				return true;
			}

			// Thread was yielded during the process
			if ( m_isYielding )
			{
				// Rewind code back so we retry in next frame, it's enough for current tick
				topFrame->m_code = codePos;
				m_lastYield = codePos;
				break;
			}

			// Count ticks
			m_totalTicks++;

			// No yield, reset
			m_lastYield = NULL;

			// We have returned from function
			if ( isReturn )
			{
				// Rewind to the end of the code
				topFrame->m_code = codeEnd;
				break;
			}

			// If new frame has been pushed on stack, process the new function
			if ( topFrame != m_frames[ m_frames.Size()-1 ] )
			{
				topFrame = m_frames[ m_frames.Size()-1 ];
				ASSERT( topFrame && topFrame->m_thread == this );
				break;
			}
		}

		// End of code reached, pop the frame
		if ( topFrame->m_code == codeEnd )
		{
			// Pop and destroy the top frame
			m_frames.Remove( topFrame );

			// Copy output parameters to parent
			if ( !m_frames.Empty() )
			{
				CScriptStackFrame * parentFrame = m_frames.Back();
				ASSERT( parentFrame );
				for ( Uint32 i=0; i < parentFrame->m_numOutputParams; ++i )
				{
					CScriptStackFrame::OutputParam &param = parentFrame->m_outputParams[i];
					param.m_type->Copy( param.m_dataOffset, param.m_localOffset );
				}
				parentFrame->m_numOutputParams = 0;
			}

			// Destroy child frame
			topFrame->m_function->DestroyEntryFrame( topFrame );

			// There are no more frames, exit thread
			if ( m_frames.Empty() )
			{
				m_isKilled = true;

				// Pass to listener
				if ( m_listener )
				{
					m_listener->OnScriptThreadKilled( this, true );
				}
				else
				{
					// advance kill with no listener
					//LOG_CORE(TXT("advance kill with no listener"));
					HALT(  "advance kill with no listener" );
				}
			}
		}
	}

	// Reset yield flag
	m_isYielding = false;

	// Return true if thread was killed
	return m_isKilled;
}

void CScriptThread::PushFrame( CScriptStackFrame *frame )
{
	ASSERT( frame );
	frame->m_thread = this;
	m_frames.PushBack( frame );
}

void CScriptThread::EnableSerialization( bool enable )
{
	if ( enable )
	{
		if ( !m_serializer )
		{
			m_serializer = new CScriptThreadSerializer();
		}
	}
	else
	{
		delete m_serializer;
		m_serializer = NULL;
	}
}

CScriptThreadSerializer* CScriptThread::QuerySerializer()
{
	return m_serializer;
}

String CScriptThread::GetListenerName() const
{
	return m_listener ? m_listener->GetDebugName() : String( TXT("<none>") );
}
