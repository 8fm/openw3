/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#include "scriptFileParser.h"
#include "scriptLexer.h"
#include "scriptFileContext.h"
#include "scriptDefaultValue.h"
#include "scriptTokenStream.h"
#include "scriptFieldStubs.h"
#include "scriptCompiler.h"
#include "fileSys.h"
#include "class.h"
#include "functionFlags.h"

/// Structure to pass around parser parameters
struct YYSTYPE_File
{
	//! Constructor
	RED_INLINE YYSTYPE_File()
		: m_dword( 0 )
		, m_integer( 0 )
		, m_bool( false )
		, m_float( 0.0f )
		, m_value( NULL )
	{
	}

	//! Destructor
	RED_INLINE ~YYSTYPE_File()
	{
		// m_value is NOT destroyed here... !
	}

	//! Copy
	RED_INLINE YYSTYPE_File( const YYSTYPE_File& other )
		: m_string( other.m_string )
		, m_dword( other.m_dword )
		, m_integer( other.m_integer )
		, m_bool( other.m_bool )
		, m_float( other.m_float )
		, m_typeName( other.m_typeName )
		, m_context( other.m_context )
		, m_token( other.m_token )
		, m_value( other.m_value )
		, m_idents( other.m_idents )
	{
	}

	//! Move
	RED_INLINE YYSTYPE_File( YYSTYPE_File&& other )
		: m_string( Move( other.m_string ) )
		, m_dword( Move( other.m_dword ) )
		, m_integer( Move( other.m_integer ) )
		, m_bool( Move( other.m_bool ) )
		, m_float( Move( other.m_float ) )
		, m_context( Move( other.m_context ) )
		, m_typeName( Move( other.m_typeName ) )
		, m_token( Move( other.m_token ) )
		, m_value( Move( other.m_value ) )
		, m_idents( Move( other.m_idents ) )
	{}

	//! Assign
	RED_INLINE YYSTYPE_File& operator=( const YYSTYPE_File& other )
	{
		if ( &other != this )
		{
			m_string = other.m_string;
			m_dword = other.m_dword;
			m_integer = other.m_integer;
			m_bool = other.m_bool;
			m_float = other.m_float;
			m_context = other.m_context;
			m_typeName = other.m_typeName;
			m_token = other.m_token;
			m_value = other.m_value;
			m_idents = other.m_idents;
		}
		return *this;
	}

	//! Move
	RED_INLINE YYSTYPE_File& operator=( YYSTYPE_File&& other )
	{
		if ( &other != this )
		{
			m_string = Move( other.m_string );
			m_dword = Move( other.m_dword );
			m_integer = Move( other.m_integer );
			m_bool = Move( other.m_bool );
			m_float = Move( other.m_float );
			m_context = Move( other.m_context );
			m_token = Move( other.m_token );
			m_typeName = Move( other.m_typeName );
			m_value = Move( other.m_value );
			m_idents = Move( other.m_idents );
		}
		return *this;
	}

	typedef TDynArray< String, MC_ScriptCompilation, MemoryPool_ScriptCompilation > Idents;

	//! Data
	String					m_string;
	Uint32					m_dword;
	Int32					m_integer;
	Bool					m_bool;
	Float					m_float;
	String					m_typeName;
	CScriptFileContext*		m_context;
	String					m_token;
	CScriptDefaultValue*	m_value;
	Idents					m_idents;
}; 

#include RED_EXPAND_AND_STRINGIFY(PROJECT_PLATFORM\PROJECT_CONFIGURATION\scriptfileparser_bison.cxx)

CScriptFileParser::CScriptFileParser( CScriptSystemStub& outputStubs, CScriptCompiler* compiler )
	: m_compiler( compiler )
	, m_currentClass( NULL )
	, m_currentStruct( NULL )
	, m_currentEnum( NULL )
	, m_currentFunction( NULL )
	, m_lastAddedFunctionLocalPropertyStub( NULL )
	, m_stubs( outputStubs )
	, m_allocProxy( &CScriptFileParser::Realloc, &CScriptFileParser::Free )
{
	m_definition.Initialize( &m_allocProxy );

	m_definition.AddLiteral( Red::Scripts::Literal_String, TOKEN_STRING );
	m_definition.AddLiteral( Red::Scripts::Literal_Name, TOKEN_NAME );
	m_definition.AddLiteral( Red::Scripts::Literal_Integer, TOKEN_INTEGER );
	m_definition.AddLiteral( Red::Scripts::Literal_Float, TOKEN_FLOAT );
	m_definition.AddLiteral( Red::Scripts::Literal_Identifier, TOKEN_IDENT );

	// Keywords
	m_definition.AddKeyword( TXT("import"), TOKEN_IMPORT	); 
	m_definition.AddKeyword( TXT("abstract"), TOKEN_ABSTRACT ); 
	m_definition.AddKeyword( TXT("const"), TOKEN_CONST ); 
	m_definition.AddKeyword( TXT("extends"), TOKEN_EXTENDS ); 
	m_definition.AddKeyword( TXT("in"), TOKEN_IN	); 
	m_definition.AddKeyword( TXT("class"), TOKEN_CLASS ); 
	m_definition.AddKeyword( TXT("statemachine"), TOKEN_STATEMACHINE ); 
	m_definition.AddKeyword( TXT("state"), TOKEN_STATE ); 
	m_definition.AddKeyword( TXT("enum"), TOKEN_ENUM ); 
	m_definition.AddKeyword( TXT("struct"), TOKEN_STRUCT ); 
	m_definition.AddKeyword( TXT("function"), TOKEN_FUNCTION ); 
	m_definition.AddKeyword( TXT("def"), TOKEN_DEF ); 
	m_definition.AddKeyword( TXT("editable"), TOKEN_EDITABLE ); 
	m_definition.AddKeyword( TXT("final"), TOKEN_FINAL ); 
	m_definition.AddKeyword( TXT("out"), TOKEN_OUT ); 
	m_definition.AddKeyword( TXT("optional"), TOKEN_OPTIONAL ); 
	m_definition.AddKeyword( TXT("autobind"), TOKEN_AUTOBIND ); 
	m_definition.AddKeyword( TXT("any"), TOKEN_ANY ); 
	m_definition.AddKeyword( TXT("single"), TOKEN_SINGLE ); 
	m_definition.AddKeyword( TXT("local"), TOKEN_LOCAL ); 
	m_definition.AddKeyword( TXT("inlined"), TOKEN_INLINED );
	m_definition.AddKeyword( TXT("private"), TOKEN_PRIVATE );
	m_definition.AddKeyword( TXT("protected"), TOKEN_PROTECTED );
	m_definition.AddKeyword( TXT("public"), TOKEN_PUBLIC );
	m_definition.AddKeyword( TXT("event"), TOKEN_EVENT );
	m_definition.AddKeyword( TXT("timer"), TOKEN_TIMER );
	m_definition.AddKeyword( TXT("cleanup"), TOKEN_CLEANUP );
	m_definition.AddKeyword( TXT("array"), TOKEN_ARRAY );
	m_definition.AddKeyword( TXT("string"), TOKEN_TYPE_STRING );
	m_definition.AddKeyword( TXT("byte"), TOKEN_TYPE_BYTE );
	m_definition.AddKeyword( TXT("bool"), TOKEN_TYPE_BOOL );
	m_definition.AddKeyword( TXT("int"), TOKEN_TYPE_INT );
	m_definition.AddKeyword( TXT("float"), TOKEN_TYPE_FLOAT );
	m_definition.AddKeyword( TXT("name"), TOKEN_TYPE_NAME );
	m_definition.AddKeyword( TXT("void"), TOKEN_TYPE_VOID );
	m_definition.AddKeyword( TXT("defaults"), TOKEN_DEFAULTS );
	m_definition.AddKeyword( TXT("default"), TOKEN_DEFAULT );
	m_definition.AddKeyword( TXT("hint"), TOKEN_HINT );
	m_definition.AddKeyword( TXT("true"), TOKEN_BOOL_TRUE );
	m_definition.AddKeyword( TXT("false"), TOKEN_BOOL_FALSE );
	m_definition.AddKeyword( TXT("NULL"), TOKEN_NULL );
	m_definition.AddKeyword( TXT("entry"), TOKEN_ENTRY );
	m_definition.AddKeyword( TXT("enter"), TOKEN_ENTER );
	m_definition.AddKeyword( TXT("latent"), TOKEN_LATENT );
	m_definition.AddKeyword( TXT("var"), TOKEN_VAR );
	m_definition.AddKeyword( TXT("exec"), TOKEN_EXEC );
	m_definition.AddKeyword( TXT("storyscene"), TOKEN_SCENE );
	m_definition.AddKeyword( TXT("saved"), TOKEN_SAVED );
	m_definition.AddKeyword( TXT("quest"), TOKEN_QUEST );
	m_definition.AddKeyword( TXT("savepoint"), TOKEN_SAVEPOINT );
	m_definition.AddKeyword( TXT("reward"), TOKEN_REWARD );

	m_definition.AddKeyword( TXT("+="), TOKEN_OP_IADD );
	m_definition.AddKeyword( TXT("-="), TOKEN_OP_ISUB );
	m_definition.AddKeyword( TXT("*="), TOKEN_OP_IMUL );
	m_definition.AddKeyword( TXT("/="), TOKEN_OP_IDIV );
	m_definition.AddKeyword( TXT("&="), TOKEN_OP_IAND );
	m_definition.AddKeyword( TXT("|="), TOKEN_OP_IOR );
	m_definition.AddKeyword( TXT("||"), TOKEN_OP_LOGIC_OR );
	m_definition.AddKeyword( TXT("&&"), TOKEN_OP_LOGIC_AND );
	m_definition.AddKeyword( TXT("=="), TOKEN_OP_EQUAL );
	m_definition.AddKeyword( TXT("!="), TOKEN_OP_NOTEQUAL );
	m_definition.AddKeyword( TXT("=="), TOKEN_OP_EQUAL );
	m_definition.AddKeyword( TXT(">="), TOKEN_OP_GREQ );
	m_definition.AddKeyword( TXT("<="), TOKEN_OP_LEEQ );
	m_definition.AddKeyword( TXT("new"), TOKEN_NEW );
	m_definition.AddKeyword( TXT("in"), TOKEN_IN );
	m_definition.AddKeyword( TXT("delete"), TOKEN_DELETE );
	m_definition.AddKeyword( TXT("if"), TOKEN_IF );
	m_definition.AddKeyword( TXT("else"), TOKEN_ELSE );
	m_definition.AddKeyword( TXT("switch"), TOKEN_SWITCH );
	m_definition.AddKeyword( TXT("case"), TOKEN_CASE );
	m_definition.AddKeyword( TXT("for"), TOKEN_FOR );
	m_definition.AddKeyword( TXT("while"), TOKEN_WHILE );
	m_definition.AddKeyword( TXT("do"), TOKEN_DO );
	m_definition.AddKeyword( TXT("return"), TOKEN_RETURN );
	m_definition.AddKeyword( TXT("break"), TOKEN_BREAK );
	m_definition.AddKeyword( TXT("continue"), TOKEN_CONTINUE );
	m_definition.AddKeyword( TXT("this"), TOKEN_THIS );
	m_definition.AddKeyword( TXT("super"), TOKEN_SUPER );
	m_definition.AddKeyword( TXT("parent"), TOKEN_PARENT );
	m_definition.AddKeyword( TXT("virtual_parent"), TOKEN_VIRTUAL_PARENT );

	m_definition.AddKeyword( TXT("theGame"), TOKEN_GET_GAME );
	m_definition.AddKeyword( TXT("thePlayer"), TOKEN_GET_PLAYER );
	m_definition.AddKeyword( TXT("theCamera"), TOKEN_GET_CAMERA );
	m_definition.AddKeyword( TXT("theSound"), TOKEN_GET_SOUND );
	m_definition.AddKeyword( TXT("theDebug"), TOKEN_GET_DEBUG );
	m_definition.AddKeyword( TXT("theTimer"), TOKEN_GET_TIMER );
	m_definition.AddKeyword( TXT("theInput"), TOKEN_GET_INPUT );
	m_definition.AddKeyword( TXT("theTelemetry"), TOKEN_GET_TELEMETRY );

	// Chars
	m_definition.AddChar( '(' );
	m_definition.AddChar( ')' );
	m_definition.AddChar( '{' );
	m_definition.AddChar( '}' );
	m_definition.AddChar( '[' );
	m_definition.AddChar( ']' );
	m_definition.AddChar( '<' );
	m_definition.AddChar( '>' );
	m_definition.AddChar( ',' );
	m_definition.AddChar( '=' );
	m_definition.AddChar( ';' );
	m_definition.AddChar( '&' );
	m_definition.AddChar( '|' );
	m_definition.AddChar( '^' );
	m_definition.AddChar( '+' );
	m_definition.AddChar( '-' );
	m_definition.AddChar( '*' );
	m_definition.AddChar( '/' );
	m_definition.AddChar( '%' );
	m_definition.AddChar( '.' );
	m_definition.AddChar( '!' );
	m_definition.AddChar( '?' );
	m_definition.AddChar( ':' );
	m_definition.AddChar( '~' );
}

CScriptFileParser::~CScriptFileParser()
{
}

void CScriptFileParser::EmitError( const CScriptFileContext* context, const String& text )
{
	// Pass to compiler
	m_compiler->ScriptError( *context, TXT("%") RED_PRIWs, text.AsChar() );
}

Bool CScriptFileParser::ParseFile( const String& scriptFilePath, const String& scriptShortFilePath )
{
	// Open file
	String scriptCode;
	if ( !GFileManager->LoadFileToString( scriptFilePath, scriptCode, true ) )
	{
		CScriptFileContext fakeContext( scriptShortFilePath, 1 );
		EmitError( &fakeContext, TXT( "Unable to open file" ) );
		return false;
	}

	// Tokenize
	CScriptTokenStream tokens;
	CScriptLexer lexer( m_definition, tokens, scriptShortFilePath, m_compiler );

	lexer.Initialize( &m_allocProxy );

	if ( !lexer.Tokenize( scriptCode.AsChar() ) )
	{
		return false;
	}

	// Initialize file parser
	GInitFileParser( &tokens, this );

	// Parse
	yyparse();

	// Parse tokens
	return true;
}

#define TRANSLATE_FLAG( x, y )	\
	translatedFlags |= ( flags & x ) ? y : 0;

void CScriptFileParser::StartClass( const CScriptFileContext* context, const String& name, const String& extents, Uint32 flags )
{
	// Translate flags
	Uint32 translatedFlags = 0;
	TRANSLATE_FLAG( SSF_Import, CF_Native );
	TRANSLATE_FLAG( SSF_Abstract, CF_Abstract );
	TRANSLATE_FLAG( SSF_StateMachine, CF_StateMachine );

	// Create class
	RED_FATAL_ASSERT( context != nullptr, "Invalid context for class" );
	m_currentClass = new CScriptClassStub( *context, name, extents, translatedFlags );
	m_stubs.AddClass( m_currentClass );
}

void CScriptFileParser::StartState( const CScriptFileContext* context, const String& name, const String& parentClass, Uint32 flags, const String& extends )
{
	// Translate flags
	Uint32 translatedFlags = 0;
	TRANSLATE_FLAG( SSF_Import, CF_Native );
	TRANSLATE_FLAG( SSF_Abstract, CF_Abstract );

	// Create state
	RED_FATAL_ASSERT( context != nullptr, "Invalid context for state" );
	m_currentClass = new CScriptClassStub( *context, name, extends, translatedFlags );
	m_currentClass->m_isState = true;
	m_currentClass->m_stateParentClass = parentClass;
	m_stubs.AddClass( m_currentClass );
}

void CScriptFileParser::StartStruct( const CScriptFileContext* context, const String& name, Uint32 flags )
{
	// Translate flags
	Uint32 translatedFlags = 0;
	TRANSLATE_FLAG( SSF_Import, CF_Native );

	// Create Struct
	RED_FATAL_ASSERT( context != nullptr, "Invalid context for struct" );
	m_currentStruct = new CScriptStructStub( *context, name, translatedFlags );
	m_stubs.AddStruct( m_currentStruct );
}

void CScriptFileParser::StartEnum( const CScriptFileContext* context, const String& name )
{
	// Create Enum
	RED_FATAL_ASSERT( context != nullptr, "Invalid context for enum" );
	m_currentEnum = new CScriptEnumStub( *context, name );
	m_stubs.AddEnum( m_currentEnum );
}

void CScriptFileParser::SetReturnValueType( const String& retTypeName )
{
	if ( m_currentFunction )
	{
		m_currentFunction->SetReturnType( retTypeName );
	}
}

void CScriptFileParser::StartFunction( const CScriptFileContext* context, const String& name, Uint32 flags )
{
	// Translate flags
	Uint32 translatedFlags = 0;
	TRANSLATE_FLAG( SSF_Import, FF_NativeFunction );
	TRANSLATE_FLAG( SSF_Final, FF_FinalFunction );
	TRANSLATE_FLAG( SSF_Private, FF_PrivateFunction );
	TRANSLATE_FLAG( SSF_Protected, FF_ProtectedFunction );
	TRANSLATE_FLAG( SSF_Public, FF_PublicFunction );
	TRANSLATE_FLAG( SSF_Event, FF_EventFunction );
	TRANSLATE_FLAG( SSF_Latent, FF_LatentFunction );
	TRANSLATE_FLAG( SSF_Entry, FF_EntryFunction );
	TRANSLATE_FLAG( SSF_Exec, FF_ExecFunction );
	TRANSLATE_FLAG( SSF_Timer, FF_TimerFunction );
	TRANSLATE_FLAG( SSF_Scene, FF_SceneFunction );
	TRANSLATE_FLAG( SSF_Quest, FF_QuestFunction );
	TRANSLATE_FLAG( SSF_Reward, FF_RewardFunction );
	TRANSLATE_FLAG( SSF_Cleanup, FF_CleanupFunction );

	RED_FATAL_ASSERT( context != nullptr, "Invalid context for function" );

	// Create function
	if ( m_currentClass )
	{
		// Start a function
		m_currentFunction = new CScriptFunctionStub( *context, name, translatedFlags );
		m_currentClass->AddFunction( m_currentFunction );
	}
	else
	{
		// Start global function
		m_currentFunction = new CScriptFunctionStub( *context, name, translatedFlags | FF_StaticFunction );
		m_stubs.AddFunction( m_currentFunction );
	}
}

void CScriptFileParser::SetFunctionUndefined()
{
	ASSERT( m_currentFunction, TXT( "This function cannot be called if a function is not being declared" ) );

	m_currentFunction->m_flags |= FF_UndefinedBody;
}

void CScriptFileParser::AddProperty( const CScriptFileContext* context, const String& name, Uint32 flags, const String& typeName, Bool isFunctionParameter )
{
	// Translate flags
	Uint32 translatedFlags = 0;
	TRANSLATE_FLAG( SSF_Import, PF_Native );
	TRANSLATE_FLAG( SSF_Const, PF_ReadOnly );
	TRANSLATE_FLAG( SSF_Editable, PF_Editable );
	TRANSLATE_FLAG( SSF_Inlined, PF_Inlined );
	TRANSLATE_FLAG( SSF_Optional, PF_FuncOptionaParam );
	TRANSLATE_FLAG( SSF_Out, PF_FuncOutParam);
	TRANSLATE_FLAG( SSF_Saved, PF_Saved);
	TRANSLATE_FLAG( SSF_Private, PF_Private );
	TRANSLATE_FLAG( SSF_Protected, PF_Protected );
	TRANSLATE_FLAG( SSF_Public, PF_Public );

	// Create property
	RED_FATAL_ASSERT( context != nullptr, "Invalid context for Property" );
	CScriptPropertyStub* stub = new CScriptPropertyStub( *context, name, translatedFlags, typeName );

	// Add to scope
	if ( m_currentFunction )
	{
		if ( isFunctionParameter )
		{
			stub->m_flags |= PF_FuncParam;
			m_currentFunction->AddParam( stub );
		}
		else
		{
			stub->m_flags |= PF_FuncLocal;
			m_currentFunction->AddLocal( stub );
			m_lastAddedFunctionLocalPropertyStub = stub;
		}
	}
	else if ( m_currentStruct )
	{
		m_currentStruct->AddField( stub );
		// dex note: scripted struct properties does not have the PF_Scripted flag set
		// that is due to the fact that structs are always allocated in the "normal" memory (directly under "this" pointer)
		// PF_Scripted flag changes this behavior and works only for CObjects that have a special buffer with "scripted data".
	}
	else if ( m_currentClass )
	{
		stub->m_flags |= PF_Scripted;
		m_currentClass->AddProperty( stub );
	}
}

void CScriptFileParser::AddBindableProperty( const CScriptFileContext* context, const String& name, Uint32 flags, const String& typeName, const String& binding )
{
	Uint32 translatedFlags = 0;
	TRANSLATE_FLAG( SSF_Private, PF_Private );
	TRANSLATE_FLAG( SSF_Protected, PF_Protected );
	TRANSLATE_FLAG( SSF_Public, PF_Public );
	TRANSLATE_FLAG( SSF_Optional, PF_AutoBindOptional );

	// Special flags
	translatedFlags |= PF_ReadOnly;
	translatedFlags |= PF_AutoBind;
	translatedFlags |= PF_Scripted;
	translatedFlags |= PF_NotSerialized;

	// Create property
	RED_FATAL_ASSERT( context != nullptr, "Invalid context for BindableProperty" );
	CScriptPropertyStub* stub = new CScriptPropertyStub( *context, name, translatedFlags, typeName );

	// Setup binding information
	stub->m_binding = binding;

	if ( m_currentClass )
	{
		m_currentClass->AddProperty( stub );
	}
}

void CScriptFileParser::SetLastFunctionPropertyInitCode( CScriptTokenStream& initCode )
{
	ASSERT( m_lastAddedFunctionLocalPropertyStub );
	m_lastAddedFunctionLocalPropertyStub->m_initCodeTokens = initCode.GetTokens();
}

void CScriptFileParser::ResetFunctionPropertyList()
{
	m_lastAddedFunctionLocalPropertyStub = NULL;
}

void CScriptFileParser::AddEmumOption( const CScriptFileContext* context, const String& name, Uint32 value )
{
	ASSERT( m_currentEnum );

	RED_FATAL_ASSERT( context != nullptr, "Invalid context for enum option" );
	CScriptEnumOptionStub* stub = new CScriptEnumOptionStub( *context, name, value );
	m_currentEnum->AddOption( stub );
}

void CScriptFileParser::AddCode( const CScriptTokenStream& tokens )
{
	ASSERT( m_currentFunction );

	m_currentFunction->m_code = tokens;
}

void CScriptFileParser::AddDefaultValue( const CScriptFileContext* context, const String& name, CScriptDefaultValue* value )
{
	RED_FATAL_ASSERT( context != nullptr, "Invalid context for default value" );

	if ( m_currentStruct )
	{
		CScriptDefaultValueStub* stub = new CScriptDefaultValueStub( *context, name, value );
		m_currentStruct->AddDefaultValue( stub );
	}
	else if ( m_currentClass )
	{
		CScriptDefaultValueStub* stub = new CScriptDefaultValueStub( *context, name, value );
		m_currentClass->AddDefaultValue( stub );
	}
	else
	{
		delete value;
	}
}

//! Add a hint
void CScriptFileParser::AddHint( const CScriptFileContext*, const String& name, const String& hint )
{
	if ( m_currentStruct )
	{
		m_currentStruct->AddHint( name, hint );
	}
	else if ( m_currentClass )
	{
		m_currentClass->AddHint( name, hint );
	}
}

void CScriptFileParser::PopContext()
{
	// Exit context
	if ( m_currentEnum )
	{
		m_currentEnum = NULL;
	}
	else if ( m_currentFunction )
	{
		if( m_currentClass )
		{
			if( m_currentFunction->m_flags & FF_UndefinedBody && !( m_currentFunction->m_flags & FF_NativeFunction ) )
			{
				m_currentClass->m_flags |= CF_UndefinedFunctions;
			}
		}

		m_currentFunction = NULL;
	}
	else if ( m_currentStruct )
	{
		m_currentStruct = NULL;
	}
	else if ( m_currentClass )
	{
		m_currentClass = NULL;
	}
}

void* CScriptFileParser::Realloc( void* ptr, size_t size )
{
	return RED_MEMORY_REALLOCATE( MemoryPool_ScriptCompilation, ptr, MC_ScriptCompilation, size );
}

void CScriptFileParser::Free( void* ptr )
{
	RED_MEMORY_FREE( MemoryPool_ScriptCompilation, MC_ScriptCompilation, ptr );
}
