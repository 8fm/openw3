/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "scriptingSystem.h"
#include "scriptCodeGenerator.h"
#include "fileSys.h"

CScriptCompiledCode::CScriptCompiledCode()
	: m_code( TDataBufferAllocator< MC_BufferScriptCode >::GetInstance() )
	, m_sourceLine( 0 )
{
}

void CScriptCompiledCode::Initialize( const CScriptCodeGenerator& generator )
{
	// Location information
	m_sourceFile = generator.GetSourceFile();
	m_sourceLine = generator.GetSourceLine();

	// Strip mod context
	size_t contextClosePosition = 0;
	if( m_sourceFile.FindCharacter( TXT( ']' ), contextClosePosition ) )
	{
		 String contextStripped = m_sourceFile.MidString( contextClosePosition + 1 );

		 if( contextStripped.GetLength() > 0 )
		 {
			 m_modContext = m_sourceFile.MidString( 1, contextClosePosition - 1 );
			 m_sourceFile = std::move( contextStripped );
		 }
	}

	String modStrip = TXT( "local" ) MACRO_TXT( DIRECTORY_SEPARATOR_LITERAL_STRING );
	if( m_sourceFile.BeginsWith( modStrip ) )
	{
		String contextStripped = m_sourceFile.MidString( modStrip.GetLength() );

		if( contextStripped.GetLength() > 0 )
		{
			m_sourceFile = std::move( contextStripped );
		}
	}

	modStrip = TXT( "source" ) MACRO_TXT( DIRECTORY_SEPARATOR_LITERAL_STRING );
	if( m_sourceFile.BeginsWith( modStrip ) )
	{
		String contextStripped = m_sourceFile.MidString( modStrip.GetLength() );

		if( contextStripped.GetLength() > 0 )
		{
			m_sourceFile = std::move( contextStripped );
		}
	}

	ASSERT( !m_sourceFile.Empty() );
	ASSERT( m_sourceLine != 0 );

	// Initialize source code
	const CScriptCodeGenerator::BufferU8& code = generator.GetCode();
	DataBuffer buffer( TDataBufferAllocator< MC_BufferScriptCode >::GetInstance(), code.Size(), code.Data() );
	m_code.MoveHandle( buffer );

	// Copy breakpoint information
	const CScriptCodeGenerator::BufferU32& breakpoints = generator.GetBreakpoints();
	m_breakpoints.Resize( breakpoints.Size() );
	Red::System::MemoryCopy( m_breakpoints.TypedData(), breakpoints.TypedData(), breakpoints.Size() * sizeof( Uint32 ) );

	// Generate savepoints information
	const CScriptCodeGenerator::BufferU32& savepointOffsets = generator.GetSavepoints();
	m_savepoints.Reserve( savepointOffsets.Size() );
	for( Uint32 i = 0; i < savepointOffsets.Size(); ++i )
	{
		m_savepoints.PushBack( std::move( ScriptSavepoint( savepointOffsets[ i ] ) ) );
	}
}

void CScriptCompiledCode::CreateBuffer( Uint32 size )
{
	RED_FATAL_ASSERT( m_code.GetSize() == 0, "Script code buffer has already been allocated" );
	m_code.Allocate( size );
}

void CScriptCompiledCode::BreakpointRemoveAll()
{
	// Remove the breakpoint flag
	for ( Uint32 i = 0; i < m_breakpoints.Size(); ++i )
	{
		// Get the breakpoint header
		Uint8* code = (Uint8*)m_code.GetData() + m_breakpoints[i];
		ASSERT( *code == OP_Breakpoint );
		code++;

		// Skip the line number
		code += sizeof( Uint32 );

		// Zero the status
		*code = 0;
	}
}

Bool CScriptCompiledCode::BreakpointToggle( const ScriptBreakpoint& breakpoint, Bool status )
{
	Bool found = false;

	// Breakpoint should be in the same file
	
	const Uint32 numBreakpoints = m_breakpoints.Size();

	if( numBreakpoints > 0 )
	{
		if ( breakpoint.m_line >= m_sourceLine && m_sourceFile.EqualsNC( breakpoint.m_file ) )
		{
			// Find the breakpoint with matching line
			for ( Uint32 i = 0; i < numBreakpoints; ++i )
			{
				// Get the breakpoint header
				Uint8* code = (Uint8*)m_code.GetData() + m_breakpoints[i];
				RED_FATAL_ASSERT( *code == OP_Breakpoint, "Compiled script code is invalid" );

				++code;

				// Read line
				Uint32 line = *(Uint32*)code;
				if ( line == breakpoint.m_line )
				{
					// Toggle breakpoint flag
					code += sizeof( Uint32 );
					*code = status ? 1 : 0;
					found = true;
				}
			}
		}
	}
	

	// Return flag
	return found;
}

Bool CScriptCompiledCode::BreakpointIsSet( const ScriptBreakpoint& breakpoint ) const
{
	// Breakpoint should be in the same file
	if ( breakpoint.m_line >= m_sourceLine && m_sourceFile.EqualsNC( breakpoint.m_file ) )
	{
		// Find the breakpoint with matching line
		for ( Uint32 i = 0; i < m_breakpoints.Size(); ++i )
		{
			// Get the breakpoint header
			Uint8* code = (Uint8*)m_code.GetData() + m_breakpoints[i];
			ASSERT( *code == OP_Breakpoint );
			code++;

			// Read line
			Uint32 line = *(Uint32*)code;
			if ( line == breakpoint.m_line )
			{
				// Step over line value in code buffer
				code += sizeof( Uint32 );
				return ( *code != 0 );
			}
		}
	}

	// Not found
	return false;
}

Bool CScriptCompiledCode::BreakpointGetActive( TDynArray< ScriptBreakpoint >& breakpoints ) const
{
	// Get the lines breakpoints are set on
	TDynArray< Uint32 > lines;
	for ( Uint32 i=0; i<m_breakpoints.Size(); i++ )
	{
		// Get the breakpoint header
		Uint8* code = (Uint8*)m_code.GetData() + m_breakpoints[i];
		ASSERT( *code == OP_Breakpoint );
		code++;

		// Read the line
		Uint32 line = *(Uint32*)code;
		code += sizeof( Uint32 );

		// Is set ?
		if ( *code )
		{
			lines.PushBackUnique( line );
		}
	}

	// Report breakpoints
	for ( Uint32 i=0; i<lines.Size(); i++ )
	{
		ScriptBreakpoint breakpoint( GetSourceFile(), lines[i] );
		breakpoints.PushBack( breakpoint );
	}

	// Returns true if there were at least one active breakpoint in this function
	return lines.Size() > 0;
}

const Uint8* CScriptCompiledCode::GetCodeBehindSavePoint( const CName& savepointName ) const
{
	const Uint8* codeStart = GetCode();

	for ( TDynArray< ScriptSavepoint >::const_iterator it = m_savepoints.Begin();
			it != m_savepoints.End(); ++it )
	{
		if ( it->GetName( codeStart ) == savepointName )
		{
			codeStart = it->GetSavePointEndCode( codeStart );
			break;
		}
	}

	ASSERT( codeStart < GetCodeEnd(), TXT("Jumping behind the end of the function code") );

	return codeStart;
}

///////////////////////////////////////////////////////////////////////////////

const CName& ScriptSavepoint::GetName( const Uint8* codeStart ) const
{
	const CName* name = reinterpret_cast< const CName* >( codeStart + m_offset + 3 );
	return *name;
}

const Uint8* ScriptSavepoint::GetSavePointEndCode( const Uint8* codeStart ) const
{
	Uint16 skipSize = *reinterpret_cast< const Uint16* >( codeStart + m_offset + 1 );
	return codeStart + m_offset + 3 + skipSize;
}
