/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "scriptFunctionParser.h"
#include "scriptSyntaxNode.h"
#include "scriptTokenStream.h"
#include "functionBuilder.h"
#include "scriptCompiler.h"
#include "class.h"
#include "object.h"

/// Structure to pass around parser parameters
struct YYSTYPE_Function
{
	//! Constructor
	RED_INLINE YYSTYPE_Function()
		: m_dword( 0 )
		, m_integer( 0 )
		, m_bool( false )
		, m_float( 0.0f )
		, m_node( NULL )
	{
	}

	//! Destructor
	RED_INLINE ~YYSTYPE_Function()
	{
		// m_value is NOT destroyed here... !
	}

	//! Copy
	RED_INLINE YYSTYPE_Function( const YYSTYPE_Function& other )
		: m_string( other.m_string )
		, m_dword( other.m_dword )
		, m_integer( other.m_integer )
		, m_bool( other.m_bool )
		, m_float( other.m_float )
		, m_name( other.m_name )
		, m_context( other.m_context )
		, m_token( other.m_token )
		, m_node( other.m_node )
	{
	}

	//! Move
	RED_INLINE YYSTYPE_Function( YYSTYPE_Function&& other )
		: m_string( Move( other.m_string ) )
		, m_dword( Move( other.m_dword ) )
		, m_integer( Move( other.m_integer ) )
		, m_bool( Move( other.m_bool ) )
		, m_float( Move( other.m_float ) )
		, m_name( Move( other.m_name ) )
		, m_context( Move( other.m_context ) )
		, m_token( Move( other.m_token ) )
		, m_node( Move( other.m_node ) )
	{}

	//! Assign
	RED_INLINE YYSTYPE_Function& operator=( const YYSTYPE_Function& other )
	{
		if ( &other != this )
		{
			m_string = other.m_string;
			m_dword = other.m_dword;
			m_integer = other.m_integer;
			m_bool = other.m_bool;
			m_float = other.m_float;
			m_name = other.m_name;
			m_context = other.m_context;
			m_token = other.m_token;
			m_node = other.m_node;
		}
		return *this;
	}

	//! Move
	RED_INLINE YYSTYPE_Function& operator=( YYSTYPE_Function&& other )
	{
		if ( &other != this )
		{
			m_string = Move( other.m_string );
			m_dword = Move( other.m_dword );
			m_integer = Move( other.m_integer );
			m_bool = Move( other.m_bool );
			m_float = Move( other.m_float );
			m_name = Move( other.m_name );
			m_context = Move( other.m_context );
			m_token = Move( other.m_token );
			m_node = Move( other.m_node );
		}
		return *this;
	}


	//! Data
	String					m_string;
	Uint32					m_dword;
	Int32					m_integer;
	Bool					m_bool;
	Float					m_float;
	CName					m_name;
	CScriptFileContext		m_context;
	String					m_token;
	CScriptSyntaxNode*		m_node;
}; 

#include RED_EXPAND_AND_STRINGIFY(PROJECT_PLATFORM\PROJECT_CONFIGURATION\scriptFunctionParser_bison.cxx)

CScriptFunctionParser::CScriptFunctionParser( CScriptCompiler* compiler )
: m_compiler( compiler )
, m_rootSyntaxNode( NULL )
, m_hasError( false )
{
}

CScriptFunctionParser::~CScriptFunctionParser()
{
}

void CScriptFunctionParser::EmitError( const CScriptFileContext& context, const String& text )
{
	// Pass to compiler
	m_hasError = true;
	m_compiler->ScriptError( context, TXT("%ls"), text.AsChar() );
}

CScriptSyntaxNode* CScriptFunctionParser::ParseFunctionCode( CScriptFunctionStub* stub )
{
	// Reset code
	m_rootSyntaxNode = NULL;

	// Convert identifiers inside a casts to types :)
	{
		CScriptTokenStream::Tokens& tokens = stub->m_code.GetTokens();
		if ( tokens.Size() >= 3 )
		{
			for ( Uint32 i = 1; i < tokens.Size() - 1; ++i )
			{
				// Casting pattern '(' ident ')', if ident is a CLASS NAME then change it to a handle to className
				if ( tokens[ i - 1 ].m_text == TXT( "(" ) && tokens[ i + 1 ].m_text == TXT( ")" ) )
				{
					if ( tokens[ i ].m_token == TOKEN_IDENT )
					{
						CName name( tokens[ i ].m_text );

						const CClass* theClass = SRTTI::GetInstance().FindClass( name );
						if ( theClass && theClass->IsBasedOn( ClassID< IScriptable >() ) )
						{
							// Change the type
							tokens[ i ].m_text = String::Printf( TXT( "handle:%ls" ), theClass->GetName().AsString().AsChar() );
							tokens[ i ].m_token = TOKEN_CLASS_TYPE;
						}
						else
						{
							const CEnum* theEnum = SRTTI::GetInstance().FindEnum( name );

							if( theEnum )
							{
								tokens[ i ].m_token = TOKEN_ENUM_TYPE;
							}
						}
					}
				}
			}
		}
	}

	// Find local function properties that has initialization code
	// Put this code at the beginning of function body code
	{
		CScriptTokenStream::Tokens& codeTokens = stub->m_code.GetTokens();
		if ( codeTokens.Size() > 0 )
		{
			CScriptTokenStream::Tokens initLocalsTokens;
			for ( auto stubI = stub->m_locals.Begin(); stubI != stub->m_locals.End(); ++stubI )
			{
				if ( !(*stubI)->m_initCodeTokens.Empty() )
				{
					CScriptToken token;
					token.m_context = (*stubI)->m_context;
					token.m_level = codeTokens[0].m_level;
					token.m_text = (*stubI)->m_name;
					token.m_token = TOKEN_IDENT;
					initLocalsTokens.PushBack( token );
					initLocalsTokens.PushBack( (*stubI)->m_initCodeTokens );
				}
			}

			if ( !initLocalsTokens.Empty() )
			{
				CScriptTokenStream::Tokens tmp = codeTokens;
				codeTokens.Clear();
				codeTokens.PushBack( initLocalsTokens );
				codeTokens.PushBack( tmp );
			}
		}
	}


	// Initialize file parser
	CScriptTokenStream& tokens = stub->m_code;
	GInitFunctionParser( &tokens, this );

	// Parse
	yyparse();

	// There were errors
	if ( m_hasError )
	{
		if ( m_rootSyntaxNode )
		{
			m_rootSyntaxNode->Release( true );
			m_rootSyntaxNode = NULL;
		}
	}

	// Done, return parsed syntax node
	return m_rootSyntaxNode;
}

void CScriptFunctionParser::SetRootSyntaxNode( CScriptSyntaxNode* node )
{
	if ( !m_rootSyntaxNode && node )
	{
		// Use only the first syntax tree
		m_rootSyntaxNode = node;
	}
	else if ( node )
	{
		node->Release( true );
	}
}
