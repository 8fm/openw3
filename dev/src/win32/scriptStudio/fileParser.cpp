/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "fileParserType.h"
#include "lexData.h"

// Include generated bison file
#include RED_EXPAND_AND_STRINGIFY(PROJECT_PLATFORM\PROJECT_CONFIGURATION\fileParser_bison.cxx)

SSFileParser::SSFileParser()
:	m_stub( nullptr )
,	m_currentClass( nullptr )
,	m_currentStruct( nullptr )
,	m_currentEnum( nullptr )
,	m_currentFunction( nullptr )
{
}

SSFileParser::~SSFileParser()
{
}

bool SSFileParser::ParseFile( SSFileStub* stub, const wstring& fileName, const wstring& code, const Red::Scripts::LexerDefinition& definition )
{
	Red::AllocatorProxy proxy( &realloc, &free );

	m_stub = stub;

	SSNewFileLexer lexer( definition, m_stub->m_lexData, fileName );
	lexer.Initialize( &proxy );

	// Reset
	m_currentClass = nullptr;
	m_currentStruct = nullptr;
	m_currentEnum = nullptr;
	m_currentFunction = nullptr;
	m_hasError = false;

	// Tokenize file
	if ( !lexer.Tokenize( code.c_str() ) )
	{
		m_hasError = true;
		return false;
	}

	// Initialize file parser
	SSTokenStream tokenStream( lexer );
	GInitFileParser( &tokenStream, this );

	// Parse
	yyparse();

	// Return error flag

	return !m_hasError;
}

void SSFileParser::EmitError( const SSFileContext& context, const wstring& text )
{
	// Show error
	RED_LOG_ERROR( RED_LOG_CHANNEL( FileParser ), wxT( "%ls(%i): From Bison -> %ls" ), context.m_file.c_str(), context.m_line, text.c_str() );

	// We had an error
	m_hasError = true;
}

void SSFileParser::StartClass( const SSFileContext& context, const wstring& name, const wstring& extends, const SSFlagList& flags )
{
	// Start a class 
	m_currentClass = new SSClassStub( context, name, flags, extends );

	m_stub->AddClass( m_currentClass );
}

void SSFileParser::StartState( const SSFileContext& context, const wstring& name, const wstring& parentClass, const SSFlagList& flags, const wstring& extends )
{
	// Start a state
	m_currentClass = new SSClassStub( context, name, flags, extends, parentClass );

	m_stub->AddClass( m_currentClass );
}

void SSFileParser::StartStruct( const SSFileContext& context, const wstring& name, const SSFlagList& flags )
{
	// Start a struct 
	m_currentStruct = new SSClassStub( context, name, flags );

	m_stub->AddClass( m_currentStruct );
}

void SSFileParser::StartEnum( const SSFileContext& context, const wstring& name )
{
	// Start an struct
	m_currentEnum = new SSEnumStub( context, name );

	m_stub->AddEnum( m_currentEnum );
}

void SSFileParser::SetReturnValueType( const wstring& retTypeName )
{
	if ( m_currentFunction )
	{
		m_currentFunction->m_retValueType = retTypeName;
	}
}

void SSFileParser::StartFunction( const SSFileContext& context, const wstring& name, const SSFlagList& flags )
{
	if ( m_currentClass )
	{
		// Start a function
		m_currentFunction = new SSFunctionStub( context, m_currentClass->m_name, name, flags );
		m_currentClass->AddFunction( m_currentFunction );
	}
	else
	{
		// Start global function
		m_currentFunction = new SSFunctionStub( context, L"", name, flags );
		m_stub->AddFunction( m_currentFunction );
	}
}

void SSFileParser::AddProperty( const SSFileContext& context, const wstring& name, const SSFlagList& flags, const wstring& typeName, bool isFunctionParameter )
{
	// Create property
	SSPropertyStub* stub = new SSPropertyStub( context, name, flags, typeName );

	// Add to scope
	if ( m_currentFunction )
	{
		if ( isFunctionParameter )
		{
			stub->m_flags.Add( wxT("parameter") );
			m_currentFunction->AddParam( stub );
		}
		else
		{
			stub->m_flags.Add( wxT("local") );
			m_currentFunction->AddLocal( stub );
		}
	}
	else if ( m_currentStruct )
	{
		m_currentStruct->AddField( stub );
	}
	else if ( m_currentClass )
	{
		m_currentClass->AddField( stub );
	}
}

void SSFileParser::AddEmumOption( const SSFileContext& context, const wstring& name )
{
	if ( m_currentEnum )
	{
		int value = 0;
		if( m_currentEnum->m_options.size() > 0 )
		{
			const SSEnumOptionStub* previousOption = m_currentEnum->m_options.back();
			value = previousOption->m_value + 1;
		}

		SSEnumOptionStub* stub = new SSEnumOptionStub( context, name, value );
		m_currentEnum->AddOption( stub );
	}
}

void SSFileParser::AddEmumOption( const SSFileContext& context, const wstring& name, int value )
{
	if ( m_currentEnum )
	{
		SSEnumOptionStub* stub = new SSEnumOptionStub( context, name, value );
		m_currentEnum->AddOption( stub );
	}
}

void SSFileParser::PopContext()
{
	// Exit context
	if ( m_currentEnum )
	{
		m_currentEnum = nullptr;
	}
	else if ( m_currentFunction )
	{
		m_currentFunction = nullptr;
	}
	else if ( m_currentStruct )
	{
		m_currentStruct = nullptr;
	}
	else if ( m_currentClass )
	{
		m_currentClass = nullptr;
	}
}

void SSFileParser::EndFunction( const SSFileContext& context )
{
	if ( m_currentFunction )
	{ 
		RED_ASSERT( m_currentFunction->m_lastLine == -1 );
		m_currentFunction->m_lastLine = context.m_line;

		// Only for global functions
		if( !m_currentClass )
		{
			for( int i = m_currentFunction->m_context.m_line; i < m_currentFunction->m_lastLine; ++i )
			{
				m_stub->m_lineToStubMap[ i ] = m_currentFunction;
			}
		}
	}
}

void SSFileParser::EndStruct( const SSFileContext& context )
{
	EndClassStub( m_currentStruct, context );
}

void SSFileParser::EndClass( const SSFileContext& context )
{
	EndClassStub( m_currentClass, context );
}

void SSFileParser::EndClassStub( SSClassStub* stub, const SSFileContext& context )
{
	if ( stub )
	{
		RED_ASSERT( stub->m_lastLine == -1 );
		stub->m_lastLine = context.m_line;

		for( int i = stub->m_context.m_line; i < stub->m_lastLine; ++i )
		{
			m_stub->m_lineToStubMap[ i ] = stub;
		}
	}
}
