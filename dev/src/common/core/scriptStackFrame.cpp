/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "scriptStackFrame.h"
#include "scriptingSystem.h"

#include "../redSystem/crt.h"

void* CScriptStackFrame::s_offset = NULL;
#ifndef NO_SCRIPT_DEBUG
String CScriptStackFrame::s_tempPropertyValueBuffer;
#endif

CScriptStackFrame::CScriptStackFrame( CScriptStackFrame* parentFrame, IScriptable *context, const CFunction *function, void *locals, void *params )
	: m_context( context )
	, m_rawContext( context )
	, m_function( function )
	, m_locals( (Uint8*) locals )
	, m_params( (Uint8*) params )
	, m_parent( parentFrame )
	, m_line( 0 )
	, m_debugFlags( 0 )
	, m_thread( NULL )
	, m_numOutputParams( 0 )
	, m_parentResult( NULL )
	, m_perfData( NULL )
{
	// Get function code
	m_code = function->GetCode().GetCode();

#ifndef NO_SCRIPT_DEBUG
	// This is a crappy hack for debug info to ensure we don't reallocate during an assert/error
	if( s_tempPropertyValueBuffer.Capacity() < 512 )
	{
		s_tempPropertyValueBuffer.Reserve( 512 );
	}
#endif
}

#if defined( RED_PLATFORM_WINPC ) && !defined( RED_FINAL_BUILD )
void CScriptStackFrame::Step( IScriptable *context, void* result )
{
#ifndef NO_SCRIPT_DEBUG
	GScriptStackFrameArray[ GScriptStackFrameArraySize ] = this;
	++GScriptStackFrameArraySize;
	RED_FATAL_ASSERT( GScriptStackFrameArraySize < SCRIPTS_STACK_FRAME_ARRAY_MAX_SIZE, "Script Stack overflow!" );
#endif

	ASSERT( ::SIsMainThread() );

	// Read native function index
	Uint8 functionIndex = *m_code++;

	// Get the function
	TNativeGlobalFunc function = CScriptNativeFunctionMap::GetGlobalNativeFunction( functionIndex );

	RED_FATAL_ASSERT( function != nullptr,
		"Calling null native function pointer at functionIndex '%u'. Possible GET_PARAMETER to script import mismatch or called GET_PARAMETER after FINISH_PARAMETERS?",
		static_cast<Uint32>( functionIndex ) );

	// Execute code
	( *function )( context, *this, result );

#ifndef NO_SCRIPT_DEBUG
	// Decrement first
	
	RED_FATAL_ASSERT( GScriptStackFrameArraySize > 0, "Script Stack overflow!" );
	--GScriptStackFrameArraySize;
	GScriptStackFrameArray[ GScriptStackFrameArraySize ] = NULL;
#endif
}
#endif

const CFunction* CScriptStackFrame::GetEntryFunction() const
{
	const CScriptStackFrame* frame = this;

	do
	{
		if( frame->m_function->IsEntry() )
		{
			return frame->m_function;				
		};

		frame = frame->m_parent;
	}
	while ( frame );

	return NULL;
}

#ifndef NO_SCRIPT_DEBUG

CScriptStackFrame* CScriptStackFrame::GScriptStackFrameArray[ CScriptStackFrame::SCRIPTS_STACK_FRAME_ARRAY_MAX_SIZE ] = { NULL };
Uint32 CScriptStackFrame::GScriptStackFrameArraySize = 0;

Uint32 CScriptStackFrame::DumpRawTopToAnsiString( AnsiChar* str, Uint32 strSize )
{
	size_t initialLength = Red::System::StringLength( str, strSize );

	IScriptable* contextHandle = m_context.Get();
	if ( contextHandle && contextHandle->GetClass() && !m_function->IsStatic() )
	{
		Red::System::StringConcatenate( str, contextHandle->GetClass()->GetName().AsAnsiChar(), strSize );
		Red::System::StringConcatenate( str, "::", strSize );
	}

	Red::System::StringConcatenate( str, m_function->GetName().AsAnsiChar(), strSize );

	AnsiChar lineId[ 16 ];
	Red::System::SNPrintF( lineId, ARRAY_COUNT( lineId ), "(...) @%i\n", m_line );
	Red::System::StringConcatenate( str, lineId, strSize );

	return static_cast< Uint32 >( Red::System::StringLength( str, strSize ) - initialLength );
}

size_t CScriptStackFrame::DumpTopToString( Char* str, Uint32 strSize )
{
	size_t initialLength = Red::System::StringLength( str, strSize );

	IScriptable* contextHandle = m_context.Get();
	if ( contextHandle && contextHandle->GetClass() && !m_function->IsStatic() )
	{
		Red::System::StringConcatenate( str, contextHandle->GetClass()->GetName().AsString().AsChar(), strSize );
		Red::System::StringConcatenate( str, TXT( "::" ), strSize );
	}

	Red::System::StringConcatenate( str, m_function->GetName().AsString().AsChar(), strSize );
	Red::System::StringConcatenate( str, TXT( "( " ), strSize );

	for ( Uint32 i = 0; i < m_function->GetNumParameters(); ++i )
	{
		CProperty* param = m_function->GetParameter( i );
		const void* value = param->GetOffsetPtr( m_params );

		// Get value
		if ( param->GetType() && param->GetType()->ToString( value, s_tempPropertyValueBuffer ) )
		{
			Red::System::StringConcatenate( str, s_tempPropertyValueBuffer.AsChar(), strSize );
		}
		else
		{
			Red::System::StringConcatenate( str, TXT( "?" ), strSize );
		}

		if( i != m_function->GetNumParameters() - 1 )
		{
			Red::System::StringConcatenate( str, TXT( ", " ), strSize );
		}
		else
		{
			Red::System::StringConcatenate( str, TXT( " " ), strSize );
		}
	}

	if ( m_function )
	{
		Red::System::StringConcatenate( str, TXT( " ) - " ), strSize );
		Red::System::StringConcatenate( str, m_function->GetCode().GetSourceFile().AsChar(), strSize );
		Red::System::StringConcatenate( str, TXT( "(" ), strSize );
	}

	Char lineId[ 16 ];
	Red::System::SNPrintF( lineId, ARRAY_COUNT( lineId ), TXT( "%i" ), m_line );

	Red::System::StringConcatenate( str, lineId, strSize );
	Red::System::StringConcatenate( str, TXT( ")" ), strSize );

	if ( contextHandle && m_function && !m_function->IsStatic() )
	{
		Red::System::StringConcatenate( str, TXT( ", object: " ), strSize );
		Red::System::StringConcatenate( str, contextHandle->GetFriendlyName().AsChar(), strSize );
	}

	return Red::System::StringLength( str, strSize ) - initialLength;
}

void CScriptStackFrame::DumpToLog()
{
	Char top[ 512 ];
	Red::System::MemorySet( top, 0, sizeof( top ) );

	DumpTopToString( top, ARRAY_COUNT( top ) );

	RED_LOG( RED_LOG_CHANNEL( Script ), TXT( "   %" ) RED_PRIWs, top );

	if ( m_parent )
	{
		m_parent->DumpToLog();
	}
}

Uint32 CScriptStackFrame::DumpToString( Char* str, Uint32 strSize )
{
	size_t bufferUsed = 0;

	Red::System::StringConcatenate( str, TXT( "     " ), strSize );

	bufferUsed += DumpTopToString( str, strSize );

	Red::System::StringConcatenate( str, TXT( "\n" ), strSize );

	if ( m_parent )
	{
		bufferUsed += m_parent->DumpToString( str, strSize );
	}

	return static_cast< Uint32 >( bufferUsed );
}

#define NOT_IN_MAIN_THREAD_STR		TXT( "     NOT IN MAIN THREAD\n" )
#define OVERFLOW_STR				TXT( "     DumpScriptStack error: GScriptStackFrameArray overflow, infinite recursion probably occured!\n" )
#define NULLFRAME_STR				TXT( "     DumpScriptStack error: NULL frame\n" )
#define EMPTY_STR					TXT( "     EMPTY\n" )

Uint32 CScriptStackFrame::GetScriptStackRaw( AnsiChar* buffer, Uint32 bufferSize )
{
	size_t bufferUsed = 0;

	if( !SIsMainThread() )
	{
		return 0;
	}
	else if( GScriptStackFrameArraySize > 0 && GScriptStackFrameArraySize < SCRIPTS_STACK_FRAME_ARRAY_MAX_SIZE )
	{
		CScriptStackFrame* frame = GScriptStackFrameArray[ GScriptStackFrameArraySize - 1 ];
		if( frame )
		{			
			bufferUsed += frame->DumpRawToAnsiString( buffer, bufferSize );
		}
	}

	return static_cast< Uint32 >( bufferUsed );
}

Uint32 CScriptStackFrame::DumpRawToAnsiString( AnsiChar* stf, Uint32 strSize )
{
	Uint32 charsWritten = DumpRawTopToAnsiString( stf, strSize );
	if( m_parent )
	{
		charsWritten += m_parent->DumpRawToAnsiString( stf, strSize );
	}

	return charsWritten;
}

Uint32 CScriptStackFrame::DumpScriptStack( Char* str, Uint32 strSize )
{
	size_t bufferUsed = 0;

	if( !SIsMainThread() )
	{
		Red::System::StringCopy( str, NOT_IN_MAIN_THREAD_STR, strSize );
		bufferUsed += Red::System::StringLengthCompileTime( NOT_IN_MAIN_THREAD_STR );
	}
	else
	{
		if( GScriptStackFrameArraySize > 0 )
		{
			if( GScriptStackFrameArraySize > SCRIPTS_STACK_FRAME_ARRAY_MAX_SIZE )
			{
				Red::System::StringConcatenate( str, OVERFLOW_STR, strSize );
				bufferUsed += Red::System::StringLengthCompileTime( OVERFLOW_STR );
			}
			else
			{
				CScriptStackFrame* frame = GScriptStackFrameArray[ GScriptStackFrameArraySize - 1 ];
				if( frame )
				{			
					bufferUsed += frame->DumpToString( str, strSize );
				}
				else
				{
					Red::System::StringConcatenate( str, NULLFRAME_STR, strSize );
					bufferUsed += Red::System::StringLengthCompileTime( NULLFRAME_STR );
				}
			}
		}
		else
		{
			Red::System::StringConcatenate( str, EMPTY_STR, strSize );
			bufferUsed += Red::System::StringLengthCompileTime( EMPTY_STR );
		}
	}

	return static_cast< Uint32 >( bufferUsed );
}

#endif // NO_SCRIPT_DEBUG
