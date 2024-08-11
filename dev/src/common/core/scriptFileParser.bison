%{
// Some yacc (bison) defines
#define YYDEBUG 1
#define YYERROR_VERBOSE 1

/// Disable warnings caused by Bison (disable these when editing the grammar to make sure you don't cause any yourself)
RED_DISABLE_WARNING_MSC( 4244 )	// conversion from 'int' to 'short', possible loss of data
RED_DISABLE_WARNING_MSC( 4065 )	// switch statement contains 'default' but no 'case' labels
RED_DISABLE_WARNING_MSC( 4267 )	// conversion from 'size_t' to 'int', possible loss of data
RED_DISABLE_WARNING_MSC( 4127 )	// conditional expression is constant
RED_DISABLE_WARNING_MSC( 4702 )	// Unreachable code

/// The parsing token
#define YYSTYPE YYSTYPE_File
#include "scriptfileparser_bison.cxx.h"

/// Spawning info
struct TempSpawnInfo
{
	String								m_typeName;
	String								m_ident;
	Uint32								m_flags;
	CScriptFileContext					m_context;
	TDynArray< CScriptDefaultValue* >	m_compoundValues;
	
	inline TempSpawnInfo()
		: m_flags( 0 )
	{};
	
	CScriptDefaultValue* PushValue( CScriptDefaultValue* value )
	{
		m_compoundValues.PushBack( value );
		return value;
	}
	
	void PopValue()
	{
		ASSERT( m_compoundValues.Size() );
		m_compoundValues.PopBack();
	}
	
	void AddSubValue( CScriptDefaultValue* value )
	{
		ASSERT( m_compoundValues.Size() );	
		CScriptDefaultValue* top = m_compoundValues[ m_compoundValues.Size() - 1 ];
		top->AddSubValue( value );
	}
};

/// Parser context
struct FileParserContext
{
	TempSpawnInfo			m_spawnInfo;
	CScriptTokenStream*		m_tokens;
	CScriptFileParser*		m_parser;
	
	FileParserContext()
		: m_tokens( NULL )
		, m_parser( NULL )
	{};
};

// Temp spawn info
FileParserContext GFileParser;

// Reset
void GInitFileParser( CScriptTokenStream* tokens, CScriptFileParser* parser )
{
	// Bind data
	GFileParser.m_tokens = tokens;
	GFileParser.m_parser = parser;
}

// Get the type name for dynamic arrays
String GetDynamicArrayTypeName( const String& arrayType )
{
	Char typeName[ 512 ];
	Red::System::SNPrintF
	(
		typeName,
		ARRAY_COUNT(typeName),
		TXT( "array:%d,%d,%ls" ),
		MC_DynArray,
		Memory::GetPoolLabel< MemoryPool_Default >(),
		arrayType.AsChar()
	);

	return String( typeName );
}

// Get the type name static arrays
String GetStaticArrayTypeName( const String& arrayType, const Int32 arraySize )
{
	Char typeName[ 512 ];
	Red::System::SNPrintF
	(
		typeName,
		ARRAY_COUNT(typeName),
		TXT( "[%d]%ls" ), 
		arraySize,
		arrayType.AsChar()
	);

	return String( typeName );
}
 
// Error function
int yyerror( const char *msg )
{
	// Emit error
	String errorString = String::Printf( TXT( "%" ) RED_PRIWas TXT( ", near '%" ) RED_PRIWs TXT( "'" ), msg, yylval.m_token.AsChar() );
	GFileParser.m_parser->EmitError( yylval.m_context, errorString );
	
	// Continue parsing
	// TODO: thing about error resolve conditions
	return 0;
}

// Token reader
int yylex()
{
	if( GFileParser.m_tokens->IsEndOfStream() )
	{
		return 0;
	}
	
	CScriptToken& token = GFileParser.m_tokens->GetReadToken();
	GFileParser.m_tokens->IncrementReadPosition();

	yylval.m_context	= &token.m_context;
	yylval.m_token		= token.m_text;

	return token.m_token;
}

%}

/* ------------------------------------------------------------------
   Token definitions
   ------------------------------------------------------------------ */

/* Expect 0 shift/reduce conflicts */
%expect 0

/* THIS LIST SHOLD MATCH EXACTLY THE OTHER LIST IN THE scriptFunctionParser.bison */
/* THIS LIST SHOLD MATCH EXACTLY THE OTHER LIST IN THE scriptFunctionParser.bison */
/* THIS LIST SHOLD MATCH EXACTLY THE OTHER LIST IN THE scriptFunctionParser.bison */
/* THIS LIST SHOLD MATCH EXACTLY THE OTHER LIST IN THE scriptFunctionParser.bison */

/* Data tokens */
%token TOKEN_IDENT
%token TOKEN_INTEGER
%token TOKEN_FLOAT
%token TOKEN_NAME
%token TOKEN_STRING
%token TOKEN_BOOL_TRUE
%token TOKEN_BOOL_FALSE
%token TOKEN_NULL

/* Keywords */
%token TOKEN_IMPORT
%token TOKEN_ABSTRACT
%token TOKEN_CONST
%token TOKEN_EXTENDS
%token TOKEN_CLASS
%token TOKEN_STATE
%token TOKEN_STATEMACHINE
%token TOKEN_ENUM
%token TOKEN_STRUCT
%token TOKEN_FUNCTION
%token TOKEN_DEF
%token TOKEN_EDITABLE
%token TOKEN_FINAL
%token TOKEN_OUT
%token TOKEN_OPTIONAL
%token TOKEN_LOCAL
%token TOKEN_INLINED
%token TOKEN_ARRAY
%token TOKEN_DEFAULTS
%token TOKEN_DEFAULT
%token TOKEN_HINT
%token TOKEN_ENTRY
%token TOKEN_ENTER
%token TOKEN_LATENT
%token TOKEN_VAR
%token TOKEN_AUTOBIND
%token TOKEN_ANY
%token TOKEN_SINGLE
%token TOKEN_EXEC
%token TOKEN_TIMER
%token TOKEN_CLEANUP
%token TOKEN_SCENE
%token TOKEN_SAVED
%token TOKEN_QUEST
%token TOKEN_REWARD

/* Type names */
%token TOKEN_TYPE_BYTE
%token TOKEN_TYPE_BOOL
%token TOKEN_TYPE_INT
%token TOKEN_TYPE_FLOAT
%token TOKEN_TYPE_STRING
%token TOKEN_TYPE_NAME
%token TOKEN_TYPE_VOID

%token TOKEN_CLASS_TYPE
%token TOKEN_ENUM_TYPE

/* Function parsing tokens */
%token TOKEN_OP_IADD
%token TOKEN_OP_ISUB
%token TOKEN_OP_IMUL
%token TOKEN_OP_IDIV
%token TOKEN_OP_IAND
%token TOKEN_OP_IOR
%token TOKEN_OP_LOGIC_OR
%token TOKEN_OP_LOGIC_AND
%token TOKEN_OP_EQUAL
%token TOKEN_OP_NOTEQUAL
%token TOKEN_OP_GREQ
%token TOKEN_OP_LEEQ
%token TOKEN_NEW 
%token TOKEN_DELETE
%token TOKEN_IN 
%token TOKEN_IF
%token TOKEN_ELSE
%token TOKEN_SWITCH
%token TOKEN_CASE
%token TOKEN_FOR
%token TOKEN_WHILE
%token TOKEN_DO
%token TOKEN_RETURN
%token TOKEN_BREAK
%token TOKEN_CONTINUE
%token TOKEN_THIS
%token TOKEN_SUPER
%token TOKEN_PARENT
%token TOKEN_VIRTUAL_PARENT
%token TOKEN_PRIVATE
%token TOKEN_PROTECTED
%token TOKEN_PUBLIC
%token TOKEN_EVENT
%token TOKEN_SAVEPOINT
%token TOKEN_GET_GAME
%token TOKEN_GET_PLAYER
%token TOKEN_GET_CAMERA
%token TOKEN_GET_SOUND
%token TOKEN_GET_DEBUG
%token TOKEN_GET_TIMER
%token TOKEN_GET_INPUT
%token TOKEN_GET_TELEMETRY

%%

////////////////////////////////
// PROGRAM
////////////////////////////////

program_file
	: file_declaration_list
	;

////////////////////////////////
// DECLARATIONS
////////////////////////////////
 
file_declaration_list
	: file_declaration_list file_declaration
	| /* empty */
	;

file_declaration
	: struct_dcl optional_semicolon
	| enum_dcl optional_semicolon
	| class_dcl optional_semicolon
	| state_dcl optional_semicolon
	| function_dcl optional_semicolon
	;

////////////////////////////////
// COMMON
////////////////////////////////

import_flag
	: TOKEN_IMPORT			{ $<m_dword>$ = SSF_Import; }
	| /* empty */			{ $<m_dword>$ = 0; }
	;

access_flag
	: TOKEN_PRIVATE			{ $<m_dword>$ = SSF_Private; }
	| TOKEN_PROTECTED		{ $<m_dword>$ = SSF_Protected; }
	| TOKEN_PUBLIC			{ $<m_dword>$ = SSF_Public; }
	| /* empty */			{ $<m_dword>$ = 0; }
	;

class_flags
	: class_flag class_flags { $<m_dword>$ = $<m_dword>1 | $<m_dword>2; } 
	| /* empty */			 { $<m_dword>$ = 0; }
	;

class_flag
	: TOKEN_ABSTRACT		{ $<m_dword>$ = SSF_Abstract; }
	| TOKEN_STATEMACHINE	{ $<m_dword>$ = SSF_StateMachine; }
	| TOKEN_IMPORT			{ $<m_dword>$ = SSF_Import; }
	;

function_flag_final
	: TOKEN_FINAL			{ $<m_dword>$ = SSF_Final; }
	| /* empty */			{ $<m_dword>$ = 0; }
	;

function_flag_latent
	: TOKEN_LATENT			{ $<m_dword>$ = SSF_Latent; }
	| /* empty */			{ $<m_dword>$ = 0; }
	;

function_flag_type
	: TOKEN_EXEC			{ $<m_dword>$ = SSF_Exec; }
	| TOKEN_TIMER			{ $<m_dword>$ = SSF_Timer; }
	| TOKEN_SCENE			{ $<m_dword>$ = SSF_Scene; }
	| TOKEN_QUEST			{ $<m_dword>$ = SSF_Quest; }
	| TOKEN_REWARD			{ $<m_dword>$ = SSF_Reward; }
	| TOKEN_CLEANUP			{ $<m_dword>$ = SSF_Cleanup; }
	| TOKEN_ENTRY			{ $<m_dword>$ = SSF_Entry; }
	| /* empty */			{ $<m_dword>$ = 0; }
	;

var_flag_const
	: TOKEN_CONST			{ $<m_dword>$ = SSF_Const; }
	| /* empty */			{ $<m_dword>$ = 0; }
	;

var_flag_editable
	: TOKEN_EDITABLE		{ $<m_dword>$ = SSF_Editable; }
	| /* empty */			{ $<m_dword>$ = 0; }
	;

var_flag_inlined
	: TOKEN_INLINED			{ $<m_dword>$ = SSF_Inlined; }
	| /* empty */			{ $<m_dword>$ = 0; }
	;

var_flag_saved
	: TOKEN_SAVED			{ $<m_dword>$ = SSF_Saved; }
	| /* empty */			{ $<m_dword>$ = 0; }
	;

////////////////////////////////
// STATE
////////////////////////////////

state_dcl
	: state_header '{' class_elem_list '}' { GFileParser.m_parser->PopContext(); }
	;

state_header
	: class_flags TOKEN_STATE ident TOKEN_IN ident						{ GFileParser.m_parser->StartState( $<m_context>2, $<m_string>3, $<m_string>5, $<m_dword>1, String::EMPTY ); }
	| class_flags TOKEN_STATE ident TOKEN_IN ident TOKEN_EXTENDS ident	{ GFileParser.m_parser->StartState( $<m_context>2, $<m_string>3, $<m_string>5, $<m_dword>1, $<m_string>7 ); }
	;

////////////////////////////////
// CLASS
////////////////////////////////

class_dcl
	: class_header '{' class_elem_list '}' { GFileParser.m_parser->PopContext(); }
	;

class_header
	: class_flags TOKEN_CLASS ident class_extends { GFileParser.m_parser->StartClass( $<m_context>2, $<m_string>3, $<m_string>4, $<m_dword>1 ); }
	;

class_elem_list
	: class_elem_list class_element
	| /* empty */
	;

class_element
	: var_dcl semicolon
	| autobind_dcl semicolon
	| function_dcl
	| defaults_dcl
	| hint_dcl
	;

class_extends
	: TOKEN_EXTENDS ident	{ $<m_string>$ = $<m_string>2; }
	| /* empty */			{ $<m_string>$ = String::EMPTY; }
	;

////////////////////////////////
// VARIABLES
////////////////////////////////

var_common_flags
	:	import_flag access_flag { $<m_dword>$ = $<m_dword>1 | $<m_dword>2; }
	;

var_flags
	:	var_common_flags	
		var_flag_const
		var_flag_editable
		var_flag_inlined
		var_flag_saved
		{  $<m_dword>$ = $<m_dword>1 | $<m_dword>2 | $<m_dword>3 | $<m_dword>4 | $<m_dword>5; }
	;

var_dcl
	:	var_flags
		TOKEN_VAR
		function_param_ident_list ':' full_type
		{
			const YYSTYPE_File::Idents& idents = $<m_idents>3;
			for ( Uint32 i = 0; i < idents.Size(); ++i )
			{
				GFileParser.m_parser->AddProperty( $<m_context>2, idents[i], $<m_dword>1, $<m_typeName>5, false );
			}
		}
	;

////////////////////////////////
// AUTO BIND
////////////////////////////////

autobind_dcl
	: autobind_flags TOKEN_AUTOBIND ident ':' ident '=' autobind_binding
	{
		GFileParser.m_parser->AddBindableProperty( $<m_context>2, $<m_string>3, $<m_dword>1, $<m_string>5, $<m_string>7 );
	}
	;

autobind_flags
	: var_common_flags autobind_optional_flag  { $<m_dword>$ = $<m_dword>1 | $<m_dword>2; }
	;

autobind_optional_flag
	: TOKEN_OPTIONAL { $<m_dword>$ = SSF_Optional; }
	| /* empty */    { $<m_dword>$ = 0; }
	;

autobind_binding
	: name			{ $<m_string>$ = $<m_string>1; $<m_context>$ = $<m_context>1; }
	| string		{ $<m_string>$ = $<m_string>1; $<m_context>$ = $<m_context>1; }
	| TOKEN_ANY		{ $<m_string>$ = TXT("$any"); $<m_context>$ = $<m_context>1; }
	| TOKEN_SINGLE	{ $<m_string>$ = TXT("$single"); $<m_context>$ = $<m_context>1; }
	;

////////////////////////////////
// STRUCT
////////////////////////////////

struct_dcl
	:  struct_info '{' struct_elem_list '}'		{ GFileParser.m_parser->PopContext(); }
	;

struct_info
	: import_flag TOKEN_STRUCT ident { GFileParser.m_parser->StartStruct( $<m_context>3, $<m_string>3, $<m_dword>1 ); }
	;

struct_elem_list
	: struct_element struct_elem_list 
	| /* empty */
	;

struct_element
	: var_dcl semicolon
	| defaults_dcl
	| hint_dcl
	;

////////////////////////////////
// ENUM
////////////////////////////////

enum_dcl
	: TOKEN_ENUM enum_info '{' enum_name_list optional_comma '}'	{ GFileParser.m_parser->PopContext(); }
	;

enum_info
	: ident { GFileParser.m_parser->StartEnum( $<m_context>1, $<m_string>1 ); }
	;
 
enum_name_list 
	: enum_name_list ',' enum_name	{}
	| enum_name						{}
	;
 
enum_name
	: ident						{ GFileParser.m_parser->AddEmumOption( $<m_context>1, $<m_string>1, INT_MAX ); }
	| ident	'=' integer			{ GFileParser.m_parser->AddEmumOption( $<m_context>1, $<m_string>1, $<m_integer>3 ); }
	| ident	'=' '-' integer		{ GFileParser.m_parser->AddEmumOption( $<m_context>1, $<m_string>1, -$<m_integer>4 ); }
	;

////////////////////////////////
// DEFAULTS
////////////////////////////////

defaults_dcl
	: TOKEN_DEFAULTS '{' defaults_val_list '}'
	| TOKEN_DEFAULT default_val	
	;
	
defaults_val_list
	: default_val defaults_val_list 
	| /* empty */
	;
	
default_val
	: ident '=' script_unnamed_value semicolon 		{ GFileParser.m_parser->AddDefaultValue( $<m_context>1, $<m_string>1, $<m_value>3 ); }
	| ident '=' '-' integer semicolon 				{ GFileParser.m_parser->AddDefaultValue( $<m_context>1, $<m_string>1, new CScriptDefaultValue( $<m_context>4, ToString( -$<m_integer>4 ) ) ); }
	| ident '=' '-' float semicolon 				{ GFileParser.m_parser->AddDefaultValue( $<m_context>1, $<m_string>1, new CScriptDefaultValue( $<m_context>4, ToString( -$<m_float>4 ) ) ); }
	;
	
////////////////////////////////
// HINT
////////////////////////////////

hint_dcl
	: TOKEN_HINT hint_val
	;

hint_val
	: ident '=' string semicolon					{ GFileParser.m_parser->AddHint( $<m_context>1, $<m_string>1, $<m_string>3 ); }
	;

////////////////////////////////
// SCRIPT VALUE
////////////////////////////////

script_value
	: script_named_value				{ $<m_value>$ = $<m_value>1; }	
	| script_unnamed_value				{ $<m_value>$ = $<m_value>1; }
	;	

script_named_value	
	: ident '=' script_unnamed_value	{ $<m_value>$ = $<m_value>3; $<m_value>3->SetName( CName( $<m_string>1 ) ); }
	;

script_unnamed_value
	: basic_value				{ $<m_value>$ = new CScriptDefaultValue( $<m_context>1, $<m_string>1 ); }
	| script_compound_value		{ $<m_value>$ = $<m_value>1; }
	| script_object_value		{ $<m_value>$ = $<m_value>1; }	
	;

script_compound_value
	: script_compound_value_header script_compound_value_list_def '}'	{ $<m_value>$ = $<m_value>1; GFileParser.m_spawnInfo.PopValue(); }
	;

script_compound_value_header
	: '{'						{ $<m_value>$ = GFileParser.m_spawnInfo.PushValue( new CScriptDefaultValue( $<m_context>1, String::EMPTY ) ); }
	;

script_compound_value_list_def
	: script_compound_value_list
	| /* empty */
	;

script_compound_value_list
	: script_compound_value_list ',' script_value 		{ GFileParser.m_spawnInfo.AddSubValue( $<m_value>3 ); }
	| script_value										{ GFileParser.m_spawnInfo.AddSubValue( $<m_value>1 ); }
	;

script_object_value
	: script_object_value_header script_object_value_list_def	 ')'	{ $<m_value>$ = $<m_value>1; GFileParser.m_spawnInfo.PopValue(); }
	;

script_object_value_header
	: '('				{ $<m_value>$ = GFileParser.m_spawnInfo.PushValue( new CScriptDefaultValue( $<m_context>1, String::EMPTY ) ); }
	| ident '('			{ $<m_value>$ = GFileParser.m_spawnInfo.PushValue( new CScriptDefaultValue( $<m_context>1, $<m_string>1 ) ); }
	;

script_object_value_list_def	
	: script_object_value_list
	| /* empty */
	;

script_object_value_list
	: script_object_value_list ',' script_named_value	{ GFileParser.m_spawnInfo.AddSubValue( $<m_value>3 ); }
	| script_named_value								{ GFileParser.m_spawnInfo.AddSubValue( $<m_value>1 ); }
	;

////////////////////////////////
// FUNCTION
////////////////////////////////

function_dcl
	: function_header
	;

function_header
	: function_inner_header '(' function_params ')' function_ret_value function_tail 
	;

function_ret_value
	: ':' full_type		{ GFileParser.m_parser->SetReturnValueType( $<m_typeName>2 ); }
	| /* empty */
	;

function_inner_header
	:	import_flag
		access_flag
		function_flag_final
		function_flag_latent
		function_flag_type
		TOKEN_FUNCTION
		ident
		{
			GFileParser.m_parser->StartFunction
			(
				$<m_context>6,
				$<m_string>7,
				$<m_dword>1 |
				$<m_dword>2 |
				$<m_dword>3 |
				$<m_dword>4 |
				$<m_dword>5
			);
		}
	| TOKEN_EVENT ident { GFileParser.m_parser->StartFunction( $<m_context>1, $<m_string>2, SSF_Event ); }
	;

function_params
	: function_param_list 
	| /* empty */
	;

function_param_list
	: function_param ',' function_param_list
	| function_param
	;

function_param
	: function_param_flag_list function_param_ident_list ':' full_type
		{
			const YYSTYPE_File::Idents& idents = $<m_idents>2;
			for ( Uint32 i = 0; i < idents.Size(); ++i )
			{
				GFileParser.m_parser->AddProperty( $<m_context>4, idents[i], $<m_dword>1, $<m_typeName>4, true );
			}
		}
	;

function_param_ident_list
	: ident ',' function_param_ident_list	{ $<m_idents>$.PushBack( $<m_string>1 ); $<m_idents>$.PushBack( $<m_idents>3 );	}
	| ident									{ $<m_idents>$.PushBack( $<m_string>1 ); }
	; 

function_param_flag_list
	: function_param_flag function_param_flag_list     { $<m_dword>$ = $<m_dword>1 | $<m_dword>2; }
	| /* empty */                                      { $<m_dword>$ = 0; } 
	;

function_param_flag
	: TOKEN_OUT                { $<m_dword>$ = SSF_Out; }
	| TOKEN_OPTIONAL           { $<m_dword>$ = SSF_Optional; }
	;

function_tail
	: '{' function_vars_dcl function_body '}'    { GFileParser.m_parser->PopContext(); }
	| semicolon									 { GFileParser.m_parser->SetFunctionUndefined(); GFileParser.m_parser->PopContext(); }
	;

function_body
	: /* empty */ { CScriptTokenStream functionCode; GFileParser.m_tokens->ExtractFunctionTokens( functionCode ); GFileParser.m_parser->AddCode( functionCode ); yyclearin; }
	;

function_vars_dcl
	: function_vars_header function_var_dcl ';' function_vars_dcl
	| function_vars_header function_var_single_dcl '=' function_var_initializer ';' function_vars_dcl
	| /* empty */
	;

function_vars_header
	: TOKEN_VAR	{ GFileParser.m_parser->ResetFunctionPropertyList(); }
	;

function_var_dcl
	: function_var_single_dcl ',' function_var_dcl
	| function_var_single_dcl
	;

function_var_single_dcl
	: function_param_ident_list ':' full_type
	{
		const YYSTYPE_File::Idents& idents = $<m_idents>1;
		for( Uint32 i = 0; i < idents.Size(); ++i )
		{
			GFileParser.m_parser->AddProperty( $<m_context>3, idents[ i ], 0, $<m_typeName>3, false );
		}
	};

function_var_initializer
	: /* empty */ { CScriptTokenStream initCode; GFileParser.m_tokens->ExtractInitCode( initCode ); GFileParser.m_parser->SetLastFunctionPropertyInitCode( initCode ); yyclearin; }
	;


////////////////////////////////
// TERMINALS
//////////////////////////////// 

full_type
	: basic_type		{ $<m_typeName>$ = $<m_typeName>1; }
	| array_type		{ $<m_typeName>$ = $<m_typeName>1; }
	| static_array_type	{ $<m_typeName>$ = $<m_typeName>1; }
	| ident				{ $<m_typeName>$ = $<m_string>1; }
	;

basic_type
	: TOKEN_TYPE_STRING	{ $<m_typeName>$ = GetTypeName<String>().AsString() }
	| TOKEN_TYPE_BYTE	{ $<m_typeName>$ = GetTypeName<Uint8>().AsString() }
	| TOKEN_TYPE_BOOL	{ $<m_typeName>$ = GetTypeName<Bool>().AsString() }
	| TOKEN_TYPE_INT	{ $<m_typeName>$ = GetTypeName<Int32>().AsString() }
	| TOKEN_TYPE_FLOAT	{ $<m_typeName>$ = GetTypeName<Float>().AsString() }
	| TOKEN_TYPE_NAME	{ $<m_typeName>$ = GetTypeName<CName>().AsString(); }
	| TOKEN_TYPE_VOID	{ $<m_typeName>$ = String::EMPTY; }
	;

array_type
	: TOKEN_ARRAY '<' full_type '>'	{ $<m_typeName>$ = GetDynamicArrayTypeName( $<m_typeName>3 ); }
	;

static_array_type
	: full_type '[' integer ']'		{ $<m_typeName>$ = GetStaticArrayTypeName( $<m_typeName>1, $<m_integer>3 ); }
	;

basic_value
	: integer			{ $<m_string>$ = $<m_string>1; $<m_context>$ = $<m_context>1; } 
	| float				{ $<m_string>$ = $<m_string>1; $<m_context>$ = $<m_context>1; } 
	| bool				{ $<m_string>$ = $<m_string>1; $<m_context>$ = $<m_context>1; } 
	| string			{ $<m_string>$ = $<m_string>1; $<m_context>$ = $<m_context>1; } 
	| name				{ $<m_string>$ = $<m_string>1; $<m_context>$ = $<m_context>1; } 
	| ident				{ $<m_string>$ = $<m_string>1; $<m_context>$ = $<m_context>1; } 
	| null				{ $<m_string>$ = $<m_string>1; $<m_context>$ = $<m_context>1; } 
	;

integer
	: TOKEN_INTEGER		{ RED_VERIFY( Red::System::StringToInt( $<m_integer>$, $<m_token>1.AsChar(), nullptr, Red::System::BaseAuto ), TXT( "Could not convert script token into integer" ) ); $<m_string>$ = $<m_token>1; $<m_context>$ = $<m_context>1; }
	;

float
	: TOKEN_FLOAT		{ $<m_float>$ = static_cast< Float >(  Red::System::StringToDouble( $<m_token>1.AsChar() ) ); $<m_string>$ = $<m_token>1; $<m_context>$ = $<m_context>1; }
	;

name
	: TOKEN_NAME		{ $<m_string>$ = $<m_token>1; $<m_context>$ = $<m_context>1; }
	;

string
	: TOKEN_STRING		{ $<m_string>$ = $<m_token>1; $<m_context>$ = $<m_context>1; }
	;

bool
	: TOKEN_BOOL_TRUE	{ $<m_bool>$ = true; $<m_string>$ = $<m_token>1; $<m_context>$ = $<m_context>1; }
	| TOKEN_BOOL_FALSE	{ $<m_bool>$ = false; $<m_string>$ = $<m_token>1; $<m_context>$ = $<m_context>1; }
	;

ident
	: TOKEN_IDENT		{ $<m_string>$ = $<m_token>1; $<m_context>$ = $<m_context>1; }
	; 

null
	: TOKEN_NULL		{ $<m_string>$ = $<m_token>1; $<m_context>$ = $<m_context>1; }
	;

semicolon
	: ';'
	;

optional_semicolon
	: ';'
	| /* empty */
	;

optional_comma
	: ','
	| /* empty */
	;
